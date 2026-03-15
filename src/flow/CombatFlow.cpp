#include "CombatFlow.h"
#include "src/gamestate/GameState.h"
#include <iostream>

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
        checkBattleEndCondition(state);
        return; // 队列未清空前，冻结宏观状态机
    }

    // 铁律 2：宏观层面 - 状态跃迁与广播
    // 绝对不出现具体的 Action 类！
    switch (currentState) {
        case CombatState::BATTLE_START:
            std::cout << "\n=== [PHASE] 战斗开始 ===\n";
            state.eventBus.publish(EventType::PHASE_BATTLE_START, state);
            currentState = CombatState::ROUND_START;
            break;

        case CombatState::ROUND_START:
            state.turnCount++;
            std::cout << "\n=== [PHASE] 第 " << state.turnCount << " 轮次开始 ===\n";
            state.eventBus.publish(EventType::PHASE_ROUND_START, state);
            currentState = CombatState::PLAYER_TURN_START;
            break;

        case CombatState::PLAYER_TURN_START:
            state.player->energy = 3;
            state.player->block = 0;
            state.isPlayerTurn = true;  // 拨动时间开关：玩家回合开始
            std::cout << "\n=== [PHASE] 玩家回合开始 ===\n";
            
            state.eventBus.publish(EventType::PHASE_PLAYER_TURN_START, state);
            state.eventBus.publish(EventType::ON_TURN_START, state, state.player.get());
            
            currentState = CombatState::PLAYER_ACTION;
            break;

        case CombatState::PLAYER_ACTION:
            // 引擎在此挂起，等待外部输入 (AI/玩家)
            break;

        case CombatState::PLAYER_TURN_END:
            state.isPlayerTurn = false;  // 拨动时间开关：玩家回合结束
            std::cout << "\n=== [PHASE] 玩家回合结束 ===\n";
            
            state.eventBus.publish(EventType::PHASE_PLAYER_TURN_END, state);
            state.eventBus.publish(EventType::ON_TURN_END, state, state.player.get());
            
            currentState = CombatState::MONSTER_TURN_START;
            break;

        case CombatState::MONSTER_TURN_START:
            std::cout << "\n=== [PHASE] 怪物回合开始 ===\n";
            
            state.eventBus.publish(EventType::PHASE_MONSTER_TURN_START, state);
            
            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    state.eventBus.publish(EventType::ON_TURN_START, state, monster.get());
                }
            }
            
            currentState = CombatState::MONSTER_TURN;
            break;

        case CombatState::MONSTER_TURN:
            std::cout << "\n=== [PHASE] 怪物行动 ===\n";
            
            state.eventBus.publish(EventType::PHASE_MONSTER_TURN, state);
            
            currentState = CombatState::MONSTER_TURN_END;
            break;

        case CombatState::MONSTER_TURN_END:
            std::cout << "\n=== [PHASE] 怪物回合结束 ===\n";
            
            state.eventBus.publish(EventType::PHASE_MONSTER_TURN_END, state);
            
            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    state.eventBus.publish(EventType::ON_TURN_END, state, monster.get());
                }
            }
            
            currentState = CombatState::ROUND_END;
            break;

        case CombatState::ROUND_END:
            std::cout << "\n=== [PHASE] 轮次结束 (结算状态效果) ===\n";
            
            state.eventBus.publish(EventType::PHASE_ROUND_END, state);
            state.eventBus.publish(EventType::ON_ROUND_END, state);
            
            checkBattleEndCondition(state);
            if (currentState != CombatState::BATTLE_END) {
                currentState = CombatState::ROUND_START;
            }
            break;

        case CombatState::BATTLE_END:
            std::cout << "\n=== [PHASE] 战斗结束 ===\n";
            state.eventBus.publish(EventType::PHASE_BATTLE_END, state);
            
            if (state.isPlayerDead) {
                std::cout << ">>> 游戏失败 (GAME OVER) <<<\n";
            } else if (state.isMonsterDead) {
                std::cout << ">>> 战斗胜利！结算金币和卡牌奖励... <<<\n";
            }
            break;
    }
}
