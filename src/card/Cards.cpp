#include "Cards.h"
#include "src/engine/GameEngine.h"
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
void StrikeCard::use(GameEngine& engine, std::shared_ptr<Character> target) {
    auto& combat = *engine.combatState;
    STS_LOG(combat, "打出了 打击!\n");

    if (target) {
        engine.actionManager.addAction(std::make_unique<DamageAction>(
            combat.player, target, 6, DamageType::ATTACK));
    }
}

// ==========================================
// DeadlyPoisonCard 实现
// ==========================================
void DeadlyPoisonCard::use(GameEngine& engine, std::shared_ptr<Character> target) {
    auto& combat = *engine.combatState;
    STS_LOG(combat, "打出了 致命毒药!\n");

    if (target) {
        engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(
            combat.player, target, std::make_shared<PoisonPower>(5)));
    }
}

// ==========================================
// WhirlwindCard 实现
//
// 数据驱动原则：
// - 旋风斩的 source 是玩家
// - 对所有存活的怪物造成伤害
// ==========================================
void WhirlwindCard::use(GameEngine& engine, std::shared_ptr<Character> target) {
    auto& combat = *engine.combatState;
    STS_LOG(combat, "打出了 旋风斩! 消耗了 "
              << energyOnUse << " 点 X 费用。\n");

    for (int i = 0; i < energyOnUse; i++) {
        for (auto& monster : combat.monsters) {
            if (!monster->isDead()) {
                engine.actionManager.addAction(std::make_unique<DamageAction>(
                    combat.player, monster, 5, DamageType::ATTACK));
            }
        }
    }
}

// ==========================================
// ShurikenCard 实现
//
// 效果：对随机目标造成 3 次 3 点伤害
// ==========================================
void ShurikenCard::use(GameEngine& engine, std::shared_ptr<Character> target) {
    auto& combat = *engine.combatState;
    STS_LOG(combat, "打出了 飞剑回旋镖!\n");

    for (int i = 0; i < 3; ++i) {
        engine.actionManager.addAction(std::make_unique<RandomDamageAction>(
            combat.player, 3, DamageType::ATTACK));
    }
}

// ==========================================
// PainCard 实现
//
// 效果：造成 8 点伤害，然后施加 2 层易伤
// ==========================================
void PainCard::use(GameEngine& engine, std::shared_ptr<Character> target) {
    auto& combat = *engine.combatState;
    STS_LOG(combat, "打出了 痛击!\n");

    if (target) {
        engine.actionManager.addAction(std::make_unique<DamageAction>(
            combat.player, target, 8, DamageType::ATTACK));
        engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(
            combat.player, target, std::make_shared<VulnerablePower>(2)));
    }
}
