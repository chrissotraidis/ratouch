#include "ratouch_settings.h"

#include <cassert>

int main()
{
    assert(Ratouch_Normalize_App_Setting(RATOUCH_SETTING_SCALE_FILTER, 0) == 0);
    assert(Ratouch_Normalize_App_Setting(RATOUCH_SETTING_SCALE_FILTER, 9) == 1);
    assert(Ratouch_Normalize_App_Setting(RATOUCH_SETTING_ASPECT_MODE, -1) == 0);
    assert(Ratouch_Normalize_App_Setting(RATOUCH_SETTING_MUSIC_VOLUME, -1) == 0);
    assert(Ratouch_Normalize_App_Setting(RATOUCH_SETTING_MUSIC_VOLUME, 37) == 37);
    assert(Ratouch_Normalize_App_Setting(RATOUCH_SETTING_SOUND_VOLUME, 101) == 100);
    assert(Ratouch_Normalize_App_Setting(999, 50) == 0);
    return 0;
}
