#include "ios_modifier.h"

#include <cassert>
#include <initializer_list>

int main()
{
    RatouchModifierState state;
    assert(state.Active() == RatouchModifier::None);

    assert(state.Toggle(RatouchModifier::ForceAttack) == RatouchModifier::None);
    assert(state.Active() == RatouchModifier::ForceAttack);
    assert(state.Consume() == RatouchModifier::ForceAttack);
    assert(state.Active() == RatouchModifier::None);

    assert(state.Toggle(RatouchModifier::ForceMove) == RatouchModifier::None);
    assert(state.Toggle(RatouchModifier::AddSelection) == RatouchModifier::ForceMove);
    assert(state.Active() == RatouchModifier::AddSelection);
    assert(state.Cancel() == RatouchModifier::AddSelection);
    assert(state.Active() == RatouchModifier::None);

    assert(state.Toggle(RatouchModifier::QueueMove) == RatouchModifier::None);
    assert(state.Toggle(RatouchModifier::QueueMove) == RatouchModifier::QueueMove);
    assert(state.Active() == RatouchModifier::None);

    for (RatouchModifier modifier : {RatouchModifier::ForceAttack,
                                     RatouchModifier::ForceMove,
                                     RatouchModifier::AddSelection,
                                     RatouchModifier::QueueMove}) {
        assert(state.Toggle(modifier) == RatouchModifier::None);
        assert(state.Active() == modifier);
        assert(state.Cancel() == modifier);
        assert(state.Active() == RatouchModifier::None);
    }

    return 0;
}
