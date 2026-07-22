#include "video_geometry.h"

#include <algorithm>

namespace
{
float Clamp(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(value, maximum));
}
}

bool Video_Geometry_Is_Valid(const VideoPresentationGeometry& geometry)
{
    return geometry.WindowWidth > 0 && geometry.WindowHeight > 0 && geometry.OutputWidth > 0
        && geometry.OutputHeight > 0 && geometry.GameWidth > 0 && geometry.GameHeight > 0
        && geometry.DestinationWidth > 0 && geometry.DestinationHeight > 0;
}

bool Video_Window_Point_In_Presentation(const VideoPresentationGeometry& geometry, float window_x, float window_y)
{
    if (!Video_Geometry_Is_Valid(geometry)) {
        return false;
    }

    const float output_x = window_x * geometry.OutputWidth / geometry.WindowWidth;
    const float output_y = window_y * geometry.OutputHeight / geometry.WindowHeight;
    return output_x >= geometry.DestinationX && output_x < geometry.DestinationX + geometry.DestinationWidth
        && output_y >= geometry.DestinationY && output_y < geometry.DestinationY + geometry.DestinationHeight;
}

bool Video_Presentation_Button_Event(bool pressed, bool inside_presentation, bool& accepted)
{
    if (pressed) {
        accepted = inside_presentation;
        return accepted;
    }

    if (!accepted) {
        return false;
    }
    accepted = false;
    return true;
}

bool Video_Use_Relative_Mouse(bool raw_input, bool windowed)
{
    return raw_input && !windowed;
}

void Video_Window_Point_To_Game(const VideoPresentationGeometry& geometry,
                                float window_x,
                                float window_y,
                                float& game_x,
                                float& game_y)
{
    if (!Video_Geometry_Is_Valid(geometry)) {
        game_x = 0.0f;
        game_y = 0.0f;
        return;
    }

    const float output_x = window_x * geometry.OutputWidth / geometry.WindowWidth;
    const float output_y = window_y * geometry.OutputHeight / geometry.WindowHeight;
    game_x = (output_x - geometry.DestinationX) * geometry.GameWidth / geometry.DestinationWidth;
    game_y = (output_y - geometry.DestinationY) * geometry.GameHeight / geometry.DestinationHeight;
    game_x = Clamp(game_x, 0.0f, static_cast<float>(geometry.GameWidth - 1));
    game_y = Clamp(game_y, 0.0f, static_cast<float>(geometry.GameHeight - 1));
}

void Video_Window_Delta_To_Game(const VideoPresentationGeometry& geometry,
                                float window_dx,
                                float window_dy,
                                float& game_dx,
                                float& game_dy)
{
    if (!Video_Geometry_Is_Valid(geometry)) {
        game_dx = window_dx;
        game_dy = window_dy;
        return;
    }

    game_dx = window_dx * geometry.OutputWidth * geometry.GameWidth
        / (geometry.WindowWidth * geometry.DestinationWidth);
    game_dy = window_dy * geometry.OutputHeight * geometry.GameHeight
        / (geometry.WindowHeight * geometry.DestinationHeight);
}

void Video_Game_Point_To_Window(const VideoPresentationGeometry& geometry,
                                float game_x,
                                float game_y,
                                float& window_x,
                                float& window_y)
{
    if (!Video_Geometry_Is_Valid(geometry)) {
        window_x = 0.0f;
        window_y = 0.0f;
        return;
    }

    const float output_x = geometry.DestinationX + game_x * geometry.DestinationWidth / geometry.GameWidth;
    const float output_y = geometry.DestinationY + game_y * geometry.DestinationHeight / geometry.GameHeight;
    window_x = output_x * geometry.WindowWidth / geometry.OutputWidth;
    window_y = output_y * geometry.WindowHeight / geometry.OutputHeight;
}
