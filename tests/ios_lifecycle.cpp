#include "ios_lifecycle.h"

#include <SDL.h>
#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>

namespace
{
std::atomic<int> Saves(0);

void Autosave()
{
    ++Saves;
}

void Push(uint32_t type)
{
    SDL_Event event;
    SDL_zero(event);
    event.type = type;
    assert(SDL_PushEvent(&event) == 1);
}
}

int main()
{
    assert(SDL_Init(SDL_INIT_EVENTS) == 0);
    Ratouch_Set_iOS_Autosave_Callback(Autosave);
    Ratouch_Install_iOS_Lifecycle_Filter();

    for (int cycle = 0; cycle < 100; ++cycle) {
        Push(SDL_APP_WILLENTERBACKGROUND);
        Push(SDL_APP_DIDENTERBACKGROUND);
        assert(Ratouch_iOS_Should_Pause());

        std::thread resume([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            Push(SDL_APP_WILLENTERFOREGROUND);
            Push(SDL_APP_DIDENTERFOREGROUND);
        });
        Ratouch_iOS_Process_Pause();
        resume.join();

        assert(!Ratouch_iOS_Should_Pause());
        assert(Saves.load() == cycle + 1);
        Ratouch_iOS_Process_Pause();
        assert(Saves.load() == cycle + 1);
    }

    Push(SDL_APP_TERMINATING);
    Ratouch_iOS_Process_Pause();
    assert(Saves.load() == 101);

    SDL_Quit();
    return 0;
}
