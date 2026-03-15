#include "Actions.h"
#include "src/power/AbstractPower.h"
#include "src/character/Character.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include <iostream>

// ==========================================
// DamageAction 实现
// ==========================================
DamageAction::DamageAction(std::shared_ptr<Character> t, int a) 
    : target(t), amount(a) {}

bool DamageAction::update(GameState& state) {
    if (!target->isDead()) {
        target->damage(amount);
        // 触发受到伤害事件
        state.eventBus.publish(EventType::ON_DAMAGE_TAKEN, state, target.get());
    }
    return true; // 立即完成
}

// ==========================================
// ApplyPowerAction 实现
// ==========================================
ApplyPowerAction::ApplyPowerAction(std::shared_ptr<Character> t, std::shared_ptr<AbstractPower> p) 
    : target(t), power(p) {}

bool ApplyPowerAction::update(GameState& state) {
    if (!target->isDead()) {
        std::cout << "-> 给 " << target->name << " 施加了 " << power->amount 
                  << " 层 [" << power->name << "]\n";
        power->owner = target;
        target->powers.push_back(power);
        power->onApply(state); // 触发状态自带的初始化/事件注册逻辑
    }
    return true;
}

// ==========================================
// ReducePowerAction 实现
// ==========================================
ReducePowerAction::ReducePowerAction(std::shared_ptr<Character> t, 
                                     std::shared_ptr<AbstractPower> p, int a) 
    : target(t), power(p), reduceAmount(a) {}

bool ReducePowerAction::update(GameState& state) {
    if (power && power->amount > 0) {
        power->amount -= reduceAmount;
        std::cout << "-> " << target->name << " 的 [" << power->name 
                  << "] 减少了 " << reduceAmount << " 层，剩余 " 
                  << power->amount << " 层。\n";
        if (power->amount <= 0) {
            std::cout << "-> [" << power->name << "] 已完全消散。\n";
        }
    }
    return true;
}
