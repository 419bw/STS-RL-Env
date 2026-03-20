#include "AdaptiveBrain.h"
#include "src/gamestate/GameState.h"

AdaptiveBrain& AdaptiveBrain::addRule(IntentEvaluator eval) {
    evaluators.push_back(eval);
    return *this;
}

Intent AdaptiveBrain::decide(GameState& state, Monster* owner) {
    Character* playerPtr = state.player ? state.player.get() : nullptr;

    for (auto& eval : evaluators) {
        if (auto intentOpt = eval(state, owner, playerPtr)) {
            recordMoveId(intentOpt->move_id);
            if (!intentOpt->target && playerPtr) {
                intentOpt->target = playerPtr;
            }
            return *intentOpt;
        }
    }

    Intent intent{IntentType::DEFEND, 0, 1, 0, nullptr};
    if (playerPtr) {
        intent.target = playerPtr;
    }
    return intent;
}
