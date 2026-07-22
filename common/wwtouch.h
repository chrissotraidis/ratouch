#ifndef WWTOUCH_H
#define WWTOUCH_H

#include <cstdint>
#include <vector>

enum class WWTouchActionType
{
    CursorMove,
    LeftDown,
    LeftUp,
    RightDown,
    RightUp,
    PanStart,
    PanMove,
    PanEnd,
    ZoomStep,
    ZoomReset,
};

struct WWTouchAction
{
    WWTouchActionType Type;
    float X;
    float Y;
    float DeltaX;
    float DeltaY;
    int Steps;
};

enum class WWMovieInputPhase
{
    Down,
    Motion,
    Up,
};

enum class WWMovieInputAction
{
    Pass,
    Consume,
    Skip,
};

// Keeps one input gesture on one side of a movie/menu transition. A press
// skips the movie, its motion/release are consumed, and the next gesture is
// allowed through immediately.
class WWMovieInputGuard
{
public:
    void Begin_Poll(bool movie_active);
    void End_Poll();
    WWMovieInputAction Handle(bool movie_active, WWMovieInputPhase phase);

private:
    bool Active = false;
    bool WasMovieActive = false;
    bool TransitionBlocked = false;
    bool TransitionInputSeen = false;
    bool TransitionNeedsRelease = false;
};

// Tracks which physical input source most recently owned the virtual cursor.
// Touch-to-mouse synthesis is disabled at the SDL boundary, so the last real
// finger or pointer event is the authoritative source.
class WWPointerInputOwner
{
public:
    void Observe_Touch();
    void Observe_Pointer();
    bool Touch_Owns_Cursor() const;

private:
    bool TouchOwned = false;
};

// Collects presentation-mapped two-finger travel until the game loop consumes
// it. Unlike an analog-stick latch, an empty buffer means the camera stops.
class WWTouchScrollBuffer
{
public:
    void Add(float dx, float dy);
    bool Peek(float& dx, float& dy) const;
    bool Consume(float& dx, float& dy);
    void Clear();

private:
    float DeltaX = 0.0f;
    float DeltaY = 0.0f;
};

class WWTouchState
{
public:
    struct Config
    {
        uint64_t LongPressMilliseconds = 600;
        uint64_t TwoFingerTapMilliseconds = 250;
        uint64_t DoubleTapMilliseconds = 350;
        float DragThreshold = 8.0f;
        float TwoFingerTapMovement = 8.0f;
        float PinchStepRatio = 0.06f;
    };

    WWTouchState();
    explicit WWTouchState(const Config& config);
    void Set_Config(const Config& config);

    std::vector<WWTouchAction> Finger_Down(int64_t id, float x, float y, uint64_t ticks);
    std::vector<WWTouchAction> Finger_Motion(int64_t id, float x, float y, uint64_t ticks);
    std::vector<WWTouchAction> Finger_Up(int64_t id, float x, float y, uint64_t ticks);
    std::vector<WWTouchAction> Finger_Canceled(int64_t id, float x, float y, uint64_t ticks);
    std::vector<WWTouchAction> Poll(uint64_t ticks);
    std::vector<WWTouchAction> Cancel_All();

private:
    enum class Phase
    {
        Idle,
        Pending,
        Dragging,
        LongPressed,
        Pan,
        DrainingPan,
    };

    std::vector<WWTouchAction> Finish_Finger(int64_t id, float x, float y, uint64_t ticks, bool canceled);
    void Begin_Pan(std::vector<WWTouchAction>& actions, uint64_t ticks);
    WWTouchAction Action(WWTouchActionType type, float x, float y, float dx = 0.0f, float dy = 0.0f, int steps = 0);
    void Reset();

    Config Settings;
    Phase CurrentPhase = Phase::Idle;
    int64_t Finger1 = 0;
    int64_t Finger2 = 0;
    float DownX = 0.0f;
    float DownY = 0.0f;
    float LastX = 0.0f;
    float LastY = 0.0f;
    float Finger1X = 0.0f;
    float Finger1Y = 0.0f;
    float Finger2X = 0.0f;
    float Finger2Y = 0.0f;
    float PanX = 0.0f;
    float PanY = 0.0f;
    float RawPanX = 0.0f;
    float RawPanY = 0.0f;
    float PanTravel = 0.0f;
    float PinchDistance = 0.0f;
    float InitialPinchDistance = 0.0f;
    float PinchTravel = 0.0f;
    uint64_t DownTicks = 0;
    uint64_t PanStartTicks = 0;
    uint64_t LastTwoFingerTapTicks = 0;
    bool HasTwoFingerTap = false;
    bool PanChangedZoom = false;
    bool PanReleaseCanceled = false;
    bool PanHasMoved = false;
    bool Finger1Moved = false;
    bool Finger2Moved = false;
};

#endif
