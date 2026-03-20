#include "JawWorm.h"
#include "src/gamestate/GameState.h"
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

void JawWorm::executeSpecialIntent(GameState& state) {
    const Intent& intent = getRealIntent();
    if (intent.move_id == JawWormBrain::BELLOW) {
        STS_LOG(state, "    [" << name << "] BELLOW，获得护盾 + 力量\n");
        state.addAction(std::make_unique<GainBlockAction>(
            shared_from_this(), intent.effect_value));
        state.addAction(std::make_unique<ApplyPowerAction>(
            shared_from_this(), shared_from_this(), std::make_shared<StrengthPower>(bellowStr)));
    }
}