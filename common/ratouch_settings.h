#ifndef RATOUCH_SETTINGS_H
#define RATOUCH_SETTINGS_H

#define RATOUCH_APP_SETTING_CHANGED 0x52545345

enum RatouchAppSetting
{
    RATOUCH_SETTING_SCALE_FILTER = 0,
    RATOUCH_SETTING_ASPECT_MODE = 1,
    RATOUCH_SETTING_MUSIC_VOLUME = 2,
    RATOUCH_SETTING_SOUND_VOLUME = 3,
};

#ifdef __cplusplus
extern "C" {
#endif

int Ratouch_Normalize_App_Setting(int setting, int value);

#if defined(IOS_BUILD) || defined(RATOUCH_MACOS_BUILD)
int Ratouch_Apple_App_Setting(int setting);
void Ratouch_Apple_Request_App_Setting(int setting, int value);
void Ratouch_Apple_Apply_App_Setting(int setting, int value);
#endif

#ifdef __cplusplus
}
#endif

#endif
