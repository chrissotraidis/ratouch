#include "wwtouch.h"

#include <cmath>

void WWMovieInputGuard::Begin_Poll(bool movie_active)
{
    if (WasMovieActive && !movie_active) {
        TransitionBlocked = true;
        TransitionInputSeen = false;
        TransitionNeedsRelease = false;
    }
    WasMovieActive = movie_active;
}

void WWMovieInputGuard::End_Poll()
{
    if (TransitionBlocked && (!TransitionInputSeen || !TransitionNeedsRelease)) {
        TransitionBlocked = false;
    }
    TransitionInputSeen = false;
}

WWMovieInputAction WWMovieInputGuard::Handle(bool movie_active, WWMovieInputPhase phase)
{
    if (movie_active) {
        if (phase == WWMovieInputPhase::Down) {
            Active = true;
            return WWMovieInputAction::Skip;
        }
        if (phase == WWMovieInputPhase::Up) {
            Active = false;
        }
        return WWMovieInputAction::Consume;
    }

    if (TransitionBlocked) {
        TransitionInputSeen = true;
        if (phase != WWMovieInputPhase::Up) {
            TransitionNeedsRelease = true;
        } else {
            TransitionNeedsRelease = false;
        }
        return WWMovieInputAction::Consume;
    }

    if (Active) {
        if (phase == WWMovieInputPhase::Up) {
            Active = false;
        }
        return WWMovieInputAction::Consume;
    }

    return WWMovieInputAction::Pass;
}

void WWPointerInputOwner::Observe_Touch()
{
    TouchOwned = true;
}

void WWPointerInputOwner::Observe_Pointer()
{
    TouchOwned = false;
}

bool WWPointerInputOwner::Touch_Owns_Cursor() const
{
    return TouchOwned;
}

void WWTouchScrollBuffer::Add(float dx, float dy)
{
    DeltaX += dx;
    DeltaY += dy;
}

bool WWTouchScrollBuffer::Peek(float& dx, float& dy) const
{
    dx = DeltaX;
    dy = DeltaY;
    return std::fabs(DeltaX) > 0.01f || std::fabs(DeltaY) > 0.01f;
}

bool WWTouchScrollBuffer::Consume(float& dx, float& dy)
{
    const bool pending = Peek(dx, dy);
    Clear();
    return pending;
}

void WWTouchScrollBuffer::Clear()
{
    DeltaX = 0.0f;
    DeltaY = 0.0f;
}

WWTouchState::WWTouchState() = default;

WWTouchState::WWTouchState(const Config& config)
    : Settings(config)
{
}

void WWTouchState::Set_Config(const Config& config)
{
    Settings = config;
}

WWTouchAction WWTouchState::Action(WWTouchActionType type, float x, float y, float dx, float dy, int steps)
{
    return {type, x, y, dx, dy, steps};
}

void WWTouchState::Reset()
{
    CurrentPhase = Phase::Idle;
    Finger1 = 0;
    Finger2 = 0;
    PinchDistance = 0.0f;
    PanReleaseCanceled = false;
    PanHasMoved = false;
    Finger1Moved = false;
    Finger2Moved = false;
}

void WWTouchState::Begin_Pan(std::vector<WWTouchAction>& actions, uint64_t ticks)
{
    PanX = RawPanX = (Finger1X + Finger2X) * 0.5f;
    PanY = RawPanY = (Finger1Y + Finger2Y) * 0.5f;
    const float dx = Finger1X - Finger2X;
    const float dy = Finger1Y - Finger2Y;
    PinchDistance = InitialPinchDistance = std::sqrt(dx * dx + dy * dy);
    PanTravel = 0.0f;
    PinchTravel = 0.0f;
    PanChangedZoom = false;
    PanHasMoved = false;
    Finger1Moved = false;
    Finger2Moved = false;
    PanStartTicks = ticks;
    actions.push_back(Action(WWTouchActionType::PanStart, PanX, PanY));
    CurrentPhase = Phase::Pan;
}

std::vector<WWTouchAction> WWTouchState::Finger_Down(int64_t id, float x, float y, uint64_t ticks)
{
    std::vector<WWTouchAction> actions;
    if (CurrentPhase == Phase::Idle) {
        Finger1 = id;
        Finger1X = DownX = LastX = x;
        Finger1Y = DownY = LastY = y;
        DownTicks = ticks;
        CurrentPhase = Phase::Pending;
        actions.push_back(Action(WWTouchActionType::CursorMove, x, y));
    } else if (CurrentPhase == Phase::Pending || CurrentPhase == Phase::Dragging) {
        Finger2 = id;
        Finger2X = x;
        Finger2Y = y;
        if (CurrentPhase == Phase::Dragging) {
            actions.push_back(Action(WWTouchActionType::LeftUp, LastX, LastY));
        }
        Begin_Pan(actions, ticks);
    }
    return actions;
}

std::vector<WWTouchAction> WWTouchState::Finger_Motion(int64_t id, float x, float y, uint64_t)
{
    std::vector<WWTouchAction> actions;
    if (CurrentPhase == Phase::DrainingPan && id == Finger1) {
        const float dx = x - Finger1X;
        const float dy = y - Finger1Y;
        PanTravel += std::sqrt(dx * dx + dy * dy);
        Finger1X = x;
        Finger1Y = y;
        return actions;
    }

    if (id == Finger1) {
        Finger1X = LastX = x;
        Finger1Y = LastY = y;
        Finger1Moved = true;
    } else if (CurrentPhase == Phase::Pan && id == Finger2) {
        Finger2X = x;
        Finger2Y = y;
        Finger2Moved = true;
    } else {
        return actions;
    }

    if (CurrentPhase == Phase::Pending && id == Finger1) {
        const float dx = x - DownX;
        const float dy = y - DownY;
        if (std::sqrt(dx * dx + dy * dy) >= Settings.DragThreshold) {
            actions.push_back(Action(WWTouchActionType::CursorMove, DownX, DownY));
            actions.push_back(Action(WWTouchActionType::LeftDown, DownX, DownY));
            actions.push_back(Action(WWTouchActionType::CursorMove, x, y));
            CurrentPhase = Phase::Dragging;
        }
    } else if (CurrentPhase == Phase::Dragging && id == Finger1) {
        actions.push_back(Action(WWTouchActionType::CursorMove, x, y));
    } else if (CurrentPhase == Phase::Pan) {
        const float centerX = (Finger1X + Finger2X) * 0.5f;
        const float centerY = (Finger1Y + Finger2Y) * 0.5f;
        const float rawDeltaX = centerX - RawPanX;
        const float rawDeltaY = centerY - RawPanY;
        PanTravel += std::sqrt(rawDeltaX * rawDeltaX + rawDeltaY * rawDeltaY);
        RawPanX = centerX;
        RawPanY = centerY;

        // SDL delivers each finger's motion separately. Sampling pan and pinch
        // after both fingers move prevents half of a translation from looking
        // like a pinch, and half of a pinch from nudging the map.
        if (!Finger1Moved || !Finger2Moved) {
            return actions;
        }

        const float centerDeltaX = centerX - PanX;
        const float centerDeltaY = centerY - PanY;
        if (PanTravel >= Settings.TwoFingerTapMovement) {
            PanHasMoved = true;
        }
        if (PanHasMoved && (std::fabs(centerDeltaX) > 0.01f || std::fabs(centerDeltaY) > 0.01f)) {
            actions.push_back(Action(WWTouchActionType::PanMove, centerX, centerY, centerDeltaX, centerDeltaY));
        }
        if (PanHasMoved) {
            PanX = centerX;
            PanY = centerY;
        }

        const float dx = Finger1X - Finger2X;
        const float dy = Finger1Y - Finger2Y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        PinchTravel = std::fmax(PinchTravel, std::fabs(distance - InitialPinchDistance));
        if (PinchDistance > 1.0f) {
            const float ratio = distance / PinchDistance;
            if (ratio > 1.0f + Settings.PinchStepRatio) {
                actions.push_back(Action(WWTouchActionType::ZoomStep, centerX, centerY, 0.0f, 0.0f, 1));
                PanChangedZoom = true;
                PinchDistance = distance;
            } else if (ratio < 1.0f - Settings.PinchStepRatio) {
                actions.push_back(Action(WWTouchActionType::ZoomStep, centerX, centerY, 0.0f, 0.0f, -1));
                PanChangedZoom = true;
                PinchDistance = distance;
            }
        }
        Finger1Moved = false;
        Finger2Moved = false;
    }
    return actions;
}

std::vector<WWTouchAction> WWTouchState::Finish_Finger(int64_t id, float x, float y, uint64_t ticks, bool canceled)
{
    std::vector<WWTouchAction> actions;
    const bool tracked = id == Finger1 || (CurrentPhase == Phase::Pan && id == Finger2);
    if (!tracked) {
        return actions;
    }

    switch (CurrentPhase) {
    case Phase::Pending:
        if (!canceled) {
            const float dx = x - DownX;
            const float dy = y - DownY;
            const bool releasedBeyondDragThreshold = std::sqrt(dx * dx + dy * dy) >= Settings.DragThreshold;
            actions.push_back(Action(WWTouchActionType::CursorMove, DownX, DownY));
            actions.push_back(Action(WWTouchActionType::LeftDown, DownX, DownY));
            if (releasedBeyondDragThreshold) {
                actions.push_back(Action(WWTouchActionType::CursorMove, x, y));
            }
            actions.push_back(Action(WWTouchActionType::LeftUp,
                                     releasedBeyondDragThreshold ? x : DownX,
                                     releasedBeyondDragThreshold ? y : DownY));
        }
        HasTwoFingerTap = false;
        break;
    case Phase::Dragging:
        actions.push_back(Action(WWTouchActionType::LeftUp, x, y));
        HasTwoFingerTap = false;
        break;
    case Phase::Pan:
        {
            const float oldCenterX = (Finger1X + Finger2X) * 0.5f;
            const float oldCenterY = (Finger1Y + Finger2Y) * 0.5f;
            if (id == Finger1) {
                Finger1X = x;
                Finger1Y = y;
            } else {
                Finger2X = x;
                Finger2Y = y;
            }
            const float centerX = (Finger1X + Finger2X) * 0.5f;
            const float centerY = (Finger1Y + Finger2Y) * 0.5f;
            const float centerDX = centerX - oldCenterX;
            const float centerDY = centerY - oldCenterY;
            PanTravel += std::sqrt(centerDX * centerDX + centerDY * centerDY);

            const float pinchDX = Finger1X - Finger2X;
            const float pinchDY = Finger1Y - Finger2Y;
            const float distance = std::sqrt(pinchDX * pinchDX + pinchDY * pinchDY);
            PinchTravel = std::fmax(PinchTravel, std::fabs(distance - InitialPinchDistance));
        }
        actions.push_back(Action(WWTouchActionType::PanEnd, PanX, PanY));
        PanReleaseCanceled = canceled;
        if (id == Finger1) {
            Finger1 = Finger2;
            Finger1X = Finger2X;
            Finger1Y = Finger2Y;
        }
        Finger2 = 0;
        CurrentPhase = Phase::DrainingPan;
        return actions;
    case Phase::DrainingPan:
        {
            const float dx = x - Finger1X;
            const float dy = y - Finger1Y;
            PanTravel += std::sqrt(dx * dx + dy * dy);
        }
        if (!canceled && !PanReleaseCanceled && !PanChangedZoom
            && ticks - PanStartTicks <= Settings.TwoFingerTapMilliseconds
            && PanTravel < Settings.TwoFingerTapMovement && PinchTravel < Settings.TwoFingerTapMovement) {
            if (HasTwoFingerTap && ticks - LastTwoFingerTapTicks <= Settings.DoubleTapMilliseconds) {
                actions.push_back(Action(WWTouchActionType::ZoomReset, PanX, PanY));
                HasTwoFingerTap = false;
            } else {
                LastTwoFingerTapTicks = ticks;
                HasTwoFingerTap = true;
            }
        } else {
            HasTwoFingerTap = false;
        }
        break;
    case Phase::Idle:
    case Phase::LongPressed:
        break;
    }
    Reset();
    return actions;
}

std::vector<WWTouchAction> WWTouchState::Finger_Up(int64_t id, float x, float y, uint64_t ticks)
{
    return Finish_Finger(id, x, y, ticks, false);
}

std::vector<WWTouchAction> WWTouchState::Finger_Canceled(int64_t id, float x, float y, uint64_t ticks)
{
    return Finish_Finger(id, x, y, ticks, true);
}

std::vector<WWTouchAction> WWTouchState::Poll(uint64_t ticks)
{
    std::vector<WWTouchAction> actions;
    if (CurrentPhase == Phase::Pending && ticks - DownTicks >= Settings.LongPressMilliseconds) {
        actions.push_back(Action(WWTouchActionType::CursorMove, DownX, DownY));
        actions.push_back(Action(WWTouchActionType::RightDown, DownX, DownY));
        actions.push_back(Action(WWTouchActionType::RightUp, DownX, DownY));
        CurrentPhase = Phase::LongPressed;
        HasTwoFingerTap = false;
    }
    return actions;
}

std::vector<WWTouchAction> WWTouchState::Cancel_All()
{
    std::vector<WWTouchAction> actions;
    if (CurrentPhase == Phase::Dragging) {
        actions.push_back(Action(WWTouchActionType::LeftUp, LastX, LastY));
    } else if (CurrentPhase == Phase::Pan) {
        actions.push_back(Action(WWTouchActionType::PanEnd, PanX, PanY));
    }
    HasTwoFingerTap = false;
    Reset();
    return actions;
}
