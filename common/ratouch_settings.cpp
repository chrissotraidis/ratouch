#include "ratouch_settings.h"

int Ratouch_Normalize_App_Setting(int setting, int value)
{
    switch (setting) {
    case RATOUCH_SETTING_SCALE_FILTER:
    case RATOUCH_SETTING_ASPECT_MODE:
        return value > 0 ? 1 : 0;
    case RATOUCH_SETTING_MUSIC_VOLUME:
    case RATOUCH_SETTING_SOUND_VOLUME:
        if (value < 0) {
            return 0;
        }
        if (value > 100) {
            return 100;
        }
        return value;
    default:
        return 0;
    }
}
