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
// 特性：轮次结束时掉层，有保护罩机制
// ==========================================

float VulnerablePower::modifyDamageTaken(float damage) {
    // 层数为 0 时不再生效
    if (amount <= 0) {
        return damage;
    }
    
    float final_damage = damage * 1.5f;
    ENGINE_TRACE("[" << name << "] 伤害修饰: " << damage << " -> " << final_damage 
                 << " (+" << (final_damage - damage) << ", 50%易伤)");
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
// 特性：回合开始时受到伤害并掉层
// ==========================================
void PoisonPower::onApply(GameState& state) {
    std::weak_ptr<AbstractPower> weakSelf = shared_from_this();
    
    state.eventBus.subscribe(EventType::ON_TURN_START, 
        [weakSelf](GameState& gs, void* context) -> bool {
            auto self = weakSelf.lock();
            if (!self) {
                return false;  // 对象已销毁，通知 EventBus 移除此监听者
            }
            
            Character* current_turn_char = static_cast<Character*>(context);
            if (current_turn_char == self->owner.get() && self->amount > 0) {
                ENGINE_TRACE("PoisonPower 触发: " << self->amount << " 层中毒造成伤害");
                
                gs.addAction(std::make_unique<DamageAction>(self->owner, self->amount));
                gs.addAction(std::make_unique<ReducePowerAction>(self->owner, self, 1));
            }
            return true;
        });
}
