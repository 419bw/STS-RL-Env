#pragma once

#include <memory>
#include <deque>
#include "src/action/AbstractAction.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// ActionManager - 全局动作管理器
//
// ECS/DOD 架构核心组件：
// - 统一管理所有 Action 的入队和执行
// - 适用于战斗和地图节点
//
// 设计原则：
// - O(1) 入队操作
// - 防死锁看门狗（MAX_LOOPS = 1000）
// - executeUntilBlocked 只在 PLAYING_CARD 阶段执行
//
// 生命周期：
// - 始终存在于 GameEngine 中
// - 不随 CombatState 销毁而销毁
// ==========================================

class ActionManager {
public:
    // ==========================================
    // 动作队列
    // ==========================================
    std::deque<std::unique_ptr<AbstractAction>> actionQueue;
    std::unique_ptr<AbstractAction> currentAction;

    // ==========================================
    // 核心接口
    // ==========================================

    // O(1) 入队到队尾
    void addAction(std::unique_ptr<AbstractAction> action);

    // O(1) 入队到队头
    void addActionToFront(std::unique_ptr<AbstractAction> action);

    // 检查队列是否为空
    bool isQueueEmpty() const;

    // 执行动作直到被阻塞
    void executeUntilBlocked(GameEngine& engine, CombatFlow& flow);

private:
    // 防死锁看门狗
    int loopCount = 0;
    static constexpr int MAX_LOOPS = 1000;
};
