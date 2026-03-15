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
            // 仅仅发布事件，不创建任何具体 Action
            state.eventBus.publish(EventType::PHASE_BATTLE_START, state);
            currentState = CombatState::TURN_START;
            break;

        case CombatState::TURN_START:
            state.turnCount++;
            state.player->energy = 3;
            state.player->block = 0;
            std::cout << "\n=== [PHASE] 第 " << state.turnCount << " 回合开始 ===\n";
            
            // 发布宏观阶段事件
            state.eventBus.publish(EventType::PHASE_TURN_START, state);
            
            // 发布角色级别的回合开始事件 (玩家 + 所有怪物)
            state.eventBus.publish(EventType::ON_TURN_START, state, state.player.get());
            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    state.eventBus.publish(EventType::ON_TURN_START, state, monster.get());
                }
            }
            
            currentState = CombatState::PLAYER_ACTION;
            break;

        case CombatState::PLAYER_ACTION:
            // 引擎在此挂起，等待外部输入 (AI/玩家)
            // 不发布任何事件，只是空转
            break;

        case CombatState::TURN_END:
            std::cout << "\n=== [PHASE] 玩家回合结束 ===\n";
            
            // 发布宏观阶段事件
            state.eventBus.publish(EventType::PHASE_TURN_END, state);
            
            // 发布角色级别的回合结束事件
            state.eventBus.publish(EventType::ON_TURN_END, state, state.player.get());
            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    state.eventBus.publish(EventType::ON_TURN_END, state, monster.get());
                }
            }
            
            currentState = CombatState::MONSTER_TURN_START;
            break;

        case CombatState::MONSTER_TURN_START:
            std::cout << "\n=== [PHASE] 怪物回合开始 ===\n";
            
            // 发布宏观阶段事件
            state.eventBus.publish(EventType::PHASE_MONSTER_TURN_START, state);
            
            // 发布怪物回合开始事件 (所有怪物)
            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    state.eventBus.publish(EventType::ON_TURN_START, state, monster.get());
                }
            }
            
            currentState = CombatState::MONSTER_TURN;
            break;

        case CombatState::MONSTER_TURN:
            std::cout << "\n=== [PHASE] 怪物行动 ===\n";
            
            // 发布宏观阶段事件
            state.eventBus.publish(EventType::PHASE_MONSTER_TURN, state);
            
            currentState = CombatState::MONSTER_TURN_END;
            break;

        case CombatState::MONSTER_TURN_END:
            std::cout << "\n=== [PHASE] 怪物回合结束 ===\n";
            
            // 发布宏观阶段事件
            state.eventBus.publish(EventType::PHASE_MONSTER_TURN_END, state);
            
            // 发布怪物回合结束事件 (所有怪物)
            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    state.eventBus.publish(EventType::ON_TURN_END, state, monster.get());
                }
            }
            
            currentState = CombatState::TURN_START;
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
