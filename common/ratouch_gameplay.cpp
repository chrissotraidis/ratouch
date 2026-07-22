#include "ratouch_gameplay.h"

namespace
{
// Platform callbacks request work through SDL; this state is owned by the engine thread.
bool GameplayActive = false;
}

void Ratouch_Set_Gameplay_Active(bool active)
{
    GameplayActive = active;
}

bool Ratouch_Is_Gameplay_Active()
{
    return GameplayActive;
}

bool Ratouch_Should_Autosave(bool engine_game_active)
{
    return GameplayActive && engine_game_active;
}
