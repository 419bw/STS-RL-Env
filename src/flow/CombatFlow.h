#pragma once

#include "src/core/Types.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// CombatFlow - 战斗流程控制器
// 
// 铁律二：CombatFlow 必须是"瞎子"广播员
// - 管理宏观的战斗阶段轮转
// - 禁止 #include 任何具体的 Action 头文件
// - 只负责：结算动作队列、发布宏观阶段事件
// ==========================================

class CombatFlow {
public:
    CombatState currentState;

    CombatFlow() : currentState(CombatState::BATTLE_START) {}

    // 核心推进函数 (Tick)
    void tick(GameState& state);

    // SBA 全局巡视 (State-Based Action Check)
    // 在每个动作执行完毕后调用，检查全局状态变化
    void sbaGlobalCheck(GameState& state);

    // 检查战斗是否结束
    void checkBattleEndCondition(GameState& state);
};
