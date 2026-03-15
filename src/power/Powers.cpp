#include "Powers.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/action/Actions.h"
#include "src/character/Character.h"
#include <iostream>

// ==========================================
// PoisonPower 实现
// 
// 回调返回值：
// - true：对象存活，继续监听
// - false：对象已销毁，自动移除监听者
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
            return true;  // 对象存活，继续监听
        });
}
