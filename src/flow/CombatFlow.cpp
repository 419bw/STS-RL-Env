#include "CombatFlow.h"
#include "src/engine/GameEngine.h"
#include "src/action/ActionManager.h"
#include "src/utils/Logger.h"
#include <iostream>

CombatFlow::CombatFlow() : currentPhase(BattlePhase::BATTLE_START) {}

// ==========================================
// SBA 全局巡视 (State-Based Action Check)
//
// 在每个动作执行完毕后调用
// 检查全局状态变化，如死亡判定
// ==========================================
void CombatFlow::sbaGlobalCheck(GameEngine& engine) {
    if (!engine.combatState) return;

    auto& state = *engine.combatState;
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
                // 清理死亡怪物身上的所有 power
                monster->clearPowers(engine);

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
void CombatFlow::checkBattleEndCondition(GameEngine& engine) {
    if (!engine.combatState) return;

    if (engine.combatState->isPlayerDead || engine.combatState->isMonsterDead) {
        currentPhase = BattlePhase::BATTLE_END;
    }
}

// ==========================================
// 核心推进函数 (Tick)
//
// 铁律：CombatFlow 绝对不知道具体的 Action
// 只负责推动时间流逝和发布广播
//
// 铁律：所有动作队列执行由 ActionManager 统一负责
// CombatFlow 只负责状态跃迁和事件发布
//
// 注意：使用自己的 currentPhase 状态，不会干扰 combatState->currentPhase
// ==========================================

void CombatFlow::tick(GameEngine& engine) {
    // 宏观层面 - 状态跃迁与广播
    // 绝对不出现具体的 Action 类！
    if (!engine.combatState) return;

    auto& state = *engine.combatState;

    switch (currentPhase) {
        case BattlePhase::BATTLE_START:
            STS_LOG(state, "\n=== [PHASE] 战斗开始 ===\n");
            engine.eventBus.publish(EventType::PHASE_BATTLE_START, engine);
            engine.actionManager.executeUntilBlocked(engine, *this);
            currentPhase = BattlePhase::ROUND_START;
            break;

        case BattlePhase::ROUND_START:
            state.turnCount++;
            STS_LOG(state, "\n=== [PHASE] 第 " << state.turnCount << " 轮次开始 ===\n");
            engine.eventBus.publish(EventType::PHASE_ROUND_START, engine);
            engine.actionManager.executeUntilBlocked(engine, *this);
            currentPhase = BattlePhase::PLAYER_TURN_START;
            break;

        case BattlePhase::PLAYER_TURN_START:
            state.player->resetEnergy(3);
            state.player->block = 0;
            state.isPlayerTurn = true;
            state.currentPhase = StatePhase::PLAYING_CARD;
            STS_LOG(state, "\n=== [PHASE] 玩家回合开始 ===\n");

            engine.eventBus.publish(EventType::PHASE_PLAYER_TURN_START, engine);
            engine.eventBus.publish(EventType::ON_TURN_START, engine, state.player.get());
            engine.actionManager.executeUntilBlocked(engine, *this);

            currentPhase = BattlePhase::PLAYER_ACTION;
            break;

        case BattlePhase::PLAYER_ACTION:
            break;

        case BattlePhase::PLAYER_TURN_END:
            state.isPlayerTurn = false;
            STS_LOG(state, "\n=== [PHASE] 玩家回合结束 ===\n");

            engine.eventBus.publish(EventType::PHASE_PLAYER_TURN_END, engine);
            engine.eventBus.publish(EventType::ON_TURN_END, engine, state.player.get());
            engine.actionManager.executeUntilBlocked(engine, *this);

            currentPhase = BattlePhase::MONSTER_TURN_START;
            break;

        case BattlePhase::MONSTER_TURN_START:
            STS_LOG(state, "\n=== [PHASE] 怪物回合开始 ===\n");

            engine.eventBus.publish(EventType::PHASE_MONSTER_TURN_START, engine);

            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    engine.eventBus.publish(EventType::ON_TURN_START, engine, monster.get());
                }
            }
            engine.actionManager.executeUntilBlocked(engine, *this);

            currentPhase = BattlePhase::MONSTER_TURN;
            break;

        case BattlePhase::MONSTER_TURN:
            STS_LOG(state, "\n=== [PHASE] 怪物行动 ===\n");

            engine.eventBus.publish(EventType::PHASE_MONSTER_TURN, engine);
            engine.actionManager.executeUntilBlocked(engine, *this);

            currentPhase = BattlePhase::MONSTER_TURN_END;
            break;

        case BattlePhase::MONSTER_TURN_END:
            STS_LOG(state, "\n=== [PHASE] 怪物回合结束 ===\n");

            engine.eventBus.publish(EventType::PHASE_MONSTER_TURN_END, engine);

            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    engine.eventBus.publish(EventType::ON_TURN_END, engine, monster.get());
                }
            }
            engine.actionManager.executeUntilBlocked(engine, *this);
            currentPhase = BattlePhase::ROUND_END;
            break;

        case BattlePhase::ROUND_END:
            STS_LOG(state, "\n=== [PHASE] 轮次结束 (结算状态效果) ===\n");

            engine.eventBus.publish(EventType::PHASE_ROUND_END, engine);
            engine.eventBus.publish(EventType::ON_ROUND_END, engine);
            engine.actionManager.executeUntilBlocked(engine, *this);

            sbaGlobalCheck(engine);
            checkBattleEndCondition(engine);
            if (currentPhase != BattlePhase::BATTLE_END) {
                currentPhase = BattlePhase::ROUND_START;
            }
            break;

        case BattlePhase::BATTLE_END:
            STS_LOG(state, "\n=== [PHASE] 战斗结束 ===\n");
            engine.eventBus.publish(EventType::PHASE_BATTLE_END, engine);

            if (state.isPlayerDead) {
                STS_LOG(state, ">>> 游戏失败 (GAME OVER) <<<\n");
            } else if (state.isMonsterDead) {
                STS_LOG(state, ">>> 战斗胜利！结算金币和卡牌奖励... <<<\n");
            }
            break;

        default:
            break;
    }
}
