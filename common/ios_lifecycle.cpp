#include "ios_lifecycle.h"

#include <SDL.h>
#include <atomic>

namespace
{
std::atomic<bool> Backgrounded(false);
std::atomic<bool> Inactive(false);
std::atomic<bool> AutosaveRequested(false);
RatouchAutosaveCallback Autosave = nullptr;

int Lifecycle_Filter(void*, SDL_Event* event)
{
    switch (event->type) {
    case SDL_APP_WILLENTERBACKGROUND:
        Inactive.store(true, std::memory_order_release);
        Backgrounded.store(true, std::memory_order_release);
        AutosaveRequested.store(true, std::memory_order_release);
        break;
    case SDL_APP_DIDENTERBACKGROUND:
        Backgrounded.store(true, std::memory_order_release);
        break;
    case SDL_APP_WILLENTERFOREGROUND:
        Backgrounded.store(false, std::memory_order_release);
        break;
    case SDL_APP_DIDENTERFOREGROUND:
        Backgrounded.store(false, std::memory_order_release);
        Inactive.store(false, std::memory_order_release);
        break;
    case SDL_APP_TERMINATING:
        AutosaveRequested.store(true, std::memory_order_release);
        break;
    default:
        break;
    }
    return 1;
}
}

void Ratouch_Install_iOS_Lifecycle_Filter()
{
    SDL_SetEventFilter(Lifecycle_Filter, nullptr);
}

void Ratouch_Set_iOS_Autosave_Callback(RatouchAutosaveCallback callback)
{
    Autosave = callback;
}

bool Ratouch_iOS_Should_Pause()
{
    return Backgrounded.load(std::memory_order_acquire) || Inactive.load(std::memory_order_acquire);
}

void Ratouch_iOS_Process_Pause()
{
    if (AutosaveRequested.exchange(false, std::memory_order_acq_rel) && Autosave != nullptr) {
        Autosave();
    }
    while (Ratouch_iOS_Should_Pause()) {
        SDL_PumpEvents();
        SDL_Delay(50);
    }
}
