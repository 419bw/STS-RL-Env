#pragma once

#include "src/core/Types.h"
#include "src/core/ForwardDeclarations.h"
#include "src/state/CombatState.h"

// ==========================================
// CombatFlow - 战斗流程控制器
//
// 铁律二：CombatFlow 必须是"瞎子"广播员
// - 管理宏观的战斗阶段轮转
// - 禁止 #include 任何具体的 Action 头文件
// - 只负责：结算动作队列、发布宏观阶段事件
// - 维护自己的内部状态，不直接修改 combatState->currentPhase
// ==========================================

class CombatFlow {
public:
    CombatFlow();
    // 核心推进函数 (Tick)
    void tick(GameEngine& engine);
    // SBA 全局巡视 (State-Based Action Check)
    // 在每个动作执行完毕后调用，检查全局状态变化
    void sbaGlobalCheck(GameEngine& engine);
    // 检查战斗是否结束
    void checkBattleEndCondition(GameEngine& engine);
    // 获取当前战斗阶段
    BattlePhase getCurrentPhase() const { return currentPhase; }
    // 设置当前战斗阶段
    void setPhase(BattlePhase phase) { currentPhase = phase; }

private:
    BattlePhase currentPhase;
};
