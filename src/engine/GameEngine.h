#pragma once

#include <memory>
#include "src/event/EventBus.h"
#include "src/action/ActionManager.h"
#include "src/state/RunState.h"
#include "src/state/CombatState.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// GameEngine - 游戏全局引擎
//
// 三层架构核心：
// - runState: 持久层（一整局游戏）
// - combatState: 易失层（仅战斗中存在）
// - actionManager: 全局动作管理器
// - eventBus: 全局事件总线
//
// 设计原则：
// - GameEngine 是普通类，可被多次实例化（支持 RL 并行训练）
// - 绝非单例！多个 GameEngine 实例可并行运行
// ==========================================

class GameEngine {
public:
    GameEngine();
    ~GameEngine();

    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
    GameEngine(GameEngine&&) = default;
    GameEngine& operator=(GameEngine&&) = default;

    // ==========================================
    // 核心组件
    // ==========================================

    // 持久层：一整局游戏（Act 1 → 通关心脏）
    std::shared_ptr<RunState> runState;

    // 易失层：仅在战斗中存在（nullptr = 不在战斗）
    std::shared_ptr<CombatState> combatState;

    // 全局动作管理器（始终存在，地图/战斗通用）
    ActionManager actionManager;

    // 全局事件总线（贯穿全局，遗物效果注册于此）
    EventBus eventBus;

    // ==========================================
    // 核心接口
    // ==========================================

    // 开始新游戏
    void startNewRun(unsigned int seed);

    // 进入战斗
    void startCombat(std::shared_ptr<Monster> monster);

    // 结束战斗
    void endCombat();

    // 主循环推进
    void tick();

    // 是否在战斗中
    bool isInCombat() const { return combatState != nullptr; }
};

// 前向声明
class RunState;
class CombatState;
class Monster;
