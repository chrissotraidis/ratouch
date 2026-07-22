#include "video_geometry.h"

#include <cassert>
#include <cmath>

namespace
{
void Expect_Near(float actual, float expected)
{
    assert(std::fabs(actual - expected) < 0.01f);
}
}

int main()
{
    const VideoPresentationGeometry retina_letterbox = {
        1512, 982, 3024, 1964, 640, 400, 0, 37, 3024, 1890,
    };

    float x = 0.0f;
    float y = 0.0f;
    Video_Window_Point_To_Game(retina_letterbox, 756.0f, 491.0f, x, y);
    Expect_Near(x, 320.0f);
    Expect_Near(y, 200.0f);

    assert(Video_Window_Point_In_Presentation(retina_letterbox, 756.0f, 491.0f));
    assert(!Video_Window_Point_In_Presentation(retina_letterbox, 756.0f, 10.0f));
    assert(Video_Window_Point_In_Presentation(retina_letterbox, 0.0f, 18.5f));
    assert(!Video_Window_Point_In_Presentation(retina_letterbox, 0.0f, 18.49f));
    assert(!Video_Window_Point_In_Presentation(VideoPresentationGeometry{}, 0.0f, 0.0f));

    bool accepted = false;
    assert(!Video_Presentation_Button_Event(true, false, accepted));
    assert(!accepted);
    assert(!Video_Presentation_Button_Event(false, true, accepted));

    assert(!Video_Use_Relative_Mouse(true, true));
    assert(Video_Use_Relative_Mouse(true, false));
    assert(!Video_Use_Relative_Mouse(false, true));
    assert(!Video_Use_Relative_Mouse(false, false));
    assert(Video_Presentation_Button_Event(true, true, accepted));
    assert(accepted);
    assert(Video_Presentation_Button_Event(false, false, accepted));
    assert(!accepted);
    assert(!Video_Presentation_Button_Event(false, true, accepted));

    Video_Window_Point_To_Game(retina_letterbox, 0.0f, 0.0f, x, y);
    Expect_Near(x, 0.0f);
    Expect_Near(y, 0.0f);

    Video_Window_Delta_To_Game(retina_letterbox, 100.0f, 100.0f, x, y);
    Expect_Near(x, 42.32804f);
    Expect_Near(y, 42.32804f);

    Video_Game_Point_To_Window(retina_letterbox, 320.0f, 200.0f, x, y);
    Expect_Near(x, 756.0f);
    Expect_Near(y, 491.0f);

    const VideoPresentationGeometry zoomed = {
        1512, 982, 3024, 1964, 640, 400, -756, -435, 4536, 2835,
    };
    Video_Window_Point_To_Game(zoomed, 756.0f, 491.25f, x, y);
    Expect_Near(x, 320.0f);
    Expect_Near(y, 200.0f);
    Video_Window_Delta_To_Game(zoomed, 100.0f, 100.0f, x, y);
    Expect_Near(x, 28.21869f);
    Expect_Near(y, 28.21869f);
    assert(Video_Window_Point_In_Presentation(zoomed, 0.0f, 0.0f));
    assert(Video_Window_Point_In_Presentation(zoomed, 1511.0f, 981.0f));

    return 0;
}
