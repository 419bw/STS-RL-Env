#include "RandomBrain.h"
#include "src/gamestate/GameState.h"
#include <random>

RandomBrain::RandomBrain(std::vector<Intent> intents)
    : possibleIntents(std::move(intents)) {}

Intent RandomBrain::decide(GameState& state, Monster* owner) {
    if (possibleIntents.empty()) {
        return Intent{IntentType::DEFEND, 0, 1, 0, nullptr};
    }
    std::uniform_int_distribution<size_t> dist(0, possibleIntents.size() - 1);
    Intent intent = possibleIntents[dist(state.rng.monsterRng)];
    recordMoveId(intent.move_id);
    return intent;
}
