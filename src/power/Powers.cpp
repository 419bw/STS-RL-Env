#include "Powers.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/action/Actions.h"
#include "src/character/Character.h"
#include <iostream>

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
                std::cout << "[" << self->name << "] 触发！向队列推入 " << self->amount 
                          << " 点伤害动作以及掉层动作。\n";
                
                gs.addAction(std::make_unique<DamageAction>(self->owner, self->amount));
                gs.addAction(std::make_unique<ReducePowerAction>(self->owner, self, 1));
            }
            return true;
        });
}

// ==========================================
// VulnerablePower 实现
// 
// 特性：轮次结束时掉层，有保护罩机制
// ==========================================
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
                std::cout << "[" << self->name << "] 保护罩解除，下轮次开始正常掉层\n";
                return true;
            }
            
            // 正常掉层
            if (self->amount > 0) {
                std::cout << "[" << self->name << "] 轮次结束掉层："
                          << self->amount << " -> " << (self->amount - 1) << "\n";
                gs.addAction(std::make_unique<ReducePowerAction>(self->owner, self, 1));
            }
            return true;
        });
}
