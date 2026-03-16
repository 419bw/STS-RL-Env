#pragma once

#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"

// ==========================================
// ActionSystem - 动作系统 (核心流转马达)
// 
// ECS/DOD 架构的核心组件：
// - 纯静态方法，无状态
// - 负责驱动动作队列的执行
// - 集成 SBA 全局状态巡视
// 
// 铁律：所有业务逻辑由 ActionSystem 执行
// GameState 只负责存储数据
// ==========================================

class ActionSystem {
public:
    // ==========================================
    // 静态驱动器：不断弹出并执行队列动作，直到被阻塞或清空
    // 
    // 核心逻辑：
    // 1. 只要队列不为空 且 处于自由出牌状态，就继续执行
    // 2. 当 currentPhase 变为 WAITING_FOR_CARD_SELECTION 时自动停止
    // 3. Action 返回 true 表示执行完毕，从队列移除
    // 4. 每个动作执行完毕后，进行 SBA 全局巡视
    // ==========================================
    static void executeUntilBlocked(GameState& state, CombatFlow& flow) {
        while (!state.actionQueue.empty() && state.currentPhase == StatePhase::PLAYING_CARD) {
            auto& action = state.actionQueue.front();
            bool completed = action->update(state);
            
            if (completed) {
                state.actionQueue.pop_front();
                
                // ★ 铁律：每个动作执行完毕后，必须进行上帝视角的巡视！
                flow.sbaGlobalCheck(state);
                flow.checkBattleEndCondition(state);
            }
            // 如果动作返回 false，说明切换了 Phase 阻塞了引擎，while 循环自动打破
        }
    }
    
    // ==========================================
    // 重载版本：仅执行队列（不进行 SBA 检查）
    // 用于特殊场景，如测试或内部调用
    // ==========================================
    static void executeQueueOnly(GameState& state) {
        while (!state.actionQueue.empty() && state.currentPhase == StatePhase::PLAYING_CARD) {
            auto& action = state.actionQueue.front();
            bool completed = action->update(state);
            
            if (completed) {
                state.actionQueue.pop_front();
            }
        }
    }
};
