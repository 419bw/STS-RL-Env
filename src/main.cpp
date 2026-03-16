// ==========================================
// 模拟对局主循环 (Simulation Entry)
// ==========================================

#include <iostream>
#include <memory>
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
    
    // 多怪物测试：添加三个怪物
    state.monsters.push_back(std::make_shared<Monster>("大嘴花", 30));
    state.monsters.push_back(std::make_shared<Monster>("史莱姆", 50));
    state.monsters.push_back(std::make_shared<Monster>("哥布林", 40));

    CombatFlow flow;

    BasicRules::registerRules(state);

    auto vajra = std::make_shared<CustomVajraRelic>();
    vajra->onEquip(state, state.player.get());
    
    auto chemical_x = std::make_shared<ChemicalXRelic>();
    chemical_x->onEquip(state, state.player.get());

    auto strike = std::make_shared<StrikeCard>();
    auto whirlwind = std::make_shared<WhirlwindCard>();

    bool hasPlayedCard = false;
    bool wantsToEndTurn = false;
    int lastTurnCount = 0;

    for (int step = 0; step < 200; ++step) {
        flow.tick(state);
        
        if (state.turnCount != lastTurnCount) {
            hasPlayedCard = false;
            wantsToEndTurn = false;
            lastTurnCount = state.turnCount;
        }
        
        // 使用 flow.currentState 而不是 state.currentState
        if (flow.currentState == CombatState::PLAYER_ACTION && 
            state.isActionQueueEmpty()) {
            
            if (!hasPlayedCard) {
                if (state.turnCount == 1) {
                    // 第一回合：对第一个怪物打出打击
                    STS_LOG(state, "\n--- AI 决策：对大嘴花打出打击 ---\n");
                    PlayerActions::playCard(state, strike, state.monsters[0]);
                } 
                else if (state.turnCount == 2) {
                    // 第二回合：打出旋风斩（攻击所有怪物）
                    STS_LOG(state, "\n--- AI 决策：打出旋风斩（攻击所有怪物） ---\n");
                    PlayerActions::playCard(state, whirlwind, nullptr);
                }
                else if (state.turnCount == 3) {
                    // 第三回合：对第二个怪物打出打击
                    if (!state.monsters[1]->isDead()) {
                        STS_LOG(state, "\n--- AI 决策：对史莱姆打出打击 ---\n");
                        PlayerActions::playCard(state, strike, state.monsters[1]);
                        STS_LOG(state, "\n--- AI 决策：对哥布林打出打击 ---\n");
                        PlayerActions::playCard(state, strike, state.monsters[2]);
                    } else if (!state.monsters[2]->isDead()) {
                        STS_LOG(state, "\n--- AI 决策：对哥布林打出打击 ---\n");
                        PlayerActions::playCard(state, strike, state.monsters[2]);
                    }
                }
                hasPlayedCard = true;
            }
            else if (!wantsToEndTurn) {
                wantsToEndTurn = true;
            }
            else {
                PlayerActions::endTurn(state, flow);
            }
        }
        
        if (flow.currentState == CombatState::BATTLE_END) {
            flow.tick(state);
            break; 
        }
    }

    return 0;
}
