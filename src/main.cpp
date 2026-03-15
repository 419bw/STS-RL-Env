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

int main() {
    GameState state;
    state.monsters.push_back(std::make_shared<Monster>("大嘴花", 40));

    CombatFlow flow;

    BasicRules::registerRules(state);

    auto vajra = std::make_shared<CustomVajraRelic>();
    vajra->onEquip(state);
    
    auto chemical_x = std::make_shared<ChemicalXRelic>();
    chemical_x->onEquip(state);

    auto strike = std::make_shared<StrikeCard>();
    auto whirlwind = std::make_shared<WhirlwindCard>();

    bool hasPlayedCard = false;
    bool wantsToEndTurn = false;
    int lastTurnCount = 0;

    for (int step = 0; step < 100; ++step) {
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
                    std::cout << "\n--- AI 决策：打出打击 ---\n";
                    PlayerActions::playCard(state, strike, state.monsters[0]);
                } 
                else if (state.turnCount == 2) {
                    std::cout << "\n--- AI 决策：打出旋风斩 ---\n";
                    PlayerActions::playCard(state, whirlwind, nullptr);
                    state.isMonsterDead = true;
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
