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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood Win32 Library                   *
 *                                                                         *
 *                    File Name : DDRAW.CPP                                *
 *                                                                         *
 *                   Programmer : Philip W. Gorrow                         *
 *                                                                         *
 *                   Start Date : October 10, 1995                         *
 *                                                                         *
 *                  Last Update : October 10, 1995   []                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

#include "gbuffer.h"
#include "palette.h"
#include "video.h"
#include "video_geometry.h"
#ifdef IOS_BUILD
#include "ios_lifecycle.h"
#endif
#include "wwkeyboard.h"
#include "wwmouse.h"
#include "settings.h"
#ifdef IOS_BUILD
#include "ios_controls.h"
#endif
#include "debugstring.h"

#include <SDL.h>

extern WWKeyboardClass* Keyboard;
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Palette* palette;
static Uint32 pixel_format;
static SDL_Rect render_dst;
static int render_output_w;
static int render_output_h;
static int presentation_zoom = 0;

static struct
{
    int GameW;
    int GameH;
    bool Clip;
    float ScaleX{1.0f};
    float ScaleY{1.0f};
    void* Raw;
    int W;
    int H;
    int HotX;
    int HotY;
    float X;
    float Y;
    SDL_Cursor* Pending;
    SDL_Cursor* Current;
    SDL_Surface* Surface;
} hwcursor;

#define ARRAY_SIZE(x) int(sizeof(x) / sizeof(x[0]))
#define MAKEFORMAT(f)                                                                                                  \
    {                                                                                                                  \
        SDL_PIXELFORMAT_##f, #f                                                                                        \
    }
Uint32 SettingsPixelFormat()
{
    /*
    ** Known good RGB formats for both the surface and texture.
    */
    static struct
    {
        Uint32 format;
        std::string name;
    } formats[] = {
        MAKEFORMAT(ARGB8888),
        MAKEFORMAT(RGBA8888),
        MAKEFORMAT(ABGR8888),
        MAKEFORMAT(BGRA8888),
        MAKEFORMAT(RGB24),
        MAKEFORMAT(BGR24),
        MAKEFORMAT(RGB888),
        MAKEFORMAT(BGR888),
        MAKEFORMAT(RGB555),
        MAKEFORMAT(BGR555),
        MAKEFORMAT(RGB565),
        MAKEFORMAT(BGR565),
    };

    std::string str = Settings.Video.PixelFormat;

    for (auto& c : str) {
        c = toupper(c);
    }

    for (int i = 0; i < ARRAY_SIZE(formats); i++) {
        if (str.compare(formats[i].name) == 0) {
            return formats[i].format;
        }
    }

    return SDL_PIXELFORMAT_UNKNOWN;
}

static void Update_HWCursor();

static VideoPresentationGeometry Current_Presentation_Geometry()
{
    int window_w = 0;
    int window_h = 0;
    int output_w = 0;
    int output_h = 0;
    if (window != nullptr) {
        SDL_GetWindowSize(window, &window_w, &window_h);
    }
    if (renderer != nullptr) {
        SDL_GetRendererOutputSize(renderer, &output_w, &output_h);
    }
    return {
        window_w,
        window_h,
        output_w,
        output_h,
        hwcursor.GameW,
        hwcursor.GameH,
        render_dst.x,
        render_dst.y,
        render_dst.w,
        render_dst.h,
    };
}

static void Update_HWCursor_Settings()
{
    /*
    ** Update mouse scaling settings.
    */
    int win_w, win_h;
    SDL_GetRendererOutputSize(renderer, &win_w, &win_h);
    render_output_w = win_w;
    render_output_h = win_h;
    /*
    ** Update screen boxing settings.
    */
    float ar = (float)hwcursor.GameW / hwcursor.GameH;
    if (Settings.Video.Boxing) {
        size_t colonPos = Settings.Video.BoxingAspectRatio.find(":");
        std::string arW;
        std::string arH;

        /*
        ** If we don't have a valid string for aspect ratio, default back to 4:3.
        */
        if (colonPos == std::string::npos) {
            arW = "4";
            arH = "3";
        } else {
            size_t arLen = Settings.Video.BoxingAspectRatio.length();
            arW = Settings.Video.BoxingAspectRatio.substr(0, colonPos);
            arH = Settings.Video.BoxingAspectRatio.substr(colonPos + 1, arLen - colonPos);
        }

        ar = std::stof(arW) / std::stof(arH);

        render_dst.w = win_w;
        render_dst.h = render_dst.w / ar;
        if (render_dst.h > win_h) {
            render_dst.h = win_h;
            render_dst.w = render_dst.h * ar;
        }
        render_dst.x = (win_w - render_dst.w) / 2;
        render_dst.y = (win_h - render_dst.h) / 2;
    } else {
        render_dst.w = win_w;
        render_dst.h = win_h;
        render_dst.x = 0;
        render_dst.y = 0;
    }

#ifdef IOS_BUILD
    static const float zoom_levels[] = {1.0f, 1.5f, 2.0f};
    const float zoom = zoom_levels[presentation_zoom];
    const int fitted_w = render_dst.w;
    const int fitted_h = render_dst.h;
    render_dst.w = static_cast<int>(fitted_w * zoom);
    render_dst.h = static_cast<int>(fitted_h * zoom);
    render_dst.x = (win_w - render_dst.w) / 2;
    render_dst.y = (win_h - render_dst.h) / 2;
#endif

    hwcursor.ScaleX = render_dst.w / (float)hwcursor.GameW;
    hwcursor.ScaleY = render_dst.h / (float)hwcursor.GameH;

    /*
    ** Ensure cursor clip is in the desired state.
    */
    Set_Video_Cursor_Clip(hwcursor.Clip);

    /*
    ** Update visible cursor scaling.
    */
    Update_HWCursor();
}

class SurfaceMonitorClassDummy : public SurfaceMonitorClass
{

public:
    SurfaceMonitorClassDummy()
    {
    }

    virtual void Restore_Surfaces()
    {
    }

    virtual void Set_Surface_Focus(bool in_focus)
    {
    }

    virtual void Release()
    {
    }
};

SurfaceMonitorClassDummy AllSurfacesDummy;           // List of all direct draw surfaces
SurfaceMonitorClass& AllSurfaces = AllSurfacesDummy; // List of all direct draw surfaces

/***********************************************************************************************
 * Set_Video_Mode -- Initializes Direct Draw and sets the required Video Mode                  *
 *                                                                                             *
 * INPUT:           int width           - the width of the video mode in pixels                *
 *                  int height          - the height of the video mode in pixels               *
 *                  int bits_per_pixel  - the number of bits per pixel the video mode supports *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
bool Set_Video_Mode(int w, int h, int bits_per_pixel)
{
#ifdef IOS_BUILD
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
#endif
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
#ifdef IOS_BUILD
    Ratouch_Install_iOS_Lifecycle_Filter();
#endif
    SDL_ShowCursor(SDL_DISABLE);

    int win_w = w;
    int win_h = h;
    int win_flags = SDL_WINDOW_ALLOW_HIGHDPI;
#if defined(__APPLE__) && !defined(IOS_BUILD)
    win_flags |= SDL_WINDOW_RESIZABLE;
#endif
#ifdef IOS_BUILD
    win_flags |= SDL_WINDOW_FULLSCREEN;
#endif
    Uint32 requested_pixel_format = SettingsPixelFormat();

    if (!Settings.Video.Windowed) {
        /*
        ** Native fullscreen if no proper width and height set.
        */
        if (Settings.Video.Width < w || Settings.Video.Height < h) {
            win_w = Settings.Video.Width = 0;
            win_h = Settings.Video.Height = 0;
            win_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        } else {
            win_w = Settings.Video.Width;
            win_h = Settings.Video.Height;
            win_flags |= SDL_WINDOW_FULLSCREEN;
        }
    } else if (Settings.Video.WindowWidth > w || Settings.Video.WindowHeight > h) {
        win_w = Settings.Video.WindowWidth;
        win_h = Settings.Video.WindowHeight;
    } else {
        Settings.Video.WindowWidth = win_w;
        Settings.Video.WindowHeight = win_h;
    }

    window = SDL_CreateWindow(
#ifdef __APPLE__
        "RAtouch",
#else
        "Vanilla Conquer",
#endif
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        win_w,
        win_h,
        win_flags);
    if (window == nullptr) {
        DBG_ERROR("SDL_CreateWindow failed: %s", SDL_GetError());
        Reset_Video_Mode();
        return false;
    }
#ifdef IOS_BUILD
    Ratouch_Install_Command_Overlay();
#endif

    DBG_INFO("Created SDL2 %s window in %dx%d", (win_flags ? "fullscreen" : "windowed"), win_w, win_h);

    pixel_format = SDL_GetWindowPixelFormat(window);
    if (pixel_format == SDL_PIXELFORMAT_UNKNOWN || SDL_BITSPERPIXEL(pixel_format) < 16) {
        DBG_ERROR("SDL2 window pixel format unsupported: %s (%d bpp)",
                  SDL_GetPixelFormatName(pixel_format),
                  SDL_BITSPERPIXEL(pixel_format));
        Reset_Video_Mode();
        return false;
    }

    DBG_INFO("  pixel format: %s (%d bpp)", SDL_GetPixelFormatName(pixel_format), SDL_BITSPERPIXEL(pixel_format));

    DBG_INFO("SDL2 drivers available: (user preference '%s')", Settings.Video.Driver.c_str());
    int renderer_index = -1;
    for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
        SDL_RendererInfo info;
        if (SDL_GetRenderDriverInfo(i, &info) == 0) {
            if (Settings.Video.Driver.compare(info.name) == 0) {
                renderer_index = i;
            }

            DBG_INFO(" %s%s", info.name, (i == renderer_index ? " (selected)" : ""));
        }
    }

    renderer = SDL_CreateRenderer(window, renderer_index, SDL_RENDERER_TARGETTEXTURE);
    if (renderer == nullptr) {
        DBG_ERROR("SDL_CreateRenderer failed: %s", SDL_GetError());
        Reset_Video_Mode();
        return false;
    }

    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer, &info) != 0) {
        DBG_ERROR("SDL_GetRendererInfo failed: %s", SDL_GetError());
        Reset_Video_Mode();
        return false;
    }

    DBG_INFO("Initialized SDL2 driver '%s'", info.name);
    DBG_INFO("  flags:");
    if (info.flags & SDL_RENDERER_SOFTWARE) {
        DBG_INFO("    SDL_RENDERER_SOFTWARE");
    }
    if (info.flags & SDL_RENDERER_ACCELERATED) {
        DBG_INFO("    SDL_RENDERER_ACCELERATED");
    }
    if (info.flags & SDL_RENDERER_PRESENTVSYNC) {
        DBG_INFO("    SDL_RENDERER_PRESENT_VSYNC");
    }
    if (info.flags & SDL_RENDERER_TARGETTEXTURE) {
        DBG_INFO("    SDL_RENDERER_TARGETTEXTURE");
    }

    DBG_INFO("  max texture size: %dx%d", info.max_texture_width, info.max_texture_height);

    DBG_INFO("  %d texture formats supported: (user preference '%s')",
             info.num_texture_formats,
             SDL_GetPixelFormatName(requested_pixel_format));

    /*
    ** Pick the first pixel format or the user requested one. It better be RGB.
    */
    pixel_format = SDL_PIXELFORMAT_UNKNOWN;
    for (int i = 0; i < info.num_texture_formats; i++) {
        if ((pixel_format == SDL_PIXELFORMAT_UNKNOWN && i == 0) || info.texture_formats[i] == requested_pixel_format) {
            pixel_format = info.texture_formats[i];
        }
    }

    for (int i = 0; i < info.num_texture_formats; i++) {
        DBG_INFO("    %s%s",
                 SDL_GetPixelFormatName(info.texture_formats[i]),
                 (pixel_format == info.texture_formats[i] ? " (selected)" : ""));
    }

    /*
    ** Set requested scaling algorithm.
    */
    if (!SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, Settings.Video.Scaler.c_str(), SDL_HINT_OVERRIDE)) {
        DBG_WARN("  scaler '%s' is unsupported");
    } else {
        DBG_INFO("  scaler set to '%s'", Settings.Video.Scaler.c_str());
    }

    if (palette == nullptr) {
        palette = SDL_AllocPalette(256);
    }

    /*
    ** Set mouse scaling options.
    */
    hwcursor.GameW = w;
    hwcursor.GameH = h;
    hwcursor.X = w / 2;
    hwcursor.Y = h / 2;
    Update_HWCursor_Settings();

    /*
    ** Init gamepad.
    */
    if (Settings.Mouse.ControllerEnabled) {
        SDL_Init(SDL_INIT_GAMECONTROLLER);
        Keyboard->Open_Controller();
    }

    return true;
}

void Toggle_Video_Fullscreen()
{
    if (window == nullptr) {
        return;
    }
    Settings.Video.Windowed = !Settings.Video.Windowed;

    if (!Settings.Video.Windowed) {
        if (Settings.Video.Width == 0 || Settings.Video.Height == 0) {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        } else {
            SDL_SetWindowSize(window, Settings.Video.Width, Settings.Video.Height);
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        }
    } else {
        SDL_SetWindowFullscreen(window, 0);
        SDL_SetWindowSize(window, Settings.Video.WindowWidth, Settings.Video.WindowHeight);
    }

    Update_HWCursor_Settings();
}

void Get_Video_Scale(float& x, float& y)
{
    x = hwcursor.ScaleX;
    y = hwcursor.ScaleY;
}

void Set_Video_Cursor_Clip(bool clipped)
{
    hwcursor.Clip = clipped;

    if (window) {
        const SDL_bool was_relative = SDL_GetRelativeMouseMode();
        SDL_bool wants_relative = SDL_FALSE;

#ifdef IOS_BUILD
        // iPad trackpads expose an absolute system pointer. Relative mode
        // decouples the software game cursor from that visible pointer.
        SDL_SetWindowGrab(window, SDL_FALSE);
#else
        if (Settings.Video.Windowed) {
            SDL_SetWindowGrab(window, SDL_FALSE);
        } else {
            SDL_SetWindowGrab(window, SDL_TRUE);
            wants_relative = Video_Use_Relative_Mouse(Settings.Mouse.RawInput, Settings.Video.Windowed)
                ? SDL_TRUE
                : SDL_FALSE;
        }
#endif

        if (SDL_SetRelativeMouseMode(wants_relative) < 0) {
            DBG_ERROR("Raw input not supported, disabling.");
            Settings.Mouse.RawInput = false;
            return;
        }

#ifndef IOS_BUILD
        if (Settings.Mouse.RawInput && was_relative != wants_relative) {
            if (wants_relative == SDL_TRUE) {
                int window_x = 0;
                int window_y = 0;
                int game_x = 0;
                int game_y = 0;
                SDL_GetMouseState(&window_x, &window_y);
                Map_Video_Window_Point(window_x, window_y, game_x, game_y);
                hwcursor.X = game_x;
                hwcursor.Y = game_y;
            } else {
                float window_x = 0.0f;
                float window_y = 0.0f;
                Video_Game_Point_To_Window(
                    Current_Presentation_Geometry(), hwcursor.X, hwcursor.Y, window_x, window_y);
                SDL_WarpMouseInWindow(window, static_cast<int>(window_x), static_cast<int>(window_y));
            }
        }
#else
        (void)was_relative;
#endif
    }
}

void Move_Video_Mouse(float xrel, float yrel)
{
    if (Keyboard->Is_Gamepad_Active() || hwcursor.Clip || !Settings.Video.Windowed) {
        hwcursor.X += xrel * (Settings.Mouse.Sensitivity / 100.0f);
        hwcursor.Y += yrel * (Settings.Mouse.Sensitivity / 100.0f);
    }

    if (hwcursor.X >= hwcursor.GameW) {
        hwcursor.X = hwcursor.GameW - 1;
    } else if (hwcursor.X < 0) {
        hwcursor.X = 0;
    }

    if (hwcursor.Y >= hwcursor.GameH) {
        hwcursor.Y = hwcursor.GameH - 1;
    } else if (hwcursor.Y < 0) {
        hwcursor.Y = 0;
    }
}

void Get_Video_Mouse(int& x, int& y)
{
#ifdef IOS_BUILD
    // Both direct touch and absolute pointer events update the cached game
    // position. SDL's mouse state does not move for direct touches.
    x = hwcursor.X;
    y = hwcursor.Y;
#else
    if (Keyboard->Is_Gamepad_Active() || Is_Video_Relative_Mouse_Active()) {
        x = hwcursor.X;
        y = hwcursor.Y;
    } else {
        int window_x = 0;
        int window_y = 0;
        SDL_GetMouseState(&window_x, &window_y);
        Map_Video_Window_Point(window_x, window_y, x, y);
    }
#endif
}

bool Is_Video_Relative_Mouse_Active()
{
#ifdef IOS_BUILD
    return false;
#else
    return Video_Use_Relative_Mouse(Settings.Mouse.RawInput, Settings.Video.Windowed);
#endif
}

void Set_Video_Mouse(int x, int y)
{
    hwcursor.X = x;
    hwcursor.Y = y;
    if (hwcursor.X >= hwcursor.GameW) {
        hwcursor.X = hwcursor.GameW - 1;
    } else if (hwcursor.X < 0) {
        hwcursor.X = 0;
    }
    if (hwcursor.Y >= hwcursor.GameH) {
        hwcursor.Y = hwcursor.GameH - 1;
    } else if (hwcursor.Y < 0) {
        hwcursor.Y = 0;
    }
}

bool Is_Video_Window_Point_In_Presentation(float window_x, float window_y)
{
    return Video_Window_Point_In_Presentation(Current_Presentation_Geometry(), window_x, window_y);
}

void Map_Video_Window_Point(float window_x, float window_y, int& x, int& y)
{
    float game_x = 0.0f;
    float game_y = 0.0f;
    Video_Window_Point_To_Game(Current_Presentation_Geometry(), window_x, window_y, game_x, game_y);
    x = static_cast<int>(game_x);
    y = static_cast<int>(game_y);
}

void Map_Video_Window_Delta(float window_dx, float window_dy, float& x, float& y)
{
    Video_Window_Delta_To_Game(Current_Presentation_Geometry(), window_dx, window_dy, x, y);
}

void Map_Video_Point(float normalized_x, float normalized_y, int& x, int& y)
{
    int window_w = 0;
    int window_h = 0;
    SDL_GetWindowSize(window, &window_w, &window_h);
    Map_Video_Window_Point(normalized_x * window_w, normalized_y * window_h, x, y);
}

void Adjust_Video_Zoom(int steps)
{
#ifdef IOS_BUILD
    presentation_zoom += steps;
    if (presentation_zoom < 0) {
        presentation_zoom = 0;
    } else if (presentation_zoom > 2) {
        presentation_zoom = 2;
    }
    Update_HWCursor_Settings();
#else
    (void)steps;
#endif
}

void Reset_Video_Zoom()
{
#ifdef IOS_BUILD
    presentation_zoom = 0;
    Update_HWCursor_Settings();
#endif
}

/***********************************************************************************************
 * Reset_Video_Mode -- Resets video mode and deletes Direct Draw Object                        *
 *                                                                                             *
 * INPUT:		none                                                                            *
 *                                                                                             *
 * OUTPUT:     none                                                                            *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/26/1995 PWG : Created.                                                                 *
 *=============================================================================================*/
void Reset_Video_Mode(void)
{
    if (hwcursor.Pending) {
        SDL_FreeCursor(hwcursor.Pending);
        hwcursor.Pending = nullptr;
    }

    if (hwcursor.Current) {
        SDL_FreeCursor(hwcursor.Current);
        hwcursor.Current = nullptr;
    }

    if (hwcursor.Surface) {
        SDL_FreeSurface(hwcursor.Surface);
        hwcursor.Surface = nullptr;
    }

    SDL_DestroyRenderer(renderer);
    renderer = nullptr;

    SDL_FreePalette(palette);
    palette = nullptr;

    SDL_DestroyWindow(window);
    window = nullptr;

    Keyboard->Close_Controller();
}

static void Update_HWCursor()
{
    float scale_x = 1.0f;
    float scale_y = 1.0f;
    int scaled_w = hwcursor.W;
    int scaled_h = hwcursor.H;

    /*
    ** Pre-scale cursor *only* if we are not emulating a hw cursor.
    */
    if (Settings.Video.HardwareCursor) {
        scale_x = hwcursor.ScaleX;
        scale_y = hwcursor.ScaleY;
        scaled_w *= scale_x;
        scaled_h *= scale_y;
    }

    /*
    ** Allocate or reallocate surface if it has the wrong size.
    */
    if (hwcursor.Surface == nullptr || hwcursor.Surface->w != scaled_w || hwcursor.Surface->h != scaled_h) {
        if (hwcursor.Surface) {
            SDL_FreeSurface(hwcursor.Surface);
        }

        /*
        ** Real HW cursor needs to be scaled up. Emulated can use original cursor data.
        */
        if (Settings.Video.HardwareCursor) {
            hwcursor.Surface = SDL_CreateRGBSurfaceWithFormat(0, scaled_w, scaled_h, 8, SDL_PIXELFORMAT_INDEX8);
        } else {
            hwcursor.Surface =
                SDL_CreateRGBSurfaceFrom(hwcursor.Raw, hwcursor.W, hwcursor.H, 8, hwcursor.W, 0, 0, 0, 0);
        }

        SDL_SetSurfacePalette(hwcursor.Surface, palette);
        SDL_SetColorKey(hwcursor.Surface, SDL_TRUE, 0);
    }

    /*
    ** Prepare HW cursor by scaling up and creating the SDL version.
    */
    if (Settings.Video.HardwareCursor) {
        uint8_t* src = (uint8_t*)hwcursor.Raw;
        uint8_t* dst = (uint8_t*)hwcursor.Surface->pixels;
        int src_pitch = hwcursor.W;
        int dst_pitch = hwcursor.Surface->pitch;

        for (int y = 0; y < scaled_h; y++) {
            for (int x = 0; x < scaled_w; x++) {
                dst[dst_pitch * y + x] = src[src_pitch * (int)(y / scale_y) + (int)(x / scale_x)];
            }
        }

        if (hwcursor.Pending) {
            SDL_FreeCursor(hwcursor.Pending);
        }

        /*
        ** Queue new cursor to be set during frame flip.
        */
        hwcursor.Pending =
            SDL_CreateColorCursor(hwcursor.Surface, hwcursor.HotX * hwcursor.ScaleX, hwcursor.HotY * hwcursor.ScaleY);
    }
}

void Set_Video_Cursor(void* cursor, int w, int h, int hotx, int hoty)
{
    hwcursor.Raw = cursor;
    hwcursor.W = w;
    hwcursor.H = h;
    hwcursor.HotX = hotx;
    hwcursor.HotY = hoty;

    Update_HWCursor();
}

/***********************************************************************************************
 * Get_Free_Video_Memory -- returns amount of free video memory                                *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   bytes of available video RAM                                                      *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/29/95 12:52PM ST : Created                                                            *
 *=============================================================================================*/
unsigned int Get_Free_Video_Memory(void)
{
    return 1000000000;
}

/***********************************************************************************************
 * Get_Video_Hardware_Caps -- returns bitmask of direct draw video hardware support            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   hardware flags                                                                    *
 *                                                                                             *
 * WARNINGS: Must call Set_Video_Mode 1st to create the direct draw object                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    1/12/96 9:14AM ST : Created                                                              *
 *=============================================================================================*/
unsigned Get_Video_Hardware_Capabilities(void)
{
    return VIDEO_BLITTER;
}

/***********************************************************************************************
 * Wait_Vert_Blank -- Waits for the start (leading edge) of a vertical blank                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/
void Wait_Vert_Blank(void)
{
}

/***********************************************************************************************
 * Set_Palette -- set a direct draw palette                                                    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to 768 rgb palette bytes                                                      *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/11/95 3:33PM ST : Created                                                             *
 *=============================================================================================*/
void Set_DD_Palette(void* rpalette)
{
    SDL_Color colors[256];

    unsigned char* rcolors = (unsigned char*)rpalette;
    for (int i = 0; i < 256; i++) {
        colors[i].r = (unsigned char)rcolors[i * 3] << 2;
        colors[i].g = (unsigned char)rcolors[i * 3 + 1] << 2;
        colors[i].b = (unsigned char)rcolors[i * 3 + 2] << 2;
        colors[i].a = 0xFF;
    }

    /*
    ** First color is transparent. This needs to be set so that hardware cursor has transparent
    ** surroundings when converting from 8-bit to 32-bit.
    */
    colors[0].a = 0;

    SDL_SetPaletteColors(palette, colors, 0, 256);

    /*
    ** Cursor needs to be updated when palette changes.
    */
    Update_HWCursor();
}

/***********************************************************************************************
 * Wait_Blit -- waits for the DirectDraw blitter to become idle                                *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07-25-95 03:53pm ST : Created                                                             *
 *=============================================================================================*/

void Wait_Blit(void)
{
}

/***********************************************************************************************
 * SMC::SurfaceMonitorClass -- constructor for surface monitor class                           *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/3/95 3:23PM ST : Created                                                              *
 *=============================================================================================*/

SurfaceMonitorClass::SurfaceMonitorClass()
{
    SurfacesRestored = false;
}

/*
** VideoSurfaceDDraw
*/
class VideoSurfaceSDL2;
static VideoSurfaceSDL2* frontSurface = nullptr;

class VideoSurfaceSDL2 : public VideoSurface
{
public:
    VideoSurfaceSDL2(int w, int h, GBC_Enum flags)
        : flags(flags)
        , windowSurface(nullptr)
        , texture(nullptr)
    {
        surface = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
        SDL_SetSurfacePalette(surface, palette);

        if (flags & GBC_VISIBLE) {
            windowSurface = SDL_CreateRGBSurfaceWithFormat(0, w, h, SDL_BITSPERPIXEL(pixel_format), pixel_format);
            texture = SDL_CreateTexture(renderer, windowSurface->format->format, SDL_TEXTUREACCESS_STREAMING, w, h);
            SDL_SetTextureScaleMode(
                texture, Settings.Video.Scaler == "linear" ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
            frontSurface = this;
        }
    }

    virtual ~VideoSurfaceSDL2()
    {
        if (frontSurface == this) {
            frontSurface = nullptr;
        }

        SDL_FreeSurface(surface);

        if (texture) {
            SDL_DestroyTexture(texture);
        }

        if (windowSurface) {
            SDL_FreeSurface(windowSurface);
        }
    }

    virtual void* GetData() const
    {
        return surface->pixels;
    }
    virtual int GetPitch() const
    {
        return surface->pitch;
    }
    virtual bool IsAllocated() const
    {
        return false;
    }

    virtual void AddAttachedSurface(VideoSurface* surface)
    {
    }

    virtual bool IsReadyToBlit()
    {
        return true;
    }

    virtual bool LockWait()
    {
        return (SDL_LockSurface(surface) == 0);
    }

    virtual bool Unlock()
    {
        SDL_UnlockSurface(surface);
        return true;
    }

    virtual void Blt(const Rect& destRect, VideoSurface* src, const Rect& srcRect, bool mask)
    {
        SDL_BlitSurface(((VideoSurfaceSDL2*)src)->surface, (SDL_Rect*)(&srcRect), surface, (SDL_Rect*)&destRect);
    }

    virtual void FillRect(const Rect& rect, unsigned char color)
    {
        SDL_Rect rectSDL = {rect.X, rect.Y, rect.Width + 1, rect.Height + 1};
        SDL_FillRect(surface, &rectSDL, color);
    }

    void SetScaleFilter(bool smooth)
    {
        if (texture != nullptr) {
            SDL_SetTextureScaleMode(texture, smooth ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
        }
    }

    void RenderSurface()
    {
        void* pixels;
        int pitch;

        int output_w = 0;
        int output_h = 0;
        if (SDL_GetRendererOutputSize(renderer, &output_w, &output_h) == 0
            && output_w > 0 && output_h > 0
            && (output_w != render_output_w || output_h != render_output_h || render_dst.w <= 0 || render_dst.h <= 0)) {
            Update_HWCursor_Settings();
        }

        SDL_BlitSurface(surface, NULL, windowSurface, NULL);

        if (Settings.Video.HardwareCursor) {
            /*
            ** Swap cursor before a frame is drawn. This reduces flickering when it's done only once per frame.
            */
            if (hwcursor.Pending) {
                SDL_SetCursor(hwcursor.Pending);

                if (hwcursor.Current) {
                    SDL_FreeCursor(hwcursor.Current);
                }

                hwcursor.Current = hwcursor.Pending;
                hwcursor.Pending = nullptr;
            }

            /*
            ** Update hardware cursor visibility.
            */
            SDL_ShowCursor(!Get_Mouse_State());
        } else if (!Get_Mouse_State() && hwcursor.Surface != nullptr) {
            /*
            ** Draw software emulated cursor.
            */
            int x, y;
            SDL_Rect dst;

            Get_Video_Mouse(x, y);

            dst.x = x - hwcursor.HotX;
            dst.y = y - hwcursor.HotY;
            dst.w = hwcursor.Surface->w;
            dst.h = hwcursor.Surface->h;

            SDL_BlitSurface(hwcursor.Surface, nullptr, windowSurface, &dst);
        }

        SDL_UpdateTexture(texture, NULL, windowSurface->pixels, windowSurface->pitch);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, &render_dst);
        SDL_RenderPresent(renderer);
    }

private:
    SDL_Surface* surface;
    SDL_Surface* windowSurface;
    SDL_Texture* texture;
    GBC_Enum flags;
};

void Set_Video_Scale_Filter(bool smooth)
{
    Settings.Video.Scaler = smooth ? "linear" : "nearest";
    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, Settings.Video.Scaler.c_str(), SDL_HINT_OVERRIDE);
    if (frontSurface != nullptr) {
        frontSurface->SetScaleFilter(smooth);
    }
}

void Set_Video_Aspect_Mode(bool fill)
{
    Settings.Video.Boxing = !fill;
    Settings.Video.BoxingAspectRatio = "16:10";
    if (renderer != nullptr && window != nullptr) {
        Update_HWCursor_Settings();
    }
}

void Video_Render_Frame()
{
#ifdef IOS_BUILD
    if (Ratouch_iOS_Should_Pause()) {
        return;
    }
#endif
    if (frontSurface) {
        frontSurface->RenderSurface();
    }
}

/*
** Video
*/

Video::Video()
{
}

Video::~Video()
{
}

Video& Video::Shared()
{
    static Video video;
    return video;
}

VideoSurface* Video::CreateSurface(int w, int h, GBC_Enum flags)
{
    return new VideoSurfaceSDL2(w, h, flags);
}
