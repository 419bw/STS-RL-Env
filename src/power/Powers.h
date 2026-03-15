#pragma once

#include "AbstractPower.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 具体状态：易伤 (Vulnerable) - 数值修饰型
// 
// 特性：
// - 受到伤害增加 50%（默认）
// - 轮次结束时层数 -1
// - 刚挂上的状态有保护罩，本轮次不掉层
// 
// 数据驱动原则：
// - Power 是纯粹的无状态计算器
// - 只读取参与双方的面板属性
// - 不关心属性是怎么来的（遗物、被动等）
// - 对比攻击者的输出倍率和自身的承受倍率，取极值
// ==========================================
class VulnerablePower : public AbstractPower {
public:
    VulnerablePower(int amount) 
        : AbstractPower("易伤", amount, PowerType::DEBUFF) {}
    
    // 重写受击伤害计算（数据驱动）
    // source: 攻击者，用于读取其 vulnerableDamageDealtMultiplier
    float modifyDamageTaken(float damage, Character* source = nullptr) override;
    
    // 注册轮次结束事件，实现掉层
    void onApply(GameState& state) override;
};

// ==========================================
// 具体状态：中毒 (Poison) - 触发型
// 
// 特性：
// - 回合开始时受到等同于层数的伤害
// - 回合开始时层数 -1
// ==========================================
class PoisonPower : public AbstractPower {
public:
    PoisonPower(int amount) 
        : AbstractPower("中毒", amount, PowerType::DEBUFF) {}

    void onApply(GameState& state) override;
};
