#pragma once

#include "IntentBrain.h"
#include <vector>

class RandomBrain : public IntentBrain {
private:
    std::vector<Intent> possibleIntents;

public:
    RandomBrain(std::vector<Intent> intents);
    Intent decide(CombatState& combat, Monster* owner) override;
};
