#include "Powers.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/action/Actions.h"
#include "src/character/Character.h"
#include <iostream>

// ==========================================
// PoisonPower 实现
// ==========================================
void PoisonPower::onApply(GameState& state) {
    // 使用 weak_ptr 捕获，避免延长对象生命周期
    std::weak_ptr<AbstractPower> weakSelf = shared_from_this();
    
    state.eventBus.subscribe(EventType::ON_TURN_START, 
        [weakSelf](GameState& gs, void* context) {
            // 尝试锁定 weak_ptr，检查对象是否存活
            auto self = weakSelf.lock();
            if (!self) {
                // 对象已销毁，安全跳过
                return;
            }
            
            Character* current_turn_char = static_cast<Character*>(context);
            // 如果轮到拥有此状态的人行动
            if (current_turn_char == self->owner.get() && self->amount > 0) {
                std::cout << "[" << self->name << "] 触发！向队列推入 " << self->amount 
                          << " 点伤害动作以及掉层动作。\n";
                
                // 动作1：将伤害动作推入队列！
                gs.addAction(std::make_unique<DamageAction>(self->owner, self->amount));
                
                // 动作2：将掉层动作也推入队列，紧跟在伤害之后！
                gs.addAction(std::make_unique<ReducePowerAction>(self->owner, self, 1));
            }
        });
}
