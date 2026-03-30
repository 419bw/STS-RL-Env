#include "AdaptiveBrain.h"
#include "src/state/CombatState.h"

AdaptiveBrain& AdaptiveBrain::addRule(IntentEvaluator eval) {
    evaluators.push_back(eval);
    return *this;
}

Intent AdaptiveBrain::decide(CombatState& combat, Monster* owner) {
    Character* playerPtr = combat.player ? combat.player.get() : nullptr;

    for (auto& eval : evaluators) {
        if (auto intentOpt = eval(combat, owner, playerPtr)) {
            recordMoveId(intentOpt->move_id);
            if (!intentOpt->target.lock() && playerPtr) {
                intentOpt->target = combat.player;
            }
            return *intentOpt;
        }
    }

    Intent intent{IntentType::DEFEND, 0, 1, 0, {}};
    if (playerPtr) {
        intent.target = combat.player;
    }
    return intent;
}
