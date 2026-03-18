#include "Powers.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/action/Actions.h"
#include "src/character/Character.h"
#include "src/core/Queries.h"
#include "src/utils/Logger.h"
#include <iostream>

// ==========================================
// VulnerablePower 实现
// 
// 四阶段管线设计：
// - 在阶段2 (atDamageReceive) 应用易伤倍率
// - 遗物的修饰通过管线自动处理
// ==========================================

float VulnerablePower::atDamageReceive(float damage, DamageType type, Character* source) {
    // 易伤只对攻击伤害生效，荆棘和直接掉血不受影响
    if (type != DamageType::ATTACK) {
        return damage;
    }
    
    if (getAmount() <= 0) {
        return damage;
    }
    
    // 使用表单系统计算易伤倍率
    // 这样遗物可以修改倍率（如纸蛙+25%，蘑菇-25%）
    VulnerableMultiplierQuery query{source, getOwner().get()};
    
    // 让攻击者的遗物填表（如纸蛙）
    if (source) {
        source->processQuery(query);
    }
    
    // 让受击者的遗物填表（如奇数蘑菇）
    if (getOwner()) {
        getOwner()->processQuery(query);
    }
    
    float final_damage = damage * query.multiplier;
    ENGINE_TRACE("[" << name << "] 伤害修饰: " << damage << " -> " << final_damage 
                 << " (x" << query.multiplier << " 倍率)");
    return final_damage;
}

void VulnerablePower::onApply(GameState& state) {
    std::weak_ptr<AbstractPower> weakSelf = shared_from_this();
    
    state.eventBus.subscribe(EventType::ON_ROUND_END, 
        [weakSelf](GameState& gs, void*) -> bool {
            auto self = weakSelf.lock();
            if (!self) {
                return false;
            }
            
            if (self->isJustApplied()) {
                self->setJustApplied(false);
                ENGINE_TRACE("VulnerablePower 保护罩解除: 下轮次开始正常掉层");
                return true;
            }
            
            if (self->getAmount() == 0) {
                gs.addAction(std::make_unique<RemoveSpecificPowerAction>(self->getOwner(), self));
            } else {
                gs.addAction(std::make_unique<ReducePowerAction>(self->getOwner(), self, 1));
            }
            return true;
        });
}

// ==========================================
// PoisonPower 实现
// 
// 特性：
// - 回合开始时受到等同于层数的伤害
// - 无视护甲，直接扣除生命值
// - 回合开始时层数 -1
// ==========================================
void PoisonPower::onApply(GameState& state) {
    std::weak_ptr<AbstractPower> weakSelf = shared_from_this();
    
    state.eventBus.subscribe(EventType::ON_TURN_START, 
        [weakSelf](GameState& gs, void* context) -> bool {
            auto self = weakSelf.lock();
            if (!self) {
                return false;
            }
            
            Character* current_turn_char = static_cast<Character*>(context);
            if (current_turn_char == self->getOwner().get()) {
                if (self->getAmount() == 0) {
                    gs.addAction(std::make_unique<RemoveSpecificPowerAction>(self->getOwner(), self));
                } else {
                    ENGINE_TRACE("PoisonPower 触发: " << self->getAmount() << " 层中毒造成伤害");
                    gs.addAction(std::make_unique<LoseHpAction>(self->getOwner(), self->getAmount()));
                    gs.addAction(std::make_unique<ReducePowerAction>(self->getOwner(), self, 1));
                }
            }
            return true;
        });
}
