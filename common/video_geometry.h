#ifndef VIDEO_GEOMETRY_H
#define VIDEO_GEOMETRY_H

struct VideoPresentationGeometry
{
    int WindowWidth;
    int WindowHeight;
    int OutputWidth;
    int OutputHeight;
    int GameWidth;
    int GameHeight;
    int DestinationX;
    int DestinationY;
    int DestinationWidth;
    int DestinationHeight;
};

bool Video_Geometry_Is_Valid(const VideoPresentationGeometry& geometry);
bool Video_Window_Point_In_Presentation(const VideoPresentationGeometry& geometry, float window_x, float window_y);
bool Video_Presentation_Button_Event(bool pressed, bool inside_presentation, bool& accepted);
bool Video_Use_Relative_Mouse(bool raw_input, bool windowed);
void Video_Window_Point_To_Game(const VideoPresentationGeometry& geometry,
                                float window_x,
                                float window_y,
                                float& game_x,
                                float& game_y);
void Video_Window_Delta_To_Game(const VideoPresentationGeometry& geometry,
                                float window_dx,
                                float window_dy,
                                float& game_dx,
                                float& game_dy);
void Video_Game_Point_To_Window(const VideoPresentationGeometry& geometry,
                                float game_x,
                                float game_y,
                                float& window_x,
                                float& window_y);

#endif
