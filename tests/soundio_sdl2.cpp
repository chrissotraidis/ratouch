#include "soundio_imp.h"

#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace
{
int Fail(const char* message)
{
    std::fprintf(stderr, "%s\n", message);
    return 1;
}
}

int main()
{
    if (!SoundImp_Init(16, true, 22050, false)) {
        return Fail("SDL audio backend did not initialize");
    }

    SampleTrackerTypeImp* sample = SoundImp_Init_Sample(16, true, 22050);
    if (sample == nullptr) {
        SoundImp_Shutdown();
        return Fail("SDL audio sample did not initialize");
    }

    std::vector<int16_t> pcm(4096, 1000);
    if (SoundImp_Get_Sample_Free_Buffer_Count(sample) != 2) {
        return Fail("new sample did not report two free buffers");
    }

    SoundImp_Buffer_Sample_Data(sample, pcm.data(), pcm.size() * sizeof(int16_t));
    if (SoundImp_Get_Sample_Free_Buffer_Count(sample) >= 2) {
        return Fail("queued sample did not consume a buffer");
    }

    SoundImp_Set_Sample_Volume(sample, 32768);
    SoundImp_Start_Sample(sample);
    if (!SoundImp_Sample_Status(sample)) {
        return Fail("queued sample did not start");
    }

    SampleTrackerTypeImp* second_sample = SoundImp_Init_Sample(16, true, 22050);
    if (second_sample == nullptr) {
        return Fail("second SDL audio sample did not initialize");
    }
    SoundImp_Buffer_Sample_Data(second_sample, pcm.data(), pcm.size() * sizeof(int16_t));
    SoundImp_Start_Sample(second_sample);
    if (!SoundImp_Sample_Status(second_sample)) {
        return Fail("parallel sample did not start");
    }

    if (!SoundImp_ResumeSound()) {
        return Fail("SDL audio backend did not resume");
    }
    SDL_Delay(20);
    SoundImp_PauseSound();
    SoundImp_Stop_Sample(sample);
    SoundImp_Stop_Sample(second_sample);
    if (SoundImp_Sample_Status(sample)) {
        return Fail("stopped sample still reports playing");
    }
    if (SoundImp_Sample_Status(second_sample)) {
        return Fail("stopped parallel sample still reports playing");
    }
    SoundImp_Shutdown_Sample(second_sample);

    SoundImp_Buffer_Sample_Data(sample, pcm.data(), pcm.size() * sizeof(int16_t));
    SoundImp_Start_Sample(sample);
    if (!SoundImp_ResumeSound()) {
        return Fail("SDL audio backend did not resume for refill test");
    }
    SDL_Delay(250);
    if (SoundImp_Sample_Status(sample)) {
        return Fail("drained sample did not report its temporary underrun");
    }
    SoundImp_Buffer_Sample_Data(sample, pcm.data(), pcm.size() * sizeof(int16_t));
    if (!SoundImp_Sample_Status(sample)) {
        return Fail("streaming sample did not resume after a refill gap");
    }
    SoundImp_Stop_Sample(sample);

    SoundImp_Set_Sample_Attributes(sample, 8, false, 11025);
    std::vector<uint8_t> pcm8(1024, 128);
    SoundImp_Buffer_Sample_Data(sample, pcm8.data(), pcm8.size());
    SoundImp_Start_Sample(sample);
    if (!SoundImp_Sample_Status(sample)) {
        return Fail("reconfigured sample did not start");
    }

    SoundImp_Stop_Sample(sample);
    SoundImp_Shutdown_Sample(sample);
    SoundImp_Shutdown();
    return 0;
}
