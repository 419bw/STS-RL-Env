#include "PlayerActions.h"
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include <iostream>

// ==========================================
// 玩家动作实现
// ==========================================

bool PlayerActions::playCard(GameState& state, 
                              std::shared_ptr<AbstractCard> card,
                              std::shared_ptr<Character> target) {
    // 注意：状态检查需要使用外部传入的 flow.currentState
    // 这里简化处理，只检查队列是否为空
    if (!state.actionQueue.empty()) {
        std::cout << "[警告] 动作队列未清空，无法出牌！\n";
        return false;
    }

    // 处理 X 费牌
    if (card->cost == -1) {
        card->energyOnUse = state.player->energy;
        state.player->energy = 0;
    } 
    // 处理普通牌
    else if (state.player->energy >= card->cost) {
        state.player->energy -= card->cost;
    } 
    // 费用不足
    else {
        std::cout << "[系统拦截] 费用不足，无法打出 " << card->id << "!\n";
        return false;
    }

    // 发布"准备打出"事件
    state.eventBus.publish(EventType::ON_CARD_PLAYING, state, card.get());

    // 调用卡牌逻辑 (塞入动作队列)
    card->use(state, target);

    // 统一发布"打出后"事件
    state.eventBus.publish(EventType::ON_CARD_PLAYED, state, card.get());

    return true;
}

void PlayerActions::endTurn(GameState& state, CombatFlow& flow) {
    if (flow.currentState == CombatState::PLAYER_ACTION && state.actionQueue.empty()) {
        std::cout << "[外部输入] -> 玩家点击了【结束回合】\n";
        flow.currentState = CombatState::PLAYER_TURN_END;
    } else {
        std::cout << "[警告] 当前状态不允许结束回合，或动作尚未结算完毕！\n";
    }
}
