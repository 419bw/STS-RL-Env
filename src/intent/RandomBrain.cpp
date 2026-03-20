#include "RandomBrain.h"
#include "src/gamestate/GameState.h"
#include <random>

RandomBrain::RandomBrain(std::vector<Intent> intents)
    : possibleIntents(std::move(intents)) {}

Intent RandomBrain::decide(GameState& state, Monster* owner) {
    std::uniform_int_distribution<size_t> dist(0, possibleIntents.size() - 1);
    Intent intent = possibleIntents[dist(state.rng.monsterRng)];
    moveHistory.push_back(intent.type);
    return intent;
}
