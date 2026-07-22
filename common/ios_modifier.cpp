#include "ios_modifier.h"

RatouchModifier RatouchModifierState::Toggle(RatouchModifier modifier)
{
    int current = ActiveModifier.load(std::memory_order_acquire);
    for (;;) {
        const int requested = static_cast<int>(modifier);
        const int next = current == requested ? static_cast<int>(RatouchModifier::None) : requested;
        if (ActiveModifier.compare_exchange_weak(current, next, std::memory_order_acq_rel)) {
            return static_cast<RatouchModifier>(current);
        }
    }
}

RatouchModifier RatouchModifierState::Active() const
{
    return static_cast<RatouchModifier>(ActiveModifier.load(std::memory_order_acquire));
}

RatouchModifier RatouchModifierState::Consume()
{
    return static_cast<RatouchModifier>(
        ActiveModifier.exchange(static_cast<int>(RatouchModifier::None), std::memory_order_acq_rel));
}

RatouchModifier RatouchModifierState::Cancel()
{
    return Consume();
}
