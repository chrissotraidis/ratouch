#ifndef RATOUCH_IOS_MODIFIER_H
#define RATOUCH_IOS_MODIFIER_H

#include <atomic>

enum class RatouchModifier
{
    None = 0,
    ForceAttack,
    ForceMove,
    AddSelection,
    QueueMove,
};

class RatouchModifierState
{
public:
    RatouchModifier Toggle(RatouchModifier modifier);
    RatouchModifier Active() const;
    RatouchModifier Consume();
    RatouchModifier Cancel();

private:
    std::atomic<int> ActiveModifier{static_cast<int>(RatouchModifier::None)};
};

#endif
