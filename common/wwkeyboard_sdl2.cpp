//
// Copyright 2020 Electronic Arts Inc.
//
// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free
// software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed
// in the hope that it will be useful, but with permitted additional restrictions
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT
// distributed with this program. You should have received a copy of the
// GNU General Public License along with permitted additional restrictions
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection

#include "macros.h"
#include "wwkeyboard_sdl2.h"
#include "video.h"
#include "video_geometry.h"
#include "sdl_keymap.h"
#include "settings.h"
#include "ratouch_settings.h"
#ifdef IOS_BUILD
#include "ios_controls.h"
#include "ios_lifecycle.h"
extern bool InMovie;
#endif
#include <cmath>
#include <SDL.h>

void Focus_Loss();
void Focus_Restore();
void Focus_Refresh();
void Process_Network();
#ifdef RATOUCH_MACOS_BUILD
void Ratouch_Mac_Prepare_Quit();
#endif

WWKeyboardClassSDL2::WWKeyboardClassSDL2()
{
#ifdef IOS_BUILD
    Refresh_Touch_Preferences();
#endif
}

WWKeyboardClassSDL2::~WWKeyboardClassSDL2()
{
}

void WWKeyboardClassSDL2::Fill_Buffer_From_System(void)
{
#ifdef IOS_BUILD
    Ratouch_iOS_Process_Pause();
#endif
#ifdef NETWORKING
    Process_Network();
#endif
    SDL_Event event;

#ifdef IOS_BUILD
    TouchScroll.Clear();
    MouseMovieInput.Begin_Poll(InMovie);
    TouchMovieInput.Begin_Poll(InMovie);
    Handle_Touch_Actions(Touch.Poll(SDL_GetTicks64()));
#endif

    while (!Is_Buffer_Full() && SDL_PollEvent(&event)) {
        unsigned short key;
        switch (event.type) {
        case SDL_QUIT:
#ifdef RATOUCH_MACOS_BUILD
            Ratouch_Mac_Prepare_Quit();
#endif
            exit(0);
            break;
        case SDL_KEYDOWN:
            Put_Key_Message(event.key.keysym.scancode, false);
            break;
        case SDL_KEYUP:
            if (event.key.keysym.scancode == SDL_SCANCODE_RETURN && Down(VK_MENU)) {
                Toggle_Video_Fullscreen();
            } else {
                Put_Key_Message(event.key.keysym.scancode, true);
            }
            break;
        case SDL_MOUSEMOTION:
#ifdef IOS_BUILD
            if (event.motion.which == SDL_TOUCH_MOUSEID) {
                break;
            }
            PointerOwner.Observe_Pointer();
#endif
            if (Is_Gamepad_Active() || Is_Video_Relative_Mouse_Active()) {
                float game_xrel = 0.0f;
                float game_yrel = 0.0f;
                Map_Video_Window_Delta(
                    static_cast<float>(event.motion.xrel), static_cast<float>(event.motion.yrel), game_xrel, game_yrel);
                Move_Video_Mouse(game_xrel, game_yrel);
            } else {
                int game_x = 0;
                int game_y = 0;
                Map_Video_Window_Point(event.motion.x, event.motion.y, game_x, game_y);
                Set_Video_Mouse(game_x, game_y);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
#ifdef IOS_BUILD
            if (event.button.which == SDL_TOUCH_MOUSEID) {
                break;
            }
            PointerOwner.Observe_Pointer();

            const WWMovieInputAction movie_action = MouseMovieInput.Handle(
                InMovie,
                event.type == SDL_MOUSEBUTTONDOWN ? WWMovieInputPhase::Down : WWMovieInputPhase::Up);
            if (movie_action != WWMovieInputAction::Pass) {
                if (movie_action == WWMovieInputAction::Skip) {
                    Put_Key_Message(VK_ESCAPE);
                }
                break;
            }
#endif
            int x, y;
            int button_index = 0;

            switch (event.button.button) {
            case SDL_BUTTON_LEFT:
            default:
                key = VK_LBUTTON;
                button_index = 0;
                break;
            case SDL_BUTTON_RIGHT:
                key = VK_RBUTTON;
                button_index = 1;
                break;
            case SDL_BUTTON_MIDDLE:
                key = VK_MBUTTON;
                button_index = 2;
                break;
            }

            bool inside_presentation = true;
            if (Is_Gamepad_Active() || Is_Video_Relative_Mouse_Active()) {
                Get_Video_Mouse(x, y);
            } else {
                inside_presentation = Is_Video_Window_Point_In_Presentation(event.button.x, event.button.y);
                Map_Video_Window_Point(event.button.x, event.button.y, x, y);
            }

            const bool released = event.type == SDL_MOUSEBUTTONUP;
            if (!Video_Presentation_Button_Event(
                    !released, inside_presentation, MouseButtonAccepted[button_index])) {
                break;
            }

            Put_Mouse_Message(key, x, y, released);
        } break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                if (Settings.Video.Windowed) {
                    Settings.Video.WindowWidth = event.window.data1;
                    Settings.Video.WindowHeight = event.window.data2;
                }
                break;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                Focus_Restore();
                break;
            case SDL_WINDOWEVENT_HIDDEN:
            case SDL_WINDOWEVENT_MINIMIZED:
            case SDL_WINDOWEVENT_FOCUS_LOST:
                Focus_Loss();
                break;
            case SDL_WINDOWEVENT_EXPOSED:
            case SDL_WINDOWEVENT_RESTORED:
                // Repaint and visibility notifications are not focus changes.
                // Refresh the display without restarting legacy audio while
                // the window is simply being uncovered.
                Focus_Refresh();
                break;
            }
            break;
        case SDL_MOUSEWHEEL:
            if (event.wheel.y > 0) { // scroll up
                Put_Key_Message(VK_MOUSEWHEEL_UP, false);
            } else if (event.wheel.y < 0) { // scroll down
                Put_Key_Message(VK_MOUSEWHEEL_DOWN, false);
            }
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (GameController != nullptr) {
                const SDL_GameController* removedController = SDL_GameControllerFromInstanceID(event.jdevice.which);
                if (removedController == GameController) {
                    SDL_GameControllerClose(GameController);
                    GameController = nullptr;
                }
            }
            break;
        case SDL_CONTROLLERDEVICEADDED:
            if (GameController == nullptr) {
                GameController = SDL_GameControllerOpen(event.jdevice.which);
            }
            break;
        case SDL_CONTROLLERAXISMOTION:
            Handle_Controller_Axis_Event(event.caxis);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            Handle_Controller_Button_Event(event.cbutton);
            break;
#ifdef IOS_BUILD
        case SDL_FINGERDOWN:
        case SDL_FINGERMOTION:
        case SDL_FINGERUP:
            Handle_Touch_Event(event.tfinger, event.type);
            break;
        case SDL_APP_WILLENTERBACKGROUND:
        case SDL_APP_DIDENTERBACKGROUND:
            Handle_Touch_Actions(Touch.Cancel_All());
            Focus_Loss();
            break;
        case SDL_APP_WILLENTERFOREGROUND:
        case SDL_APP_DIDENTERFOREGROUND:
            Focus_Restore();
            break;
#endif
#if defined(IOS_BUILD) || defined(RATOUCH_MACOS_BUILD)
        case SDL_USEREVENT:
            if (event.user.code == RATOUCH_APP_SETTING_CHANGED) {
                Ratouch_Apple_Apply_App_Setting(static_cast<int>(reinterpret_cast<intptr_t>(event.user.data1)),
                                                static_cast<int>(reinterpret_cast<intptr_t>(event.user.data2)));
                break;
            }
#ifdef IOS_BUILD
            if (event.user.code == RATOUCH_IOS_TOUCH_PREFERENCES_CHANGED) {
                Refresh_Touch_Preferences();
            }
#endif
            break;
#endif
        }
    }
#ifdef IOS_BUILD
    MouseMovieInput.End_Poll();
    TouchMovieInput.End_Poll();
#endif
    if (Is_Gamepad_Active()) {
        Process_Controller_Axis_Motion();
    }
}

bool WWKeyboardClassSDL2::Is_Gamepad_Active()
{
    return GameController != nullptr;
}

void WWKeyboardClassSDL2::Open_Controller()
{
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            GameController = SDL_GameControllerOpen(i);
        }
    }
}

void WWKeyboardClassSDL2::Close_Controller()
{
    if (SDL_GameControllerGetAttached(GameController)) {
        SDL_GameControllerClose(GameController);
        GameController = nullptr;
    }
}

void WWKeyboardClassSDL2::Process_Controller_Axis_Motion()
{
    const uint32_t currentTime = SDL_GetTicks();
    const float deltaTime = currentTime - LastControllerTime;
    LastControllerTime = currentTime;

    if (ControllerLeftXAxis != 0 || ControllerLeftYAxis != 0) {
        const int16_t xSign = (ControllerLeftXAxis > 0) - (ControllerLeftXAxis < 0);
        const int16_t ySign = (ControllerLeftYAxis > 0) - (ControllerLeftYAxis < 0);

        float movX = std::pow(std::abs(ControllerLeftXAxis), CONTROLLER_AXIS_SPEEDUP) * xSign * deltaTime
                     * Settings.Mouse.ControllerPointerSpeed / CONTROLLER_SPEED_MOD * ControllerSpeedBoost;
        float movY = std::pow(std::abs(ControllerLeftYAxis), CONTROLLER_AXIS_SPEEDUP) * ySign * deltaTime
                     * Settings.Mouse.ControllerPointerSpeed / CONTROLLER_SPEED_MOD * ControllerSpeedBoost;

        Move_Video_Mouse(movX, movY);
    }
}

void WWKeyboardClassSDL2::Handle_Controller_Axis_Event(const SDL_ControllerAxisEvent& motion)
{
    AnalogScrollActive = false;
    ScrollDirType directionX = SDIR_NONE;
    ScrollDirType directionY = SDIR_NONE;

    if (motion.axis == SDL_CONTROLLER_AXIS_LEFTX) {
        if (std::abs(motion.value) > CONTROLLER_L_DEADZONE)
            ControllerLeftXAxis = motion.value;
        else
            ControllerLeftXAxis = 0;
    } else if (motion.axis == SDL_CONTROLLER_AXIS_LEFTY) {
        if (std::abs(motion.value) > CONTROLLER_L_DEADZONE)
            ControllerLeftYAxis = motion.value;
        else
            ControllerLeftYAxis = 0;
    } else if (motion.axis == SDL_CONTROLLER_AXIS_RIGHTX) {
        if (std::abs(motion.value) > CONTROLLER_R_DEADZONE)
            ControllerRightXAxis = motion.value;
        else
            ControllerRightXAxis = 0;
    } else if (motion.axis == SDL_CONTROLLER_AXIS_RIGHTY) {
        if (std::abs(motion.value) > CONTROLLER_R_DEADZONE)
            ControllerRightYAxis = motion.value;
        else
            ControllerRightYAxis = 0;
    } else if (motion.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
        if (std::abs(motion.value) > CONTROLLER_TRIGGER_R_DEADZONE)
            ControllerSpeedBoost = 1 + (static_cast<float>(motion.value) / 32767) * CONTROLLER_TRIGGER_SPEEDUP;
        else
            ControllerSpeedBoost = 1;
    }

    if (ControllerRightXAxis != 0) {
        AnalogScrollActive = true;
        directionX = ControllerRightXAxis > 0 ? SDIR_E : SDIR_W;
    }
    if (ControllerRightYAxis != 0) {
        AnalogScrollActive = true;
        directionY = ControllerRightYAxis > 0 ? SDIR_S : SDIR_N;
    }

    if (directionX == SDIR_E && directionY == SDIR_N) {
        ScrollDirection = SDIR_NE;
    } else if (directionX == SDIR_E && directionY == SDIR_S) {
        ScrollDirection = SDIR_SE;
    } else if (directionX == SDIR_W && directionY == SDIR_N) {
        ScrollDirection = SDIR_NW;
    } else if (directionX == SDIR_W && directionY == SDIR_S) {
        ScrollDirection = SDIR_SW;
    } else if (directionX == SDIR_E) {
        ScrollDirection = SDIR_E;
    } else if (directionX == SDIR_W) {
        ScrollDirection = SDIR_W;
    } else if (directionY == SDIR_S) {
        ScrollDirection = SDIR_S;
    } else if (directionY == SDIR_N) {
        ScrollDirection = SDIR_N;
    }
}

void WWKeyboardClassSDL2::Handle_Controller_Button_Event(const SDL_ControllerButtonEvent& button)
{
    bool keyboardPress = false;
    bool mousePress = false;
    unsigned short key;
    SDL_Scancode scancode;

    switch (button.button) {
    case SDL_CONTROLLER_BUTTON_A:
        mousePress = true;
        key = VK_LBUTTON;
        break;
    case SDL_CONTROLLER_BUTTON_B:
        mousePress = true;
        key = VK_RBUTTON;
        break;
    case SDL_CONTROLLER_BUTTON_X:
        keyboardPress = true;
        scancode = SDL_SCANCODE_G;
        break;
    case SDL_CONTROLLER_BUTTON_Y:
        keyboardPress = true;
        scancode = SDL_SCANCODE_F;
        break;
    case SDL_CONTROLLER_BUTTON_BACK:
        keyboardPress = true;
        scancode = SDL_SCANCODE_ESCAPE;
        break;
    case SDL_CONTROLLER_BUTTON_START:
        keyboardPress = true;
        scancode = SDL_SCANCODE_RETURN;
        break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
        keyboardPress = true;
        scancode = SDL_SCANCODE_LCTRL;
        break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
        keyboardPress = true;
        scancode = SDL_SCANCODE_LALT;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP:
        keyboardPress = true;
        scancode = SDL_SCANCODE_1;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
        keyboardPress = true;
        scancode = SDL_SCANCODE_2;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
        keyboardPress = true;
        scancode = SDL_SCANCODE_3;
        break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        keyboardPress = true;
        scancode = SDL_SCANCODE_4;
        break;
    default:
        break;
    }

    if (keyboardPress) {
        Put_Key_Message(scancode, button.state == SDL_RELEASED);
    } else if (mousePress) {
        int x, y;
        Get_Video_Mouse(x, y);
        Put_Mouse_Message(key, x, y, button.state == SDL_RELEASED);
    }
}

bool WWKeyboardClassSDL2::Is_Analog_Scroll_Active()
{
#ifdef IOS_BUILD
    float dx = 0.0f;
    float dy = 0.0f;
    return AnalogScrollActive || TouchScroll.Peek(dx, dy);
#else
    return AnalogScrollActive;
#endif
}

unsigned char WWKeyboardClassSDL2::Get_Scroll_Direction()
{
#ifdef IOS_BUILD
    float dx = 0.0f;
    float dy = 0.0f;
    return TouchScroll.Peek(dx, dy) ? Touch_Scroll_Direction(dx, dy) : ScrollDirection;
#else
    return ScrollDirection;
#endif
}

bool WWKeyboardClassSDL2::Consume_Analog_Scroll(unsigned char& direction, int& pixel_distance)
{
#ifdef IOS_BUILD
    float dx = 0.0f;
    float dy = 0.0f;
    if (TouchScroll.Consume(dx, dy)) {
        direction = Touch_Scroll_Direction(dx, dy);
        pixel_distance = std::max(1, static_cast<int>(std::lround(std::sqrt(dx * dx + dy * dy))));
        return direction != SDIR_NONE;
    }
#endif
    if (AnalogScrollActive) {
        direction = ScrollDirection;
        pixel_distance = 0;
        return true;
    }
    return false;
}

bool WWKeyboardClassSDL2::Is_Mouse_Edge_Scroll_Allowed()
{
#ifdef IOS_BUILD
    return !PointerOwner.Touch_Owns_Cursor();
#else
    return true;
#endif
}

#ifdef IOS_BUILD
ScrollDirType WWKeyboardClassSDL2::Touch_Scroll_Direction(float dx, float dy) const
{
    const float absolute_x = std::fabs(dx);
    const float absolute_y = std::fabs(dy);
    const bool horizontal = absolute_x > 0.01f && absolute_x >= absolute_y * 0.4142f;
    const bool vertical = absolute_y > 0.01f && absolute_y >= absolute_x * 0.4142f;
    const bool east = horizontal && dx < 0.0f;
    const bool west = horizontal && dx > 0.0f;
    const bool south = vertical && dy < 0.0f;
    const bool north = vertical && dy > 0.0f;

    if (east && north) {
        return SDIR_NE;
    }
    if (east && south) {
        return SDIR_SE;
    }
    if (west && north) {
        return SDIR_NW;
    }
    if (west && south) {
        return SDIR_SW;
    }
    if (east) {
        return SDIR_E;
    }
    if (west) {
        return SDIR_W;
    }
    if (north) {
        return SDIR_N;
    }
    if (south) {
        return SDIR_S;
    }
    return SDIR_NONE;
}

void WWKeyboardClassSDL2::Refresh_Touch_Preferences()
{
    WWTouchState::Config config;
    config.LongPressMilliseconds = Ratouch_iOS_Long_Press_Milliseconds();
    config.DragThreshold = Ratouch_iOS_Drag_Threshold();
    Touch.Set_Config(config);
    TouchPanInverted = Ratouch_iOS_Invert_Pan();
}

void WWKeyboardClassSDL2::Handle_Touch_Actions(const std::vector<WWTouchAction>& actions)
{
    for (const WWTouchAction& action : actions) {
        int x = 0;
        int y = 0;
        const bool inside_presentation = Is_Video_Window_Point_In_Presentation(action.X, action.Y);
        Map_Video_Window_Point(action.X, action.Y, x, y);
        switch (action.Type) {
        case WWTouchActionType::CursorMove:
            Set_Video_Mouse(x, y);
            break;
        case WWTouchActionType::LeftDown:
            if (!Video_Presentation_Button_Event(true, inside_presentation, TouchLeftAccepted)) {
                break;
            }
            Set_Video_Mouse(x, y);
            Put_Mouse_Message(VK_LBUTTON, x, y, false);
            break;
        case WWTouchActionType::LeftUp:
            if (!Video_Presentation_Button_Event(false, inside_presentation, TouchLeftAccepted)) {
                break;
            }
            Set_Video_Mouse(x, y);
            Put_Mouse_Message(VK_LBUTTON, x, y, true);
            break;
        case WWTouchActionType::RightDown:
            if (!Video_Presentation_Button_Event(true, inside_presentation, TouchRightAccepted)) {
                break;
            }
            Ratouch_iOS_Confirm_Long_Press();
            Set_Video_Mouse(x, y);
            Put_Mouse_Message(VK_RBUTTON, x, y, false);
            break;
        case WWTouchActionType::RightUp:
            if (!Video_Presentation_Button_Event(false, inside_presentation, TouchRightAccepted)) {
                break;
            }
            Set_Video_Mouse(x, y);
            Put_Mouse_Message(VK_RBUTTON, x, y, true);
            break;
        case WWTouchActionType::PanMove: {
            float delta_x = 0.0f;
            float delta_y = 0.0f;
            Map_Video_Window_Delta(action.DeltaX, action.DeltaY, delta_x, delta_y);
            delta_x *= TouchPanInverted ? -1.0f : 1.0f;
            delta_y *= TouchPanInverted ? -1.0f : 1.0f;
            TouchScroll.Add(delta_x, delta_y);
        } break;
        case WWTouchActionType::PanStart:
            TouchScroll.Clear();
            break;
        case WWTouchActionType::PanEnd:
            break;
        case WWTouchActionType::ZoomStep:
            Adjust_Video_Zoom(action.Steps);
            break;
        case WWTouchActionType::ZoomReset:
            Reset_Video_Zoom();
            break;
        }
    }
}

void WWKeyboardClassSDL2::Handle_Touch_Event(const SDL_TouchFingerEvent& touch, uint32_t type)
{
    // Direct touch owns the virtual cursor until a real mouse or trackpad event
    // arrives. This keeps Red Alert's legacy mouse-edge scrolling from fighting
    // one-finger selection while preserving edge scroll for pointer users.
    int width = 1;
    int height = 1;
    SDL_Window* window = SDL_GetKeyboardFocus();
    if (window == nullptr) {
        window = SDL_GetMouseFocus();
    }
    if (window != nullptr) {
        SDL_GetWindowSize(window, &width, &height);
    }
    const float x = touch.x * width;
    const float y = touch.y * height;
    PointerOwner.Observe_Touch();

    WWMovieInputPhase movie_phase = WWMovieInputPhase::Motion;
    if (type == SDL_FINGERDOWN) {
        movie_phase = WWMovieInputPhase::Down;
    } else if (type == SDL_FINGERUP) {
        movie_phase = WWMovieInputPhase::Up;
    }
    const WWMovieInputAction movie_action = TouchMovieInput.Handle(InMovie, movie_phase);
    if (movie_action != WWMovieInputAction::Pass) {
        Touch.Cancel_All();
        if (movie_action == WWMovieInputAction::Skip) {
            Put_Key_Message(VK_ESCAPE);
        }
        return;
    }

    std::vector<WWTouchAction> actions;
    if (type == SDL_FINGERDOWN) {
        actions = Touch.Finger_Down(touch.fingerId, x, y, touch.timestamp);
    } else if (type == SDL_FINGERMOTION) {
        actions = Touch.Finger_Motion(touch.fingerId, x, y, touch.timestamp);
    } else {
        actions = Touch.Finger_Up(touch.fingerId, x, y, touch.timestamp);
    }
    Handle_Touch_Actions(actions);
}
#endif

KeyASCIIType WWKeyboardClassSDL2::To_ASCII(unsigned short key)
{
    if (key & WWKEY_RLS_BIT) {
        return KA_NONE;
    }

    key &= 0xFF; // drop all mods

    if (key > ARRAY_SIZE(sdl_keymap) / 2 - 1) {
        return KA_NONE;
    }

    if (SDL_GetModState() & KMOD_SHIFT) {
        return sdl_keymap[key + ARRAY_SIZE(sdl_keymap) / 2];
    } else {
        return sdl_keymap[key];
    }
}

WWKeyboardClass* CreateWWKeyboardClass(void)
{
    return new WWKeyboardClassSDL2;
}
