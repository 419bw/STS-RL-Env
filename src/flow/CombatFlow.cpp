#include "CombatFlow.h"
#include "src/gamestate/GameState.h"
#include "src/utils/Logger.h"
#include <iostream>

// ==========================================
// SBA 全局巡视 (State-Based Action Check)
// 
// 在每个动作执行完毕后调用
// 检查全局状态变化，如死亡判定
// ==========================================
void CombatFlow::sbaGlobalCheck(GameState& state) {
    // 1. 玩家死亡判定
    if (!state.isPlayerDead && state.player->isDead()) {
        state.isPlayerDead = true;
        STS_LOG(state, "\n>>> [死亡] 玩家已倒下！ <<<\n");
        ENGINE_TRACE("[SBA] 玩家已死亡！");
    }
    
    // 2. 怪物死亡判定（逐个检查，播报每个怪物的死亡）
    bool allMonstersDead = true;
    for (auto& monster : state.monsters) {
        if (monster->isDead()) {
            // 播报死亡（只播报一次）
            if (!monster->deathReported) {
                STS_LOG(state, "\n>>> [死亡] " << monster->name << " 被击败！ <<<\n");
                monster->deathReported = true;
                ENGINE_TRACE("[SBA] " << monster->name << " 已死亡！");
            }
        } else {
            allMonstersDead = false;
        }
    }
    
    // 3. 更新全局状态
    if (allMonstersDead && !state.isMonsterDead) {
        state.isMonsterDead = true;
    }
    
    // 4. 未来可扩展：其他状态触发
    // - 当生命值低于 X% 时触发遗物
    // - 当手牌数为 0 时触发效果
    // - 当能量为 0 时触发效果
    // ...
}

// ==========================================
// 检查战斗结束条件
// ==========================================
void CombatFlow::checkBattleEndCondition(GameState& state) {
    if (state.isPlayerDead || state.isMonsterDead) {
        currentState = CombatState::BATTLE_END;
    }
}

// ==========================================
// 核心推进函数 (Tick)
// 
// 铁律：CombatFlow 绝对不知道具体的 Action
// 只负责推动时间流逝和发布广播
// ==========================================

void CombatFlow::tick(GameState& state) {
    // 铁律 1：微观层面 - 永远优先结算动作队列
    if (!state.actionQueue.empty()) {
        auto& action = state.actionQueue.front();
        if (action->update(state)) {
            state.actionQueue.pop();
        }
        
        // SBA：动作执行完毕后，进行全局巡视
        sbaGlobalCheck(state);
        
        checkBattleEndCondition(state);
        return; // 队列未清空前，冻结宏观状态机
    }

    // 铁律 2：宏观层面 - 状态跃迁与广播
    // 绝对不出现具体的 Action 类！
    switch (currentState) {
        case CombatState::BATTLE_START:
            STS_LOG(state, "\n=== [PHASE] 战斗开始 ===\n");
            state.eventBus.publish(EventType::PHASE_BATTLE_START, state);
            currentState = CombatState::ROUND_START;
            break;

        case CombatState::ROUND_START:
            state.turnCount++;
            STS_LOG(state, "\n=== [PHASE] 第 " << state.turnCount << " 轮次开始 ===\n");
            state.eventBus.publish(EventType::PHASE_ROUND_START, state);
            currentState = CombatState::PLAYER_TURN_START;
            break;

        case CombatState::PLAYER_TURN_START:
            state.player->energy = 3;
            state.player->block = 0;
            state.isPlayerTurn = true;  // 拨动时间开关：玩家回合开始
            STS_LOG(state, "\n=== [PHASE] 玩家回合开始 ===\n");
            
            state.eventBus.publish(EventType::PHASE_PLAYER_TURN_START, state);
            state.eventBus.publish(EventType::ON_TURN_START, state, state.player.get());
            
            currentState = CombatState::PLAYER_ACTION;
            break;

        case CombatState::PLAYER_ACTION:
            // 引擎在此挂起，等待外部输入 (AI/玩家)
            break;

        case CombatState::PLAYER_TURN_END:
            state.isPlayerTurn = false;  // 拨动时间开关：玩家回合结束
            STS_LOG(state, "\n=== [PHASE] 玩家回合结束 ===\n");
            
            state.eventBus.publish(EventType::PHASE_PLAYER_TURN_END, state);
            state.eventBus.publish(EventType::ON_TURN_END, state, state.player.get());
            
            currentState = CombatState::MONSTER_TURN_START;
            break;

        case CombatState::MONSTER_TURN_START:
            STS_LOG(state, "\n=== [PHASE] 怪物回合开始 ===\n");
            
            state.eventBus.publish(EventType::PHASE_MONSTER_TURN_START, state);
            
            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    state.eventBus.publish(EventType::ON_TURN_START, state, monster.get());
                }
            }
            
            currentState = CombatState::MONSTER_TURN;
            break;

        case CombatState::MONSTER_TURN:
            STS_LOG(state, "\n=== [PHASE] 怪物行动 ===\n");
            
            state.eventBus.publish(EventType::PHASE_MONSTER_TURN, state);
            
            currentState = CombatState::MONSTER_TURN_END;
            break;

        case CombatState::MONSTER_TURN_END:
            STS_LOG(state, "\n=== [PHASE] 怪物回合结束 ===\n");
            
            state.eventBus.publish(EventType::PHASE_MONSTER_TURN_END, state);
            
            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    state.eventBus.publish(EventType::ON_TURN_END, state, monster.get());
                }
            }
            
            currentState = CombatState::ROUND_END;
            break;

        case CombatState::ROUND_END:
            STS_LOG(state, "\n=== [PHASE] 轮次结束 (结算状态效果) ===\n");
            
            state.eventBus.publish(EventType::PHASE_ROUND_END, state);
            state.eventBus.publish(EventType::ON_ROUND_END, state);
            
            // SBA：轮次结束后也要检查状态
            sbaGlobalCheck(state);
            
            checkBattleEndCondition(state);
            if (currentState != CombatState::BATTLE_END) {
                currentState = CombatState::ROUND_START;
            }
            break;

        case CombatState::BATTLE_END:
            STS_LOG(state, "\n=== [PHASE] 战斗结束 ===\n");
            state.eventBus.publish(EventType::PHASE_BATTLE_END, state);
            
            if (state.isPlayerDead) {
                STS_LOG(state, ">>> 游戏失败 (GAME OVER) <<<\n");
            } else if (state.isMonsterDead) {
                STS_LOG(state, ">>> 战斗胜利！结算金币和卡牌奖励... <<<\n");
            }
            break;
    }
}
