#pragma once

#include "src/core/ForwardDeclarations.h"
#include "src/flow/CombatFlow.h"
#include <memory>

// ==========================================
// PlayerActions - 玩家动作处理器
// 
// 处理玩家在 PLAYER_ACTION 阶段可以执行的操作
// 这些函数会检查状态合法性，并修改 GameState
// ==========================================

class PlayerActions {
public:
    // 打出卡牌
    static bool playCard(GameState& state, 
                         std::shared_ptr<AbstractCard> card,
                         std::shared_ptr<Character> target);
    
    // 结束回合
    static void endTurn(GameState& state, CombatFlow& flow);
};
