#include "AdaptiveBrain.h"
#include "src/gamestate/GameState.h"

AdaptiveBrain& AdaptiveBrain::addRule(IntentEvaluator eval) {
    evaluators.push_back(eval);
    return *this;
}

Intent AdaptiveBrain::decide(GameState& state, Monster* owner) {
    for (auto& eval : evaluators) {
        if (auto intentOpt = eval(state, owner, state.player.get())) {
            moveHistory.push_back(intentOpt->type);
            return *intentOpt;
        }
    }
    return Intent{IntentType::DEFEND, 0, 1, 0, nullptr};
}
