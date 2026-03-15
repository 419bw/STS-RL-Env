#pragma once

#include "AbstractPower.h"
#include "src/core/ForwardDeclarations.h"
#include <iostream>

// ==========================================
// 具体状态：易伤 (Vulnerable) - 数值修饰型，不进队列
// ==========================================
class VulnerablePower : public AbstractPower {
public:
    VulnerablePower(int amount) 
        : AbstractPower("易伤", amount, PowerType::DEBUFF) {}
    
    // 重写受击伤害计算：增加 50%
    float modifyDamageTaken(float damage) override {
        std::cout << "  (易伤状态使伤害从 " << damage 
                  << " 增加到 " << damage * 1.5f << ")\n";
        return damage * 1.5f;
    }
};

// ==========================================
// 具体状态：中毒 (Poison) - 触发型，回合开始时进队列
// ==========================================
class PoisonPower : public AbstractPower {
public:
    PoisonPower(int amount) 
        : AbstractPower("中毒", amount, PowerType::DEBUFF) {}

    void onApply(GameState& state) override;
};
