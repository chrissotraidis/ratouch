#ifndef IOS_LIFECYCLE_H
#define IOS_LIFECYCLE_H

using RatouchAutosaveCallback = void (*)();
using RatouchFocusLossCallback = void (*)();

void Ratouch_Install_iOS_Lifecycle_Filter();
void Ratouch_Set_iOS_Autosave_Callback(RatouchAutosaveCallback callback);
void Ratouch_Set_iOS_Focus_Loss_Callback(RatouchFocusLossCallback callback);
bool Ratouch_iOS_Should_Pause();
void Ratouch_iOS_Process_Pause();

#endif
