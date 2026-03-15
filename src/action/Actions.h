#pragma once

#include "AbstractAction.h"
#include "src/core/ForwardDeclarations.h"
#include <memory>
#include <string>

// ==========================================
// 占位动作 (Dummy Action) - 用于状态机流程
// ==========================================
class DummyAction : public AbstractAction {
    std::string name;
public:
    DummyAction(std::string n) : name(n) {}
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：伤害动作 (Damage Action)
// 
// 数据驱动原则：
// - 必须携带溯源信息（source: 伤害来源）
// - Action 负责将 source 传递给计算层
// ==========================================
class DamageAction : public AbstractAction {
    std::shared_ptr<Character> source;  // 伤害来源（攻击者）
    std::shared_ptr<Character> target;  // 伤害目标
    int amount;
public:
    DamageAction(std::shared_ptr<Character> src, std::shared_ptr<Character> tgt, int a);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：获得格挡动作 (Gain Block Action)
// ==========================================
class GainBlockAction : public AbstractAction {
    std::shared_ptr<Character> target;
    int amount;
public:
    GainBlockAction(std::shared_ptr<Character> t, int a);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：施加状态效果 (Apply Power Action)
// 
// 保护罩逻辑：
// - 如果在怪物回合 (!isPlayerTurn) 且释放者是怪物，设置 justApplied = true
// - 否则 justApplied = false
// ==========================================
class ApplyPowerAction : public AbstractAction {
    std::shared_ptr<Character> source;  // 释放者
    std::shared_ptr<Character> target;  // 目标
    std::shared_ptr<AbstractPower> power;
public:
    ApplyPowerAction(std::shared_ptr<Character> src, 
                      std::shared_ptr<Character> tgt, 
                      std::shared_ptr<AbstractPower> p);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：减少状态效果层数 (Reduce Power Action)
// ==========================================
class ReducePowerAction : public AbstractAction {
    std::shared_ptr<Character> target;
    std::shared_ptr<AbstractPower> power;
    int reduceAmount;
public:
    ReducePowerAction(std::shared_ptr<Character> t, std::shared_ptr<AbstractPower> p, int a);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：强制移除状态效果 (Remove Specific Power Action)
// 
// 无视层数，直接从目标身上移除指定的 Power
// 用于状态层数归零时的清理
// ==========================================
class RemoveSpecificPowerAction : public AbstractAction {
    std::shared_ptr<Character> target;
    std::shared_ptr<AbstractPower> power;
public:
    RemoveSpecificPowerAction(std::shared_ptr<Character> t, std::shared_ptr<AbstractPower> p);
    bool update(GameState& state) override;
};
