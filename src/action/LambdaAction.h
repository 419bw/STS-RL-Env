#pragma once

#include <functional>
#include <memory>
#include "src/action/AbstractAction.h"

// ==========================================
// LambdaAction - 闭包动作
//
// 使用 lambda 表达式创建自定义动作
// 避免为简单操作创建大量 Action 子类
//
// 设计原则：
// - 通过 weak_ptr 管理 source 生命周期
// - 闭包安全由 weak_ptr lock() 保证
// - 禁止使用 [&] 捕获，必须使用 [=] 或显式捕获
//
// 使用示例：
// LambdaAction::make(source, [=](GameEngine& engine, Character* src) {
//     engine.runState->gold += 10;
// });
// ==========================================

class LambdaAction : public AbstractAction {
public:
    // ==========================================
    // 工厂函数
    // @source: 动作发起者（weak_ptr，防止悬挂）
    // @closure: 闭包函数
    // ==========================================
    static std::unique_ptr<LambdaAction> make(
        std::weak_ptr<Character> source,
        std::function<void(GameEngine&, Character* source)> closure
    );

    // ==========================================
    // 执行动作
    // ==========================================
    bool update(GameEngine& engine) override;

private:
    // ==========================================
    // 构造函数
    // ==========================================
    explicit LambdaAction(
        std::weak_ptr<Character> source,
        std::function<void(GameEngine&, Character* source)> closure
    );

    // 动作发起者（weak_ptr，防止悬挂）
    std::weak_ptr<Character> source_;

    // 闭包函数
    std::function<void(GameEngine&, Character* source)> closure_;
};
