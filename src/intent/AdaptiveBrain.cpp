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
            if (!intentOpt->target) {
                intentOpt->target = state.player.get();
            }
            return *intentOpt;
        }
    }
    Intent intent{IntentType::DEFEND, 0, 1, 0, nullptr};
    intent.target = state.player.get();
    return intent;
}
