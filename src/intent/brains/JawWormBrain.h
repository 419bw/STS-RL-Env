#pragma once

#include "src/intent/IntentBrain.h"
#include "src/intent/Intent.h"

class JawWormBrain : public IntentBrain {
public:
    enum MoveId { CHOMP = 1, BELLOW = 2, THRASH = 3 };

private:
    int ascensionLevel_ = 0;
    int chompDmg = 11;
    int thrashDmg = 7;
    int bellowStr = 3;
    int bellowBlock = 6;
    int thrashBlock = 5;
    bool firstMove = true;

    Intent decideNextMove(CombatState& combat, Monster* owner, int randomNum);

public:
    JawWormBrain(int ascensionLevel = 0);
    void initializeStats(int ascensionLevel) override;
    Intent decide(CombatState& combat, Monster* owner) override;
};
