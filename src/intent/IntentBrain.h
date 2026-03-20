#pragma once

#include <vector>
#include <memory>
#include "Intent.h"

class GameState;
class Monster;

class IntentBrain {
protected:
    std::vector<int> moveIdHistory;

public:
    virtual ~IntentBrain() = default;
    virtual Intent decide(GameState& state, Monster* owner) = 0;
    virtual void initializeStats(int ascensionLevel);
    bool lastMove(int moveId) const;
    bool lastTwoMoves(int moveId) const;
    void recordMoveId(int moveId);
};

using IntentBrainPtr = std::shared_ptr<IntentBrain>;
