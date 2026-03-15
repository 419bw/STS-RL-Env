#include "Cards.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/character/Character.h"
#include <iostream>

// ==========================================
// StrikeCard 实现
// ==========================================
void StrikeCard::use(GameState& state, std::shared_ptr<Character> target) {
    // 先挂易伤，再打伤害，演示动作队列的顺序性
    std::cout << "打出了 打击(Strike) 附带挂易伤特效!\n";
    
    // 动作1：施加易伤 (进队列)
    state.addAction(std::make_unique<ApplyPowerAction>(
        target, std::make_shared<VulnerablePower>(1)));
    
    // 动作2：造成伤害 (进队列，排在施加易伤之后)
    state.addAction(std::make_unique<DamageAction>(target, 6));
}

// ==========================================
// DeadlyPoisonCard 实现
// ==========================================
void DeadlyPoisonCard::use(GameState& state, std::shared_ptr<Character> target) {
    std::cout << "打出了 致命毒药(Deadly Poison)!\n";
    state.addAction(std::make_unique<ApplyPowerAction>(
        target, std::make_shared<PoisonPower>(5)));
}

// ==========================================
// WhirlwindCard 实现
// ==========================================
void WhirlwindCard::use(GameState& state, std::shared_ptr<Character> target) {
    std::cout << "打出了 旋风斩(Whirlwind)! 消耗了 " 
              << energyOnUse << " 点 X 费用。\n";
    
    // 核心逻辑：根据记录的 energyOnUse，决定往队列里塞多少个动作
    for (int i = 0; i < energyOnUse; i++) {
        // 真实游戏中这里会遍历所有怪物，为了简化我们只打第一个怪
        state.addAction(std::make_unique<DamageAction>(state.monsters[0], 5));
    }
}
