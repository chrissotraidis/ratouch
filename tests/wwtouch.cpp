#include "wwtouch.h"

#include <cassert>
#include <initializer_list>
#include <vector>

namespace
{
void Expect(const std::vector<WWTouchAction>& actions, std::initializer_list<WWTouchActionType> expected)
{
    assert(actions.size() == expected.size());
    size_t index = 0;
    for (WWTouchActionType type : expected) {
        assert(actions[index++].Type == type);
    }
}

void ExpectCompoundPan(float dx, float dy)
{
    WWTouchState touch;
    touch.Finger_Down(1, 100, 100, 0);
    touch.Finger_Down(2, 140, 100, 10);
    Expect(touch.Finger_Motion(1, 100 + dx, 100 + dy, 20), {});
    const std::vector<WWTouchAction> actions = touch.Finger_Motion(2, 140 + dx, 100 + dy, 25);
    Expect(actions, {WWTouchActionType::PanMove});
    assert(actions[0].DeltaX == dx);
    assert(actions[0].DeltaY == dy);
}
}

int main()
{
    {
        WWTouchScrollBuffer scroll;
        float dx = 0.0f;
        float dy = 0.0f;
        assert(!scroll.Peek(dx, dy));
        scroll.Add(3, -4);
        scroll.Add(2, 1);
        assert(scroll.Peek(dx, dy));
        assert(dx == 5 && dy == -3);
        assert(scroll.Consume(dx, dy));
        assert(dx == 5 && dy == -3);
        assert(!scroll.Consume(dx, dy));
        scroll.Add(7, 8);
        scroll.Clear();
        assert(!scroll.Peek(dx, dy));
    }
    {
        WWPointerInputOwner owner;
        assert(!owner.Touch_Owns_Cursor());
        owner.Observe_Touch();
        assert(owner.Touch_Owns_Cursor());
        owner.Observe_Pointer();
        assert(!owner.Touch_Owns_Cursor());
        owner.Observe_Touch();
        assert(owner.Touch_Owns_Cursor());
    }
    {
        WWMovieInputGuard guard;
        assert(guard.Handle(true, WWMovieInputPhase::Down) == WWMovieInputAction::Skip);
        assert(guard.Handle(false, WWMovieInputPhase::Motion) == WWMovieInputAction::Consume);
        assert(guard.Handle(false, WWMovieInputPhase::Up) == WWMovieInputAction::Consume);
        assert(guard.Handle(false, WWMovieInputPhase::Down) == WWMovieInputAction::Pass);
    }
    {
        WWMovieInputGuard guard;
        guard.Begin_Poll(true);
        guard.End_Poll();
        guard.Begin_Poll(false);
        assert(guard.Handle(false, WWMovieInputPhase::Down) == WWMovieInputAction::Consume);
        guard.End_Poll();
        guard.Begin_Poll(false);
        assert(guard.Handle(false, WWMovieInputPhase::Motion) == WWMovieInputAction::Consume);
        guard.End_Poll();
        guard.Begin_Poll(false);
        assert(guard.Handle(false, WWMovieInputPhase::Up) == WWMovieInputAction::Consume);
        guard.End_Poll();
        guard.Begin_Poll(false);
        assert(guard.Handle(false, WWMovieInputPhase::Down) == WWMovieInputAction::Pass);
    }
    {
        WWMovieInputGuard guard;
        guard.Begin_Poll(true);
        guard.End_Poll();
        guard.Begin_Poll(false);
        assert(guard.Handle(false, WWMovieInputPhase::Down) == WWMovieInputAction::Consume);
        assert(guard.Handle(false, WWMovieInputPhase::Up) == WWMovieInputAction::Consume);
        guard.End_Poll();
        guard.Begin_Poll(false);
        assert(guard.Handle(false, WWMovieInputPhase::Down) == WWMovieInputAction::Pass);
    }
    {
        WWMovieInputGuard guard;
        guard.Begin_Poll(true);
        guard.End_Poll();
        guard.Begin_Poll(false);
        guard.End_Poll();
        guard.Begin_Poll(false);
        assert(guard.Handle(false, WWMovieInputPhase::Down) == WWMovieInputAction::Pass);
    }
    {
        WWMovieInputGuard guard;
        assert(guard.Handle(true, WWMovieInputPhase::Down) == WWMovieInputAction::Skip);
        assert(guard.Handle(true, WWMovieInputPhase::Up) == WWMovieInputAction::Consume);
        assert(guard.Handle(false, WWMovieInputPhase::Down) == WWMovieInputAction::Pass);
    }
    {
        WWTouchState touch;
        Expect(touch.Finger_Down(1, 20, 30, 0), {WWTouchActionType::CursorMove});
        Expect(touch.Finger_Up(1, 21, 31, 80),
               {WWTouchActionType::CursorMove, WWTouchActionType::LeftDown, WWTouchActionType::LeftUp});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 20, 30, 0);
        const std::vector<WWTouchAction> actions = touch.Finger_Up(1, 40, 50, 80);
        Expect(actions,
               {WWTouchActionType::CursorMove,
                WWTouchActionType::LeftDown,
                WWTouchActionType::CursorMove,
                WWTouchActionType::LeftUp});
        assert(actions[0].X == 20 && actions[0].Y == 30);
        assert(actions[2].X == 40 && actions[2].Y == 50);
        assert(actions[3].X == 40 && actions[3].Y == 50);
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 20, 30, 0);
        Expect(touch.Finger_Motion(1, 27, 30, 20), {});
        Expect(touch.Finger_Motion(1, 28, 30, 30),
               {WWTouchActionType::CursorMove, WWTouchActionType::LeftDown, WWTouchActionType::CursorMove});
        Expect(touch.Finger_Up(1, 40, 40, 60), {WWTouchActionType::LeftUp});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 20, 30, 0);
        Expect(touch.Poll(599), {});
        Expect(touch.Poll(600),
               {WWTouchActionType::CursorMove, WWTouchActionType::RightDown, WWTouchActionType::RightUp});
        Expect(touch.Finger_Up(1, 20, 30, 700), {});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 20, 30, 0);
        Expect(touch.Finger_Canceled(1, 20, 30, 100), {});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 20, 30, 0);
        Expect(touch.Cancel_All(), {});
        Expect(touch.Finger_Up(1, 20, 30, 100), {});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        Expect(touch.Finger_Down(2, 30, 10, 10), {WWTouchActionType::PanStart});
        Expect(touch.Finger_Motion(1, 20, 10, 20), {});
        const std::vector<WWTouchAction> actions = touch.Finger_Motion(2, 40, 10, 25);
        Expect(actions, {WWTouchActionType::PanMove});
        assert(actions[0].DeltaX == 10 && actions[0].DeltaY == 0);
        Expect(touch.Finger_Up(1, 20, 10, 30), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(2, 40, 10, 35), {});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        touch.Finger_Down(2, 30, 10, 10);
        Expect(touch.Finger_Motion(1, 16, 10, 20), {});
        Expect(touch.Finger_Motion(2, 36, 10, 25), {});
        Expect(touch.Finger_Up(1, 16, 10, 30), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(2, 36, 10, 35), {});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        touch.Finger_Down(2, 30, 10, 10);
        Expect(touch.Finger_Motion(1, 5, 10, 20), {});
        const std::vector<WWTouchAction> actions = touch.Finger_Motion(2, 35, 10, 25);
        Expect(actions, {WWTouchActionType::ZoomStep});
        assert(actions[0].Steps == 1);
        Expect(touch.Finger_Up(1, 5, 10, 30), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(2, 35, 10, 35), {});
    }
    {
        ExpectCompoundPan(10, 0);
        ExpectCompoundPan(-10, 0);
        ExpectCompoundPan(0, 10);
        ExpectCompoundPan(0, -10);
        ExpectCompoundPan(10, 10);
        ExpectCompoundPan(10, -10);
        ExpectCompoundPan(-10, 10);
        ExpectCompoundPan(-10, -10);
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        touch.Finger_Motion(1, 20, 10, 10);
        Expect(touch.Finger_Down(2, 30, 10, 20), {WWTouchActionType::LeftUp, WWTouchActionType::PanStart});
        Expect(touch.Cancel_All(), {WWTouchActionType::PanEnd});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        touch.Finger_Down(2, 30, 10, 10);
        Expect(touch.Finger_Up(1, 10, 10, 40), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(2, 30, 10, 45), {});
        touch.Finger_Down(3, 10, 10, 120);
        touch.Finger_Down(4, 30, 10, 130);
        Expect(touch.Finger_Up(3, 10, 10, 160), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(4, 30, 10, 165), {WWTouchActionType::ZoomReset});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        touch.Finger_Down(2, 30, 10, 10);
        touch.Finger_Up(1, 10, 10, 40);
        touch.Finger_Up(2, 30, 10, 45);
        touch.Finger_Down(3, 10, 10, 120);
        touch.Finger_Down(4, 30, 10, 130);
        Expect(touch.Finger_Motion(3, 5, 10, 135), {});
        Expect(touch.Finger_Motion(4, 35, 10, 140), {WWTouchActionType::ZoomStep});
        Expect(touch.Finger_Up(3, 10, 10, 160), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(4, 30, 10, 165), {});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        touch.Finger_Down(2, 30, 10, 10);
        touch.Finger_Up(1, 10, 10, 40);
        touch.Finger_Up(2, 30, 10, 45);
        touch.Finger_Down(3, 10, 10, 500);
        touch.Finger_Down(4, 30, 10, 510);
        Expect(touch.Finger_Up(3, 10, 10, 540), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(4, 30, 10, 545), {});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        touch.Finger_Down(2, 30, 10, 10);
        Expect(touch.Finger_Up(1, 10, 10, 40), {WWTouchActionType::PanEnd});

        // The remaining contact and any newly arriving finger stay isolated
        // from one-finger selection until the original pair has fully lifted.
        Expect(touch.Finger_Motion(2, 35, 10, 45), {});
        Expect(touch.Finger_Down(3, 50, 50, 50), {});
        Expect(touch.Finger_Up(3, 50, 50, 55), {});
        Expect(touch.Finger_Up(2, 35, 10, 60), {});

        Expect(touch.Finger_Down(4, 60, 60, 80), {WWTouchActionType::CursorMove});
        Expect(touch.Finger_Up(4, 60, 60, 100),
               {WWTouchActionType::CursorMove, WWTouchActionType::LeftDown, WWTouchActionType::LeftUp});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        touch.Finger_Down(2, 30, 10, 10);
        Expect(touch.Finger_Canceled(1, 10, 10, 40), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(2, 30, 10, 45), {});

        touch.Finger_Down(3, 10, 10, 100);
        touch.Finger_Down(4, 30, 10, 110);
        Expect(touch.Finger_Up(3, 10, 10, 140), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(4, 30, 10, 145), {});
    }
    {
        WWTouchState touch;
        touch.Finger_Down(1, 10, 10, 0);
        touch.Finger_Down(2, 30, 10, 10);
        Expect(touch.Finger_Up(1, 10, 10, 40), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(2, 45, 10, 45), {});

        // A moved surviving contact is a pan release, not the first half of a
        // double-two-finger tap.
        touch.Finger_Down(3, 10, 10, 100);
        touch.Finger_Down(4, 30, 10, 110);
        Expect(touch.Finger_Up(3, 10, 10, 140), {WWTouchActionType::PanEnd});
        Expect(touch.Finger_Up(4, 30, 10, 145), {});
    }
    {
        WWTouchState touch;
        WWTouchState::Config tuned;
        tuned.LongPressMilliseconds = 450;
        tuned.DragThreshold = 12.0f;
        touch.Set_Config(tuned);
        touch.Finger_Down(1, 20, 30, 0);
        Expect(touch.Finger_Motion(1, 31, 30, 100), {});
        Expect(touch.Poll(449), {});
        Expect(touch.Poll(450),
               {WWTouchActionType::CursorMove, WWTouchActionType::RightDown, WWTouchActionType::RightUp});
    }
    return 0;
}
