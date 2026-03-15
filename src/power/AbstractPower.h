#pragma once

#include <string>
#include <memory>
#include "src/core/Types.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 状态效果基类 (Power System)
// ==========================================

class AbstractPower : public std::enable_shared_from_this<AbstractPower> {
public:
    std::string name;
    int amount; // 层数 (例如 3层中毒)
    PowerType type;
    std::shared_ptr<Character> owner;

    // 微观首回合保护罩：防止刚挂上的状态被轮次结束判定"误杀"
    // true = 刚挂上，本轮次不掉层
    // false = 正常状态，轮次结束时正常掉层
    bool justApplied;

    AbstractPower(std::string n, int a, PowerType t) 
        : name(n), amount(a), type(t), justApplied(false) {}
    virtual ~AbstractPower() = default;

    // 状态挂载时触发，用于注册事件
    virtual void onApply(GameState& state) {}
    
    // 供数值修饰型状态重写 (如易伤增加50%伤害)
    virtual float modifyDamageTaken(float damage) { return damage; }
    virtual float modifyDamageDealt(float damage) { return damage; }
};
