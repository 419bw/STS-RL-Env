#include "Powers.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/action/Actions.h"
#include "src/character/Character.h"
#include "src/utils/Logger.h"
#include <iostream>

// ==========================================
// VulnerablePower 实现
// 
// 数据驱动原则：
// - Power 是纯粹的无状态计算器
// - 只读取参与双方的面板属性
// - 不关心属性是怎么来的
// - 不跨层级访问全局游戏状态
// - 不硬编码特定遗物名称
// ==========================================

float VulnerablePower::modifyDamageTaken(float damage, Character* source) {
    // 层数为 0 时不再生效
    if (amount <= 0) {
        return damage;
    }
    
    float multiplier = 1.5f; 

    // 1. 如果攻击方有特化面板（比如纸蛙 1.75），用攻击方的
    if (source && source->vulnerableDamageDealtMultiplier != 1.5f) {
        multiplier = source->vulnerableDamageDealtMultiplier;
    }
    
    // 2. 如果受击方有特化面板（比如蘑菇 1.25），用受击方的
    if (owner && owner->vulnerableDamageReceivedMultiplier != 1.5f) {
        multiplier = owner->vulnerableDamageReceivedMultiplier;
    }
    
    float final_damage = damage * multiplier;
    ENGINE_TRACE("[" << name << "] 伤害修饰: " << damage << " -> " << final_damage 
                 << " (x" << multiplier << " 倍率)");
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
            
            // 检查保护罩
            if (self->justApplied) {
                // 刚挂上的状态，剥夺保护罩，本轮次不掉层
                self->justApplied = false;
                ENGINE_TRACE("VulnerablePower 保护罩解除: 下轮次开始正常掉层");
                return true;
            }
            
            // 按照官方逻辑：
            // - 如果层数为 0，直接移除
            // - 否则，减少 1 层
            if (self->amount == 0) {
                gs.addAction(std::make_unique<RemoveSpecificPowerAction>(self->owner, self));
            } else {
                gs.addAction(std::make_unique<ReducePowerAction>(self->owner, self, 1));
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
            if (current_turn_char == self->owner.get()) {
                if (self->amount == 0) {
                    gs.addAction(std::make_unique<RemoveSpecificPowerAction>(self->owner, self));
                } else {
                    ENGINE_TRACE("PoisonPower 触发: " << self->amount << " 层中毒造成伤害");
                    // 中毒：无视护甲，直接扣血
                    gs.addAction(std::make_unique<LoseHpAction>(self->owner, self->amount));
                    gs.addAction(std::make_unique<ReducePowerAction>(self->owner, self, 1));
                }
            }
            return true;
        });
}
