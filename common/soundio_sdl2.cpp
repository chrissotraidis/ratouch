// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

#include "soundio_imp.h"

#include <SDL.h>
#include <algorithm>
#include <cstring>
#include <vector>

namespace
{
constexpr int BUFFER_COUNT = 2;

struct AudioState;
AudioState* State = nullptr;

SDL_AudioFormat Audio_Format(int bits)
{
    return bits == 8 ? AUDIO_U8 : AUDIO_S16SYS;
}
}

struct SampleTrackerTypeImp
{
    SDL_AudioStream* Stream = nullptr;
    Uint8* Scratch = nullptr;
    int ScratchSize = 0;
    int BitsPerSample = 0;
    int Channels = 0;
    int Frequency = 0;
    int LastChunkBytes = 1;
    unsigned int Volume = 65536;
    bool Playing = false;
    bool Started = false;
    int EmptyCallbacks = 0;
};

namespace
{
struct AudioState
{
    SDL_AudioDeviceID Device = 0;
    SDL_AudioSpec Output = {};
    std::vector<SampleTrackerTypeImp*> Samples;
    bool ReverseChannels = false;
    bool OwnsAudioSubsystem = false;
};

void Audio_Callback(void*, Uint8* output, int length)
{
    std::memset(output, 0, length);
    if (State == nullptr) {
        return;
    }

    for (SampleTrackerTypeImp* sample : State->Samples) {
        if (!sample->Playing || sample->Stream == nullptr) {
            continue;
        }

        const int bytes = SDL_AudioStreamGet(sample->Stream, sample->Scratch, std::min(length, sample->ScratchSize));
        if (bytes > 0) {
            sample->EmptyCallbacks = 0;
            const int volume = std::min<int>(SDL_MIX_MAXVOLUME,
                                             sample->Volume * SDL_MIX_MAXVOLUME / 65536U);
            SDL_MixAudioFormat(output, sample->Scratch, State->Output.format, bytes, volume);
        }
        if (bytes <= 0 && SDL_AudioStreamAvailable(sample->Stream) <= 0) {
            ++sample->EmptyCallbacks;
            if (sample->EmptyCallbacks > 1) {
                sample->Playing = false;
            }
        }
    }

    if (State->ReverseChannels && State->Output.channels == 2 && State->Output.format == AUDIO_S16SYS) {
        Sint16* frames = reinterpret_cast<Sint16*>(output);
        for (int i = 0; i + 1 < length / static_cast<int>(sizeof(Sint16)); i += 2) {
            std::swap(frames[i], frames[i + 1]);
        }
    }
}

SDL_AudioStream* Create_Stream(int bits, int channels, int rate)
{
    return SDL_NewAudioStream(Audio_Format(bits),
                              static_cast<Uint8>(channels),
                              rate,
                              State->Output.format,
                              State->Output.channels,
                              State->Output.freq);
}
}

void SoundImp_Buffer_Sample_Data(SampleTrackerTypeImp* sample, const void* data, size_t length)
{
    if (State == nullptr || sample == nullptr || sample->Stream == nullptr || data == nullptr || length == 0) {
        return;
    }

    SDL_LockAudioDevice(State->Device);
    const int before = SDL_AudioStreamAvailable(sample->Stream);
    if (SDL_AudioStreamPut(sample->Stream, data, static_cast<int>(length)) == 0) {
        const int after = SDL_AudioStreamAvailable(sample->Stream);
        sample->LastChunkBytes = std::max(1, after - before);
        sample->EmptyCallbacks = 0;
        if (sample->Started) {
            sample->Playing = true;
        }
    }
    SDL_UnlockAudioDevice(State->Device);
}

int SoundImp_Get_Sample_Free_Buffer_Count(SampleTrackerTypeImp* sample)
{
    if (State == nullptr || sample == nullptr || sample->Stream == nullptr) {
        return 0;
    }

    SDL_LockAudioDevice(State->Device);
    const int available = std::max(0, SDL_AudioStreamAvailable(sample->Stream));
    const int used = std::min(BUFFER_COUNT, (available + sample->LastChunkBytes - 1) / sample->LastChunkBytes);
    SDL_UnlockAudioDevice(State->Device);
    return BUFFER_COUNT - used;
}

bool SoundImp_Init(int, bool, int rate, bool reverse_channels)
{
    if (State != nullptr) {
        return true;
    }

    State = new AudioState();
    State->ReverseChannels = reverse_channels;
    State->OwnsAudioSubsystem = (SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        delete State;
        State = nullptr;
        return false;
    }

    SDL_AudioSpec desired = {};
    desired.freq = rate;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = Audio_Callback;

    State->Device = SDL_OpenAudioDevice(nullptr, 0, &desired, &State->Output, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (State->Device == 0) {
        if (State->OwnsAudioSubsystem) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
        }
        delete State;
        State = nullptr;
        return false;
    }
    return true;
}

void SoundImp_PauseSound()
{
    if (State != nullptr) {
        SDL_PauseAudioDevice(State->Device, 1);
    }
}

bool SoundImp_ResumeSound()
{
    if (State == nullptr) {
        return false;
    }
    SDL_PauseAudioDevice(State->Device, 0);
    return true;
}

SampleTrackerTypeImp* SoundImp_Init_Sample(int bits, bool stereo, int rate)
{
    if (State == nullptr) {
        return nullptr;
    }

    SampleTrackerTypeImp* sample = new SampleTrackerTypeImp();
    sample->BitsPerSample = bits;
    sample->Channels = stereo ? 2 : 1;
    sample->Frequency = rate;
    sample->ScratchSize = State->Output.size;
    sample->Scratch = new Uint8[sample->ScratchSize];
    sample->Stream = Create_Stream(bits, sample->Channels, rate);
    if (sample->Stream == nullptr) {
        delete[] sample->Scratch;
        delete sample;
        return nullptr;
    }

    SDL_LockAudioDevice(State->Device);
    State->Samples.push_back(sample);
    SDL_UnlockAudioDevice(State->Device);
    return sample;
}

bool SoundImp_Sample_Status(SampleTrackerTypeImp* sample)
{
    if (State == nullptr || sample == nullptr) {
        return false;
    }
    SDL_LockAudioDevice(State->Device);
    const bool playing = sample->Playing;
    SDL_UnlockAudioDevice(State->Device);
    return playing;
}

void SoundImp_Set_Sample_Attributes(SampleTrackerTypeImp* sample, int bits, bool stereo, int rate)
{
    if (State == nullptr || sample == nullptr) {
        return;
    }

    SDL_LockAudioDevice(State->Device);
    SDL_AudioStream* replacement = Create_Stream(bits, stereo ? 2 : 1, rate);
    if (replacement != nullptr) {
        SDL_FreeAudioStream(sample->Stream);
        sample->Stream = replacement;
        sample->BitsPerSample = bits;
        sample->Channels = stereo ? 2 : 1;
        sample->Frequency = rate;
        sample->LastChunkBytes = 1;
        sample->Playing = false;
        sample->Started = false;
        sample->EmptyCallbacks = 0;
    }
    SDL_UnlockAudioDevice(State->Device);
}

void SoundImp_Set_Sample_Volume(SampleTrackerTypeImp* sample, unsigned int volume)
{
    if (State == nullptr || sample == nullptr) {
        return;
    }
    SDL_LockAudioDevice(State->Device);
    sample->Volume = std::min(volume, 65536U);
    SDL_UnlockAudioDevice(State->Device);
}

void SoundImp_Shutdown()
{
    if (State == nullptr) {
        return;
    }
    SDL_CloseAudioDevice(State->Device);
    const bool quit_audio = State->OwnsAudioSubsystem;
    delete State;
    State = nullptr;
    if (quit_audio) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

void SoundImp_Shutdown_Sample(SampleTrackerTypeImp* sample)
{
    if (State == nullptr || sample == nullptr) {
        return;
    }
    SDL_LockAudioDevice(State->Device);
    State->Samples.erase(std::remove(State->Samples.begin(), State->Samples.end(), sample), State->Samples.end());
    SDL_FreeAudioStream(sample->Stream);
    SDL_UnlockAudioDevice(State->Device);
    delete[] sample->Scratch;
    delete sample;
}

void SoundImp_Start_Sample(SampleTrackerTypeImp* sample)
{
    if (State == nullptr || sample == nullptr) {
        return;
    }
    SDL_LockAudioDevice(State->Device);
    sample->Playing = SDL_AudioStreamAvailable(sample->Stream) > 0;
    sample->Started = sample->Playing;
    sample->EmptyCallbacks = 0;
    SDL_UnlockAudioDevice(State->Device);
}

void SoundImp_Stop_Sample(SampleTrackerTypeImp* sample)
{
    if (State == nullptr || sample == nullptr) {
        return;
    }
    SDL_LockAudioDevice(State->Device);
    SDL_AudioStreamClear(sample->Stream);
    sample->Playing = false;
    sample->Started = false;
    sample->EmptyCallbacks = 0;
    sample->LastChunkBytes = 1;
    SDL_UnlockAudioDevice(State->Device);
}
