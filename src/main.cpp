// ==========================================
// 模拟对局主循环 (Simulation Entry)
// ==========================================

#include <iostream>
#include <memory>
#include <random>
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include "src/rules/BasicRules.h"
#include "src/action/PlayerActions.h"
#include "src/card/Cards.h"
#include "src/relic/Relics.h"
#include "src/character/Character.h"
#include "src/core/Types.h"
#include "src/utils/Logger.h"

int main() {
    GameState state;
    
    state.monsters.push_back(std::make_shared<Monster>("史莱姆", 50));

    CombatFlow flow;

    BasicRules::registerRules(state);

    // ==========================================
    // 初始化牌库：往弃牌堆塞入 11 张牌
    // 测试洗牌、抽牌、爆牌逻辑
    // ==========================================
    STS_LOG(state, "\n=== [初始化] 往弃牌堆塞入 11 张牌 ===\n");
    for (int i = 0; i < 6; ++i) {
        state.discardPile.push_back(std::make_shared<StrikeCard>());
    }
    for (int i = 0; i < 3; ++i) {
        state.discardPile.push_back(std::make_shared<DeadlyPoisonCard>());
    }
    for (int i = 0; i < 2; ++i) {
        state.discardPile.push_back(std::make_shared<WhirlwindCard>());
    }
    STS_LOG(state, "[初始化] 弃牌堆: " << state.discardPile.size() << " 张\n");
    STS_LOG(state, "[初始化] 抽牌堆: " << state.drawPile.size() << " 张\n");
    STS_LOG(state, "[初始化] 手牌: " << state.hand.size() << " 张\n");

    // ==========================================
    // 遗物装备示例：演示封装后的 addRelic 接口
    // 自动触发 onEquip 生命周期
    // ==========================================
    STS_LOG(state, "\n=== [遗物装备] 演示 addRelic 接口 ===\n");
    
    // 装备化学物X
    auto chemicalX = std::make_shared<ChemicalXRelic>();
    STS_LOG(state, "[遗物] 准备装备: " << chemicalX->name << "\n");
    state.player->addRelic(chemicalX, state);
    STS_LOG(state, "[遗物] 装备完成，当前遗物数量: " << state.player->getRelicCount() << "\n");
    
    // 尝试重复装备同名遗物（会被跳过）
    auto chemicalX2 = std::make_shared<ChemicalXRelic>();
    STS_LOG(state, "\n[遗物] 尝试重复装备同名遗物...\n");
    state.player->addRelic(chemicalX2, state);
    STS_LOG(state, "[遗物] 当前遗物数量: " << state.player->getRelicCount() << "\n");
    
    // 检查是否拥有遗物
    STS_LOG(state, "\n[遗物] 检查是否拥有 [化学物X]: " 
              << (state.player->hasRelic("化学物X") ? "是" : "否") << "\n");
    STS_LOG(state, "[遗物] 检查是否拥有 [魔改金刚杵]: " 
              << (state.player->hasRelic("魔改金刚杵") ? "是" : "否") << "\n");
    
    STS_LOG(state, "\n");

    int lastTurnCount = 0;

    for (int step = 0; step < 100; ++step) {
        flow.tick(state);
        
        if (state.turnCount != lastTurnCount) {
            lastTurnCount = state.turnCount;
            STS_LOG(state, "\n=== [牌堆状态] 第 " << state.turnCount << " 回合 ===\n");
            STS_LOG(state, "  抽牌堆: " << state.drawPile.size() << " 张\n");
            STS_LOG(state, "  手牌: " << state.hand.size() << " 张\n");
            STS_LOG(state, "  弃牌堆: " << state.discardPile.size() << " 张\n");
            STS_LOG(state, "  消耗堆: " << state.exhaustPile.size() << " 张\n");
            STS_LOG(state, "  滞留区: " << state.limbo.size() << " 张\n");
        }
        
        if (flow.currentState == CombatState::PLAYER_ACTION && 
            state.currentPhase == StatePhase::PLAYING_CARD &&
            state.isActionQueueEmpty()) {
            
            if (state.hand.empty()) {
                STS_LOG(state, "\n--- AI 决策：手牌为空，结束回合 ---\n");
                PlayerActions::endTurn(state, flow);
                continue;
            }
            
            if (state.player->getEnergy() <= 0) {
                STS_LOG(state, "\n--- AI 决策：能量耗尽，结束回合 ---\n");
                PlayerActions::endTurn(state, flow);
                continue;
            }
            
            std::vector<std::shared_ptr<AbstractCard>> playableCards;
            for (auto& card : state.hand) {
                if (card->cost == -1 || card->cost <= state.player->getEnergy()) {
                    playableCards.push_back(card);
                }
            }
            
            if (playableCards.empty()) {
                STS_LOG(state, "\n--- AI 决策：无可用牌，结束回合 ---\n");
                PlayerActions::endTurn(state, flow);
                continue;
            }
            
            std::uniform_int_distribution<int> dist(0, playableCards.size() - 1);
            int cardIndex = dist(state.rng.combatRng);
            auto selectedCard = playableCards[cardIndex];
            
            std::shared_ptr<Character> target = nullptr;
            if (selectedCard->type == CardType::ATTACK || selectedCard->type == CardType::SKILL) {
                std::vector<std::shared_ptr<Monster>> aliveMonsters;
                for (auto& m : state.monsters) {
                    if (!m->isDead()) {
                        aliveMonsters.push_back(m);
                    }
                }
                if (!aliveMonsters.empty()) {
                    std::uniform_int_distribution<int> monsterDist(0, aliveMonsters.size() - 1);
                    target = aliveMonsters[monsterDist(state.rng.combatRng)];
                }
            }
            
            STS_LOG(state, "\n--- AI 决策：打出 [" << selectedCard->id << "] ");
            if (target) {
                STS_LOG(state, "目标: " << target->name);
            }
            STS_LOG(state, " ---\n");
            
            PlayerActions::playCard(state, flow, selectedCard, target);
        }
        
        if (flow.currentState == CombatState::BATTLE_END) {
            flow.tick(state);
            break; 
        }
    }

    STS_LOG(state, "\n=== [最终牌堆状态] ===\n");
    STS_LOG(state, "  抽牌堆: " << state.drawPile.size() << " 张\n");
    STS_LOG(state, "  手牌: " << state.hand.size() << " 张\n");
    STS_LOG(state, "  弃牌堆: " << state.discardPile.size() << " 张\n");
    STS_LOG(state, "  消耗堆: " << state.exhaustPile.size() << " 张\n");

    return 0;
}
