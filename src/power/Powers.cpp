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
// 查询表单系统：
// - Power 创建查询表单
// - 递给攻击者和受击者的遗物填表
// - 最终读取表单结果结算
// - 零开销抽象，栈内存创建表单
// ==========================================

float VulnerablePower::modifyDamageTaken(float damage, Character* source) {
    // 层数为 0 时不再生效
    if (getAmount() <= 0) {
        return damage;
    }
    
    // 1. 拿出一张崭新的查询表单（栈内存，零开销）
    VulnerableMultiplierQuery query{source, getOwner().get()};
    
    // 2. 把表单递给攻击者，让他看看有没有什么要改的（触发纸蛙）
    if (source) {
        source->processQuery(query);
    }
    
    // 3. 把表单递给受击者，让他看看有没有什么要改的（触发蘑菇）
    if (getOwner()) {
        getOwner()->processQuery(query);
    }
    
    // 4. 结算收工！
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
