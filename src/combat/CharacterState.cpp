// =============================================================================
//  CharacterState.cpp  ——  状态模式中的"状态工厂"
// =============================================================================

#include "combat/CharacterState.hpp"

namespace gb {

CharacterState* makeState(StateKind kind) {
    switch (kind) {
        case StateKind::Stunned: return new StunnedState();
        case StateKind::Frozen:  return new FrozenState();
        case StateKind::Berserk: return new BerserkState();
        case StateKind::Normal:
        default:                 return new NormalState();
    }
}

} // namespace gb
