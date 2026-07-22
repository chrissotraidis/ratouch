// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

#include "vqaaudio.h"
#include "audio.h"
#include "soundio_imp.h"
#include "vqafile.h"
#include "vqaloader.h"
#include "vqatask.h"

#include <SDL.h>
#include <chrono>
#include <cstring>

int AudioFlags;
int TimerIntCount;
int TimerMethod;
int VQATickCount;
int TickOffset;
unsigned VQAAudioPaused;
VQAHandle* AudioVQAHandle;

namespace
{
bool Queue_Audio(VQAHandle* handle)
{
    VQAConfig* config = &handle->Config;
    VQAAudio* audio = &handle->VQABuf->Audio;
    if (audio->SDLSample == nullptr || audio->IsLoaded[audio->field_10] != 1) {
        return false;
    }

    SoundImp_Buffer_Sample_Data(audio->SDLSample, &audio->Buffer[audio->PlayPosition], config->HMIBufSize);
    audio->IsLoaded[audio->field_10] = 0;

    audio->field_B8 = audio->field_B0;
    audio->field_B0 += config->HMIBufSize;
    if (audio->field_B0 >= audio->BuffBytes) {
        audio->field_B0 = 0;
    }

    audio->field_14 = audio->field_10 + 1;
    if (audio->field_14 >= audio->NumAudBlocks) {
        audio->field_14 = 0;
    }

    audio->PlayPosition += config->HMIBufSize;
    ++audio->field_10;
    if (audio->PlayPosition >= static_cast<unsigned>(config->AudioBufSize)) {
        audio->PlayPosition = 0;
        audio->field_10 = 0;
    }
    ++audio->field_B4;
    return true;
}

void Service_Audio(VQAHandle* handle)
{
    if (VQAAudioPaused || handle == nullptr) {
        return;
    }
    VQAAudio* audio = &handle->VQABuf->Audio;
    bool queued = false;
    while (audio->SDLSample != nullptr && SoundImp_Get_Sample_Free_Buffer_Count(audio->SDLSample) > 0) {
        if (!Queue_Audio(handle)) {
            break;
        }
        queued = true;
    }
    if (queued) {
        // An audio callback can briefly drain the stream between VQA frames.
        // Starting is idempotent and makes newly queued data resume immediately.
        SoundImp_Start_Sample(audio->SDLSample);
    }
}
}

int VQA_StartTimerInt(VQAHandle*, int)
{
    if (!(AudioFlags & VQA_AUDIO_FLAG_INTERRUPT_TIMER)) {
        AudioFlags |= VQA_AUDIO_FLAG_UNKNOWN016;
    }
    ++TimerIntCount;
    return 0;
}

void VQA_StopTimerInt(VQAHandle*)
{
    if (TimerIntCount > 0) {
        --TimerIntCount;
    }
    AudioFlags &= ~VQA_AUDIO_FLAG_INTERRUPT_TIMER;
}

int VQA_OpenAudio(VQAHandle* handle, void*)
{
    VQAConfig* config = &handle->Config;
    VQAAudio* audio = &handle->VQABuf->Audio;
    VQAHeader* header = &handle->Header;

    Start_Primary_Sound_Buffer(true);
    audio->field_10 = 0;
    audio->field_BC = 1;
    audio->SDLSample = nullptr;

    if (config->AudioRate == -1) {
        config->AudioRate = header->FPS == config->FrameRate
                                ? audio->SampleRate
                                : config->FrameRate * audio->SampleRate / header->FPS;
    }

    audio->field_C0 = 1;
    audio->field_BC = 0;
    audio->Flags |= VQA_AUDIO_FLAG_UNKNOWN001;
    AudioFlags |= VQA_AUDIO_FLAG_UNKNOWN001;
    return 0;
}

void VQA_CloseAudio(VQAHandle* handle)
{
    VQAAudio* audio = &handle->VQABuf->Audio;
    VQA_StopAudio(handle);
    AudioFlags &= ~(VQA_AUDIO_FLAG_UNKNOWN004 | VQA_AUDIO_FLAG_UNKNOWN008);
    audio->Flags &= ~(VQA_AUDIO_FLAG_UNKNOWN004 | VQA_AUDIO_FLAG_UNKNOWN008);
    audio->field_C0 = 0;
    audio->field_BC = 0;
    audio->Flags &= ~(VQA_AUDIO_FLAG_UNKNOWN001 | VQA_AUDIO_FLAG_UNKNOWN002);
    AudioFlags &= ~(VQA_AUDIO_FLAG_UNKNOWN001 | VQA_AUDIO_FLAG_UNKNOWN002 | VQA_AUDIO_FLAG_AUDIO_DMA_TIMER);
}

int VQA_StartAudio(VQAHandle* handle)
{
    VQAAudio* audio = &handle->VQABuf->Audio;
    VQAConfig* config = &handle->Config;
    AudioVQAHandle = handle;

    if (AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) {
        return -1;
    }
    if (audio->SDLSample != nullptr) {
        SoundImp_Shutdown_Sample(audio->SDLSample);
        audio->SDLSample = nullptr;
    }

    audio->SDLSample = SoundImp_Init_Sample(audio->BitsPerSample, audio->Channels > 1, audio->SampleRate);
    if (audio->SDLSample == nullptr) {
        AudioVQAHandle = nullptr;
        return -1;
    }
    SoundImp_Set_Sample_Volume(audio->SDLSample, static_cast<unsigned>(config->Volume) * 256U);

    audio->BuffBytes = config->HMIBufSize * 4;
    audio->field_B0 = 0;
    audio->field_B4 = 0;
    for (int i = 0; i < 2; ++i) {
        Queue_Audio(handle);
    }
    SoundImp_Start_Sample(audio->SDLSample);
    audio->Flags |= VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    AudioFlags |= VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    return 0;
}

void VQA_StopAudio(VQAHandle* handle)
{
    if (handle == nullptr || handle->VQABuf == nullptr) {
        return;
    }
    VQAAudio* audio = &handle->VQABuf->Audio;
    if (audio->SDLSample != nullptr) {
        SoundImp_Stop_Sample(audio->SDLSample);
        SoundImp_Shutdown_Sample(audio->SDLSample);
        audio->SDLSample = nullptr;
    }
    audio->Flags &= ~VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    AudioFlags &= ~VQA_AUDIO_FLAG_AUDIO_DMA_TIMER;
    AudioVQAHandle = nullptr;
}

void VQA_PauseAudio()
{
    if (AudioVQAHandle != nullptr && AudioVQAHandle->VQABuf != nullptr) {
        VQAAudio* audio = &AudioVQAHandle->VQABuf->Audio;
        if (audio->SDLSample != nullptr && (AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) && !VQAAudioPaused) {
            SoundImp_PauseSound();
            VQAAudioPaused = VQA_GetTime(AudioVQAHandle);
        }
    }
}

void VQA_ResumeAudio()
{
    if (AudioVQAHandle != nullptr && AudioVQAHandle->VQABuf != nullptr) {
        VQAAudio* audio = &AudioVQAHandle->VQABuf->Audio;
        if (audio->SDLSample != nullptr && (AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) && VQAAudioPaused) {
            SoundImp_ResumeSound();
            TickOffset -= VQA_GetTime(AudioVQAHandle) - VQAAudioPaused;
            VQAAudioPaused = 0;
        }
    }
}

int VQA_CopyAudio(VQAHandle* handle)
{
    VQAConfig* config = &handle->Config;
    VQAAudio* audio = &handle->VQABuf->Audio;
    Service_Audio(handle);

    if ((config->OptionFlags & 1) && audio->Buffer != nullptr && audio->TempBufSize > 0) {
        int current_block = audio->AudBufPos / config->HMIBufSize;
        int next_block = (audio->TempBufSize + audio->AudBufPos) / config->HMIBufSize;
        if (static_cast<unsigned>(next_block) >= audio->NumAudBlocks) {
            next_block -= audio->NumAudBlocks;
        }
        if (audio->IsLoaded[next_block] == 1) {
            return -10;
        }

        if (next_block < current_block) {
            const int end_space = config->AudioBufSize - audio->AudBufPos;
            const int remaining = audio->TempBufSize - end_space;
            std::memcpy(&audio->Buffer[audio->AudBufPos], audio->TempBuf, end_space);
            std::memcpy(audio->Buffer, &audio->TempBuf[end_space], remaining);
            audio->AudBufPos = remaining;
            audio->TempBufSize = 0;
            for (unsigned i = current_block; i < audio->NumAudBlocks; ++i) {
                audio->IsLoaded[i] = 1;
            }
            for (int i = 0; i < next_block; ++i) {
                audio->IsLoaded[i] = 1;
            }
        } else {
            std::memcpy(&audio->Buffer[audio->AudBufPos], audio->TempBuf, audio->TempBufSize);
            audio->AudBufPos += audio->TempBufSize;
            audio->TempBufSize = 0;
            for (int i = current_block; i < next_block; ++i) {
                audio->IsLoaded[i] = 1;
            }
        }
    }
    return 0;
}

void VQA_SetTimer(VQAHandle* handle, int time, int method)
{
    if (method == -1) {
        if (AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) {
            method = VQA_AUDIO_TIMER_METHOD_DMA;
        } else if (AudioFlags & (VQA_AUDIO_FLAG_UNKNOWN016 | VQA_AUDIO_FLAG_UNKNOWN032)) {
            method = VQA_AUDIO_TIMER_METHOD_INTERRUPT;
        } else {
            method = VQA_AUDIO_TIMER_METHOD_DOS;
        }
    } else if (!(AudioFlags & VQA_AUDIO_FLAG_AUDIO_DMA_TIMER) && method == VQA_AUDIO_TIMER_METHOD_DMA) {
        method = VQA_AUDIO_TIMER_METHOD_INTERRUPT;
    } else if (!(AudioFlags & (VQA_AUDIO_FLAG_UNKNOWN016 | VQA_AUDIO_FLAG_UNKNOWN032))
               && method == VQA_AUDIO_TIMER_METHOD_INTERRUPT) {
        method = VQA_AUDIO_TIMER_METHOD_DOS;
    }

    TimerMethod = method;
    TickOffset = 0;
    TickOffset = time - VQA_GetTime(handle);
}

unsigned VQA_GetTime(VQAHandle*)
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<unsigned>(TickOffset
                                 + 60 * std::chrono::duration_cast<std::chrono::milliseconds>(now).count() / 1000);
}

int VQA_TimerMethod()
{
    return TimerMethod;
}
