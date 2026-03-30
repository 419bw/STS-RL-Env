#pragma once

#include <vector>
#include <memory>
#include "Intent.h"

class CombatState;
class Monster;

class IntentBrain {
protected:
    std::vector<int> moveIdHistory;

public:
    virtual ~IntentBrain() = default;
    virtual Intent decide(CombatState& combat, Monster* owner) = 0;
    virtual void initializeStats(int ascensionLevel);
    virtual void reset();
    bool lastMove(int moveId) const;
    bool lastTwoMoves(int moveId) const;
    void recordMoveId(int moveId);
};

using IntentBrainPtr = std::shared_ptr<IntentBrain>;
