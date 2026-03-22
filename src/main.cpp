// ==========================================
// 详细对战逻辑演示 (Detailed Battle Demo)
// ==========================================

#include <iostream>
#include <memory>
#include <random>
#include "src/gamestate/GameState.h"
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
    GameState state(1337);

    // ==========================================
    // 1. 玩家初始化：80 HP, 3 能量, 初始手牌 5 张
    // ==========================================
    STS_LOG(state, "\n=== [初始化] 玩家创建 ===\n");
    STS_LOG(state, "  玩家: " << state.player->name << "\n");
    STS_LOG(state, "  HP: " << state.player->current_hp << "/" << state.player->max_hp << "\n");
    STS_LOG(state, "  能量: " << state.player->getEnergy() << "\n");

    // ==========================================
    // 2. 怪物阵容
    // ==========================================
    STS_LOG(state, "\n=== [初始化] 怪物阵容 ===\n");

    // JawWorm - 有趣的 AI 行为：基于 move history 做决策
    auto jawWorm = std::make_shared<JawWorm>(0);  // ascension 0
    jawWorm->name = "JawWorm";
    state.monsters.push_back(jawWorm);
    STS_LOG(state, "  [AI类型] JawWorm: 基于 move history 做决策\n");
    STS_LOG(state, "    HP: " << jawWorm->current_hp << "/" << jawWorm->max_hp << "\n");
    STS_LOG(state, "    Brain: JawWormBrain (CHOMP/BELLOW/THRASH)\n");

    // Generic Monster 1 - FixedBrain 对照组（固定意图序列）
    auto generic1 = std::make_shared<Monster>("先锋怪", 40);
    generic1->setBrain(std::make_shared<FixedBrain>(std::vector<Intent>{
        {IntentType::ATTACK, 8, 1, 0, nullptr, true, 1, " Strike"},
        {IntentType::DEFEND, 0, 0, 5, nullptr, true, 2, " Guard"},
        {IntentType::ATTACK, 12, 1, 0, nullptr, true, 3, " Heavy Strike"}
    }));
    state.monsters.push_back(generic1);
    STS_LOG(state, "  [AI类型] 先锋怪: FixedBrain (ATTACK→DEFEND→ATTACK 循环)\n");
    STS_LOG(state, "    HP: " << generic1->current_hp << "/" << generic1->max_hp << "\n");

    // Generic Monster 2 - FixedBrain 对照组（不同序列）
    auto generic2 = std::make_shared<Monster>("后卫怪", 35);
    generic2->setBrain(std::make_shared<FixedBrain>(std::vector<Intent>{
        {IntentType::DEFEND, 0, 0, 8, nullptr, true, 1, " Brace"},
        {IntentType::ATTACK, 6, 2, 0, nullptr, true, 2, " Double Strike"},
        {IntentType::ATTACK, 10, 1, 0, nullptr, true, 3, " Slam"}
    }));
    state.monsters.push_back(generic2);
    STS_LOG(state, "  [AI类型] 后卫怪: FixedBrain (DEFEND→ATTACK×2→ATTACK 循环)\n");
    STS_LOG(state, "    HP: " << generic2->current_hp << "/" << generic2->max_hp << "\n");

    // ==========================================
    // 3. 初始化牌库：初始牌组
    // ==========================================
    STS_LOG(state, "\n=== [初始化] 牌库构建 ===\n");

    // 攻击牌：Strike x5
    for (int i = 0; i < 5; ++i) {
        state.discardPile.push_back(std::make_shared<StrikeCard>());
    }
    // 技能牌：Defend x4 (如果存在)
    // 毒素牌：Deadly Poison x3
    for (int i = 0; i < 3; ++i) {
        state.discardPile.push_back(std::make_shared<DeadlyPoisonCard>());
    }
    // AOE牌：Whirlwind x2
    for (int i = 0; i < 2; ++i) {
        state.discardPile.push_back(std::make_shared<WhirlwindCard>());
    }
    // 飞刀牌：Shuriken x3
    for (int i = 0; i < 3; ++i) {
        state.discardPile.push_back(std::make_shared<ShurikenCard>());
    }

    STS_LOG(state, "  弃牌堆: " << state.discardPile.size() << " 张\n");
    STS_LOG(state, "    - Strike: 5 张 (1费 攻击)\n");
    STS_LOG(state, "    - Deadly Poison: 3 张 (1费 中毒)\n");
    STS_LOG(state, "    - Whirlwind: 2 张 (X费 AOE)\n");
    STS_LOG(state, "    - Shuriken: 3 张 (1费 攻击)\n");

    // ==========================================
    // 4. 注册基础规则
    // ==========================================
    CombatFlow flow;
    BasicRules::registerRules(state);
    STS_LOG(state, "\n=== [初始化] 基础规则注册完成 ===\n");

    // ==========================================
    // 5. 战斗主循环
    // ==========================================
    STS_LOG(state, "\n");
    STS_LOG(state, "########################################\n");
    STS_LOG(state, "#         战斗开始！                   #\n");
    STS_LOG(state, "########################################\n");

    int lastTurnCount = 0;
    int maxTurns = 20;  // 防止无限循环

    for (int step = 0; step < maxTurns * 10 && flow.currentState != CombatState::BATTLE_END; ++step) {
        flow.tick(state);

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
        // 玩家回合开始：抽牌日志
        // ==========================================
        if (flow.currentState == CombatState::PLAYER_TURN_START) {
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
        if (flow.currentState == CombatState::MONSTER_TURN_END) {
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
        if (flow.currentState == CombatState::PLAYER_ACTION &&
            state.currentPhase == StatePhase::PLAYING_CARD &&
            state.isActionQueueEmpty()) {

            // 第一次出牌前输出手牌列表
            STS_LOG(state, "  当前手牌:\n");
            for (size_t i = 0; i < state.hand.size(); ++i) {
                auto& card = state.hand[i];
                STS_LOG(state, "    [" << (i + 1) << "] " << card->id
                          << " (费用:" << card->cost << ")\n");
            }

            // 检查手牌
            if (state.hand.empty()) {
                STS_LOG(state, "\n>>> AI 决策：手牌为空，结束回合 <<<\n");
                PlayerActions::endTurn(state, flow);
                continue;
            }

            // 检查能量
            if (state.player->getEnergy() <= 0) {
                STS_LOG(state, "\n>>> AI 决策：能量耗尽，结束回合 <<<\n");
                PlayerActions::endTurn(state, flow);
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
                PlayerActions::endTurn(state, flow);
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
                    PlayerActions::endTurn(state, flow);
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

            PlayerActions::playCard(state, flow, selectedCard, target);
        }

        // ==========================================
        // 战斗结束检测
        // ==========================================
        if (flow.currentState == CombatState::BATTLE_END) {
            break;
        }
    }

    // ==========================================
    // 6. 战斗结果
    // ==========================================
    STS_LOG(state, "\n");
    STS_LOG(state, "########################################\n");
    if (state.isPlayerDead) {
        STS_LOG(state, "#         游戏失败 (GAME OVER)        #\n");
    } else if (state.isMonsterDead) {
        STS_LOG(state, "#         战斗胜利！                   #\n");
    } else {
        STS_LOG(state, "#         战斗超时 (Max Turns)         #\n");
    }
    STS_LOG(state, "########################################\n");

    // ==========================================
    // 7. 最终状态报告
    // ==========================================
    STS_LOG(state, "\n=== [最终状态] ===\n");
    STS_LOG(state, "玩家:\n");
    STS_LOG(state, "  HP: " << state.player->current_hp << "/" << state.player->max_hp << "\n");
    STS_LOG(state, "  护甲: " << state.player->block << "\n");
    STS_LOG(state, "  能量: " << state.player->getEnergy() << "\n");

    STS_LOG(state, "\n怪物:\n");
    for (auto& monster : state.monsters) {
        STS_LOG(state, "  " << monster->name << ": "
                  << (monster->isDead() ? "已击败" : "存活")
                  << " HP:" << monster->current_hp << "/" << monster->max_hp << "\n");
    }

    STS_LOG(state, "\n牌堆:\n");
    STS_LOG(state, "  抽牌堆: " << state.drawPile.size() << " 张\n");
    STS_LOG(state, "  手牌: " << state.hand.size() << " 张\n");
    STS_LOG(state, "  弃牌堆: " << state.discardPile.size() << " 张\n");
    STS_LOG(state, "  消耗堆: " << state.exhaustPile.size() << " 张\n");

    STS_LOG(state, "\n总回合数: " << state.turnCount << "\n");

    return 0;
}