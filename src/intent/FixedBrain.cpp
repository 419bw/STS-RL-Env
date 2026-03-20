#include "FixedBrain.h"
#include "src/gamestate/GameState.h"

FixedBrain::FixedBrain(std::vector<Intent> sequence)
    : intentSequence(std::move(sequence)), currentIndex(0) {}

Intent FixedBrain::decide(GameState& state, Monster* owner) {
    if (intentSequence.empty()) {
        return Intent{IntentType::DEFEND, 0, 1, 0, nullptr};
    }
    Intent intent = intentSequence[currentIndex];
    recordMoveId(intent.move_id);
    currentIndex = (currentIndex + 1) % intentSequence.size();
    return intent;
}
