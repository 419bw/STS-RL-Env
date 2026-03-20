#pragma once

#include <vector>
#include <memory>
#include "Intent.h"

class GameState;
class Monster;

class IntentBrain {
protected:
    std::vector<IntentType> moveHistory;

public:
    virtual ~IntentBrain() = default;
    virtual Intent decide(GameState& state, Monster* owner) = 0;
};

using IntentBrainPtr = std::shared_ptr<IntentBrain>;
