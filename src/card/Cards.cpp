#include "Cards.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/character/Character.h"
#include "src/utils/Logger.h"
#include <iostream>

// ==========================================
// StrikeCard 实现
// ==========================================
void StrikeCard::use(GameState& state, std::shared_ptr<Character> target) {
    STS_LOG(state, "打出了 打击 附带挂易伤特效!\n");
    
    state.addAction(std::make_unique<ApplyPowerAction>(
        state.player, target, std::make_shared<VulnerablePower>(1)));
    
    state.addAction(std::make_unique<DamageAction>(target, 6));
}

// ==========================================
// DeadlyPoisonCard 实现
// ==========================================
void DeadlyPoisonCard::use(GameState& state, std::shared_ptr<Character> target) {
    STS_LOG(state, "打出了 致命毒药!\n");
    state.addAction(std::make_unique<ApplyPowerAction>(
        state.player, target, std::make_shared<PoisonPower>(5)));
}

// ==========================================
// WhirlwindCard 实现
// ==========================================
void WhirlwindCard::use(GameState& state, std::shared_ptr<Character> target) {
    STS_LOG(state, "打出了 旋风斩! 消耗了 " 
              << energyOnUse << " 点 X 费用。\n");
    
    for (int i = 0; i < energyOnUse; i++) {
        state.addAction(std::make_unique<DamageAction>(state.monsters[0], 5));
    }
}
