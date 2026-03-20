#include "FixedBrain.h"
#include "src/gamestate/GameState.h"

FixedBrain::FixedBrain(std::vector<Intent> sequence)
    : intentSequence(std::move(sequence)), currentIndex(0) {}

Intent FixedBrain::decide(GameState& state, Monster* owner) {
    Intent intent = intentSequence[currentIndex];
    moveHistory.push_back(intent.type);
    currentIndex = (currentIndex + 1) % intentSequence.size();
    return intent;
}
