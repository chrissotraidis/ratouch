#include "ratouch_gameplay.h"

#include <cassert>

int main()
{
    assert(!Ratouch_Is_Gameplay_Active());
    assert(!Ratouch_Should_Autosave(false));
    assert(!Ratouch_Should_Autosave(true));

    Ratouch_Set_Gameplay_Active(true);
    assert(Ratouch_Is_Gameplay_Active());
    assert(!Ratouch_Should_Autosave(false));
    assert(Ratouch_Should_Autosave(true));

    Ratouch_Set_Gameplay_Active(false);
    assert(!Ratouch_Is_Gameplay_Active());
    assert(!Ratouch_Should_Autosave(true));
    return 0;
}
