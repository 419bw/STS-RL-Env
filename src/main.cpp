// ==========================================
// 战斗演示 (Battle Demo)
// 使用三层架构：GameEngine + RunState + CombatState
// ==========================================

#include <iostream>
#include <memory>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/rules/BasicRules.h"
#include "src/action/PlayerActions.h"
#include "src/card/Cards.h"
#include "src/character/Character.h"
#include "src/character/monster/JawWorm.h"
#include "src/intent/FixedBrain.h"
#include "src/core/Types.h"
#include "src/utils/Logger.h"

int main() {
    // ==========================================
    // 创建游戏引擎
    // ==========================================
    GameEngine engine;
    engine.startNewRun(1337);

    auto& runState = engine.runState;
    STS_LOG(*runState, "\n=== [初始化] 游戏引擎创建 ===\n");
    STS_LOG(*runState, "  玩家: " << runState->player->name << "\n");
    STS_LOG(*runState, "  HP: " << runState->player->current_hp << "/" << runState->player->max_hp << "\n");

    // ==========================================
    // 创建战斗
    // ==========================================
    STS_LOG(*runState, "\n=== [初始化] 怪物阵容 ===\n");

    // JawWorm - 有趣的 AI 行为：基于 move history 做决策
    auto jawWorm = std::make_shared<JawWorm>(0);
    jawWorm->name = "JawWorm";
    STS_LOG(*runState, "  [AI类型] JawWorm: 基于 move history 做决策\n");
    STS_LOG(*runState, "    HP: " << jawWorm->current_hp << "/" << jawWorm->max_hp << "\n");

    // Generic Monster - FixedBrain 对照组
    auto generic1 = std::make_shared<Monster>("先锋怪", 40);
    generic1->setBrain(std::make_shared<FixedBrain>(std::vector<Intent>{
        Intent(IntentType::ATTACK, 8, 1, 0, {}).withMove(1, "Strike"),
        Intent(IntentType::DEFEND, 0, 0, 5, {}).withMove(2, "Guard"),
        Intent(IntentType::ATTACK, 12, 1, 0, {}).withMove(3, "Heavy Strike")
    }));
    STS_LOG(*runState, "  [AI类型] 先锋怪: FixedBrain (ATTACK→DEFEND→ATTACK 循环)\n");
    STS_LOG(*runState, "    HP: " << generic1->current_hp << "/" << generic1->max_hp << "\n");

    // Generic Monster 2
    auto generic2 = std::make_shared<Monster>("后卫怪", 35);
    generic2->setBrain(std::make_shared<FixedBrain>(std::vector<Intent>{
        Intent(IntentType::DEFEND, 0, 0, 8, {}).withMove(1, "Brace"),
        Intent(IntentType::ATTACK, 6, 2, 0, {}).withMove(2, "Double Strike"),
        Intent(IntentType::ATTACK, 10, 1, 0, {}).withMove(3, "Slam")
    }));
    STS_LOG(*runState, "  [AI类型] 后卫怪: FixedBrain (DEFEND→ATTACK×2→ATTACK 循环)\n");
    STS_LOG(*runState, "    HP: " << generic2->current_hp << "/" << generic2->max_hp << "\n");

    // ==========================================
    // 初始化牌库
    // ==========================================
    STS_LOG(*runState, "\n=== [初始化] 牌库构建 ===\n");

    // 攻击牌：Strike x5
    for (int i = 0; i < 5; ++i) {
        runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    }
    // 毒素牌：Deadly Poison x3
    for (int i = 0; i < 3; ++i) {
        runState->masterDeck.push_back(std::make_shared<DeadlyPoisonCard>());
    }
    // AOE牌：Whirlwind x2
    for (int i = 0; i < 2; ++i) {
        runState->masterDeck.push_back(std::make_shared<WhirlwindCard>());
    }
    // 飞刀牌：Shuriken x3
    for (int i = 0; i < 3; ++i) {
        runState->masterDeck.push_back(std::make_shared<ShurikenCard>());
    }

    STS_LOG(*runState, "  总卡组: " << runState->masterDeck.size() << " 张\n");
    STS_LOG(*runState, "    - Strike: 5 张 (1费 攻击)\n");
    STS_LOG(*runState, "    - Deadly Poison: 3 张 (1费 中毒)\n");
    STS_LOG(*runState, "    - Whirlwind: 2 张 (X费 AOE)\n");
    STS_LOG(*runState, "    - Shuriken: 3 张 (1费 攻击)\n");

    // ==========================================
    // 进入战斗
    // ==========================================
    engine.startCombat(jawWorm);
    // 手动添加其他怪物
    engine.combatState->monsters.push_back(generic1);
    engine.combatState->monsters.push_back(generic2);

    // 注册基础规则
    BasicRules::registerRules(engine);
    STS_LOG(*runState, "\n=== [初始化] 基础规则注册完成 ===\n");

    // ==========================================
    // 战斗主循环
    // ==========================================
    STS_LOG(*runState, "\n");
    STS_LOG(*runState, "########################################\n");
    STS_LOG(*runState, "#         战斗开始！                   #\n");
    STS_LOG(*runState, "########################################\n");

    int lastTurnCount = 0;
    int maxTurns = 20;
    CombatFlow flow;

    while (engine.combatState &&
           flow.getCurrentPhase() != BattlePhase::BATTLE_END &&
           engine.combatState->turnCount < maxTurns) {

        flow.tick(engine);

        auto& state = *engine.combatState;

        // ==========================================
        // 回合开始：输出怪物意图
        // ==========================================
        if (state.turnCount != lastTurnCount) {
            lastTurnCount = state.turnCount;
            STS_LOG(state, "\n");
            STS_LOG(state, "========================================\n");
            STS_LOG(state, "回合 " << state.turnCount << " - 怪物意图刷新\n");
            STS_LOG(state, "========================================\n");

            for (auto& monster : state.monsters) {
                if (!monster->isDead()) {
                    auto intent = monster->getRealIntent();
                    STS_LOG(state, "  " << monster->name << ": "
                              << intentTypeToString(intent.type)
                              << " (伤害:" << intent.base_damage
                              << " 次:" << intent.hit_count
                              << " 效果值:" << intent.effect_value << ")\n");
                }
            }
        }

        // ==========================================
        // 玩家回合开始：抽牌完成后在手牌阶段输出
        // ==========================================
        if (flow.getCurrentPhase() == BattlePhase::PLAYER_ACTION) {
            STS_LOG(state, "\n");
            STS_LOG(state, "----------------------------------------\n");
            STS_LOG(state, "玩家回合开始 - 当前状态:\n");
            STS_LOG(state, "  HP: " << state.player->current_hp << "/" << state.player->max_hp << "\n");
            STS_LOG(state, "  能量: " << state.player->getEnergy() << "\n");
            STS_LOG(state, "  护甲: " << state.player->block << "\n");
            STS_LOG(state, "  手牌列表:\n");
            for (size_t i = 0; i < state.hand.size(); ++i) {
                auto& card = state.hand[i];
                STS_LOG(state, "    [" << (i + 1) << "] " << card->id
                          << " (费用:" << card->cost
                          << ", 类型:" << (card->type == CardType::ATTACK ? "攻击" :
                                         card->type == CardType::SKILL ? "技能" :
                                         card->type == CardType::POWER ? "能力" : "其他") << ")\n");
            }
        }

        // ==========================================
        // 回合结束：输出牌堆状态
        // ==========================================
        if (flow.getCurrentPhase() == BattlePhase::MONSTER_TURN_END) {
            STS_LOG(state, "\n");
            STS_LOG(state, "----------------------------------------\n");
            STS_LOG(state, "回合结束 - 牌堆状态:\n");
            STS_LOG(state, "  抽牌堆: " << state.drawPile.size() << " 张\n");
            STS_LOG(state, "  手牌: " << state.hand.size() << " 张\n");
            STS_LOG(state, "  弃牌堆: " << state.discardPile.size() << " 张\n");
            STS_LOG(state, "  消耗堆: " << state.exhaustPile.size() << " 张\n");
            STS_LOG(state, "----------------------------------------\n");
        }

        // ==========================================
        // 玩家行动阶段：简单 AI 出牌逻辑
        // ==========================================
        if (flow.getCurrentPhase() == BattlePhase::PLAYER_ACTION &&
            engine.actionManager.isQueueEmpty()) {

            // 检查手牌
            if (state.hand.empty()) {
                STS_LOG(state, "\n>>> AI 决策：手牌为空，结束回合 <<<\n");
                PlayerActions::endTurn(engine, flow);
                continue;
            }

            // 检查能量
            if (state.player->getEnergy() <= 0) {
                STS_LOG(state, "\n>>> AI 决策：能量耗尽，结束回合 <<<\n");
                PlayerActions::endTurn(engine, flow);
                continue;
            }

            // ==========================================
            // AI 出牌逻辑：优先打 Attack 类型牌
            // ==========================================
            std::shared_ptr<AbstractCard> selectedCard = nullptr;
            std::shared_ptr<Character> target = nullptr;

            // 第一遍：查找 Attack 类型牌
            for (auto& card : state.hand) {
                if (card->type == CardType::ATTACK &&
                    (card->cost == -1 || card->cost <= state.player->getEnergy())) {
                    selectedCard = card;
                    break;
                }
            }

            // 第二遍：如果没有 Attack 牌，查找任意可用牌
            if (!selectedCard) {
                for (auto& card : state.hand) {
                    if (card->cost == -1 || card->cost <= state.player->getEnergy()) {
                        selectedCard = card;
                        break;
                    }
                }
            }

            // 无可用牌
            if (!selectedCard) {
                STS_LOG(state, "\n>>> AI 决策：无可用牌，结束回合 <<<\n");
                PlayerActions::endTurn(engine, flow);
                continue;
            }

            // 选择目标
            if (selectedCard->type == CardType::ATTACK || selectedCard->type == CardType::SKILL) {
                std::vector<std::shared_ptr<Monster>> aliveMonsters;
                for (auto& m : state.monsters) {
                    if (!m->isDead()) {
                        aliveMonsters.push_back(m);
                    }
                }
                if (!aliveMonsters.empty()) {
                    // 优先选择最低血量的怪物
                    target = aliveMonsters[0];
                    for (auto& m : aliveMonsters) {
                        if (m->current_hp < target->current_hp) {
                            target = m;
                        }
                    }
                } else {
                    STS_LOG(state, "[警告] 没有存活怪物，停止出牌\n");
                    PlayerActions::endTurn(engine, flow);
                    continue;
                }
            }

            // ==========================================
            // 出牌日志
            // ==========================================
            STS_LOG(state, "\n");
            STS_LOG(state, ">>> AI 出牌 <<<\n");
            STS_LOG(state, "    卡牌: [" << selectedCard->id << "]"
                      << " 费用:" << selectedCard->cost
                      << " 类型:" << (selectedCard->type == CardType::ATTACK ? "攻击" :
                                    selectedCard->type == CardType::SKILL ? "技能" : "能力") << "\n");
            if (target) {
                STS_LOG(state, "    目标: " << target->name
                          << " (HP: " << target->current_hp << "/" << target->max_hp << ")\n");
            }
            STS_LOG(state, "    剩余能量: " << state.player->getEnergy() << "\n");

            PlayerActions::playCard(engine, flow, selectedCard, target);
        }

        // ==========================================
        // 战斗结束检测
        // ==========================================
        if (!engine.combatState ||
            flow.getCurrentPhase() == BattlePhase::BATTLE_END) {
            break;
        }
    }

    // ==========================================
    // 战斗结果
    // ==========================================
    if (engine.combatState) {
        STS_LOG(*engine.combatState, "\n");
        STS_LOG(*engine.combatState, "########################################\n");
        if (engine.combatState->isPlayerDead) {
            STS_LOG(*engine.combatState, "#         游戏失败 (GAME OVER)        #\n");
        } else if (engine.combatState->isMonsterDead) {
            STS_LOG(*engine.combatState, "#         战斗胜利！                   #\n");
        } else {
            STS_LOG(*engine.combatState, "#         战斗超时 (Max Turns)         #\n");
        }
        STS_LOG(*engine.combatState, "########################################\n");

        // ==========================================
        // 最终状态报告
        // ==========================================
        STS_LOG(*engine.combatState, "\n=== [最终状态] ===\n");
        STS_LOG(*engine.combatState, "玩家:\n");
        STS_LOG(*engine.combatState, "  HP: " << engine.combatState->player->current_hp << "/"
                  << engine.combatState->player->max_hp << "\n");
        STS_LOG(*engine.combatState, "  护甲: " << engine.combatState->player->block << "\n");
        STS_LOG(*engine.combatState, "  能量: " << engine.combatState->player->getEnergy() << "\n");

        STS_LOG(*engine.combatState, "\n怪物:\n");
        for (auto& monster : engine.combatState->monsters) {
            STS_LOG(*engine.combatState, "  " << monster->name << ": "
                      << (monster->isDead() ? "已击败" : "存活")
                      << " HP:" << monster->current_hp << "/" << monster->max_hp << "\n");
        }

        STS_LOG(*engine.combatState, "\n牌堆:\n");
        STS_LOG(*engine.combatState, "  抽牌堆: " << engine.combatState->drawPile.size() << " 张\n");
        STS_LOG(*engine.combatState, "  手牌: " << engine.combatState->hand.size() << " 张\n");
        STS_LOG(*engine.combatState, "  弃牌堆: " << engine.combatState->discardPile.size() << " 张\n");
        STS_LOG(*engine.combatState, "  消耗堆: " << engine.combatState->exhaustPile.size() << " 张\n");

        STS_LOG(*engine.combatState, "\n总回合数: " << engine.combatState->turnCount << "\n");
    }

    return 0;
}
