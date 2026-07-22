#ifndef RATOUCH_IOS_CONTROLS_H
#define RATOUCH_IOS_CONTROLS_H

#include <stdint.h>

#define RATOUCH_IOS_TOUCH_PREFERENCES_CHANGED 0x52544346

#ifdef __cplusplus
extern "C" {
#endif

void Ratouch_Install_Command_Overlay(void);
void Ratouch_Set_Command_Overlay_Visible(bool visible);
void Ratouch_Set_iOS_Sidebar_Visible(bool visible);
void Ratouch_iOS_Confirm_Long_Press(void);
void Ratouch_iOS_Consume_One_Shot_Modifier(void);
void Ratouch_iOS_Cancel_One_Shot_Modifier(void);
uint64_t Ratouch_iOS_Long_Press_Milliseconds(void);
float Ratouch_iOS_Drag_Threshold(void);
bool Ratouch_iOS_Invert_Pan(void);

#ifdef __cplusplus
}
#endif

#endif
