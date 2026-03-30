#include "JawWorm.h"
#include "src/engine/GameEngine.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/utils/Logger.h"

JawWorm::JawWorm(int ascensionLevel)
    : Monster("Jaw Worm", 44), brain_(std::make_shared<JawWormBrain>(ascensionLevel)) {
    setBrain(brain_);
    initializeStats(ascensionLevel);
}

void JawWorm::initializeStats(int ascensionLevel) {
    max_hp = (ascensionLevel >= 7) ? 44 : 42;
    current_hp = max_hp;
    brain_->initializeStats(ascensionLevel);
    bellowStr = (ascensionLevel >= 17) ? 5 : (ascensionLevel >= 2) ? 4 : 3;
}

void JawWorm::executeSpecialIntent(GameEngine& engine) {
    auto& combat = *engine.combatState;
    const Intent& intent = getRealIntent();
    if (intent.move_id == JawWormBrain::BELLOW) {
        STS_LOG(combat, "    [" << name << "] BELLOW，获得护盾 + 力量\n");
        engine.actionManager.addAction(std::make_unique<GainBlockAction>(
            shared_from_this(), intent.effect_value));
        engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(
            shared_from_this(), shared_from_this(), std::make_shared<StrengthPower>(bellowStr)));
    }
}
