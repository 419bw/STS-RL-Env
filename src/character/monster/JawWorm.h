#pragma once

#include "src/character/Character.h"
#include "src/intent/brains/JawWormBrain.h"

class JawWorm : public Monster {
private:
    std::shared_ptr<JawWormBrain> brain_;
    int bellowStr = 3;

public:
    JawWorm(int ascensionLevel = 0);
    void initializeStats(int ascensionLevel);
    void executeSpecialIntent(GameEngine& engine) override;
};