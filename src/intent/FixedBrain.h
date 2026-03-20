#pragma once

#include "IntentBrain.h"

class FixedBrain : public IntentBrain {
private:
    std::vector<Intent> intentSequence;
    size_t currentIndex = 0;

public:
    FixedBrain(std::vector<Intent> sequence);
    Intent decide(GameState& state, Monster* owner) override;
};
