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
// 
// 数据驱动原则：
// - 卡牌使用时必须传递 source（玩家）给 DamageAction
// - 这样 Power 才能读取攻击者的面板属性
// ==========================================
void StrikeCard::use(GameState& state, std::shared_ptr<Character> target) {
    STS_LOG(state, "打出了 打击!\n");
    
    if (target) {
        state.addAction(std::make_unique<ApplyPowerAction>(
            state.player, target, std::make_shared<VulnerablePower>(1)));
        
        state.addAction(std::make_unique<DamageAction>(state.player, target, 6));
    }
}

// ==========================================
// DeadlyPoisonCard 实现
// ==========================================
void DeadlyPoisonCard::use(GameState& state, std::shared_ptr<Character> target) {
    STS_LOG(state, "打出了 致命毒药!\n");
    
    if (target) {
        state.addAction(std::make_unique<ApplyPowerAction>(
            state.player, target, std::make_shared<PoisonPower>(5)));
    }
}

// ==========================================
// WhirlwindCard 实现
// 
// 数据驱动原则：
// - 旋风斩的 source 是玩家
// - 对所有存活的怪物造成伤害
// ==========================================
void WhirlwindCard::use(GameState& state, std::shared_ptr<Character> target) {
    STS_LOG(state, "打出了 旋风斩! 消耗了 "
              << energyOnUse << " 点 X 费用。\n");
    
    // 旋风斩：对每个怪物都造成 energyOnUse 次伤害
    for (int i = 0; i < energyOnUse; i++) {
        for (auto& monster : state.monsters) {
            if (!monster->isDead()) {
                state.addAction(std::make_unique<DamageAction>(state.player, monster, 5));
            }
        }
    }
}

// ==========================================
// ShurikenCard 实现
//
// 效果：对随机目标造成 3 次 3 点伤害
// ==========================================
ShurikenCard::ShurikenCard()
    : AbstractCard("Shuriken", 1, CardType::ATTACK, CardTarget::RANDOM) {}

void ShurikenCard::use(GameState& state, std::shared_ptr<Character> target) {
    STS_LOG(state, "打出了 飞剑回旋镖!\n");
    
    for (int i = 0; i < 3; ++i) {
        state.addAction(std::make_unique<RandomDamageAction>(
            state.player, 3, DamageType::ATTACK));
    }
}
