#pragma once

#include "AbstractAction.h"
#include "src/core/ForwardDeclarations.h"
#include <memory>
#include <string>
#include <iostream>

// ==========================================
// 占位动作 (Dummy Action) - 用于状态机流程
// ==========================================
class DummyAction : public AbstractAction {
    std::string name;
public:
    DummyAction(std::string n) : name(n) {}
    bool update(GameState& state) override {
        std::cout << "    [动作队列执行] -> " << name << "\n";
        return true;
    }
};

// ==========================================
// 具体动作：伤害动作 (Damage Action)
// ==========================================
class DamageAction : public AbstractAction {
    std::shared_ptr<Character> target;
    int amount;
public:
    DamageAction(std::shared_ptr<Character> t, int a);
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
