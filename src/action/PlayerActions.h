#pragma once

#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include <memory>
#include <vector>

// ==========================================
// PlayerActions - 玩家动作处理器
//
// 处理玩家在 PLAYER_ACTION 阶段可以执行的操作
// 这些函数会检查状态合法性，并修改 GameEngine
//
// 接口分层：
// - playCard(): 打出卡牌（自由出牌阶段）
// - chooseCard(): 选单张牌（等待选牌阶段，兼容接口）
// - chooseCards(): 选多张牌（等待选牌阶段，批量接口）
// - endTurn(): 结束回合
//
// 铁律：所有动作执行后调用 ActionManager::executeUntilBlocked
// ==========================================

class PlayerActions {
public:
    // 打出卡牌
    // 只在 currentPhase == PLAYING_CARD 时可用
    // 需要传入 GameEngine 和 CombatFlow 以访问完整上下文
    static bool playCard(GameEngine& engine, CombatFlow& flow,
                         std::shared_ptr<AbstractCard> card,
                         std::shared_ptr<Character> target);

    // 选单张牌（兼容接口）
    // 只在 currentPhase == WAITING_FOR_CARD_SELECTION 时可用
    // 根据 Purpose 自动路由到正确的物理结算动作
    static void chooseCard(GameEngine& engine, CombatFlow& flow, int choiceIndex);

    // 选多张牌（批量接口）
    // 只在 currentPhase == WAITING_FOR_CARD_SELECTION 时可用
    // 接收数组，批量路由到正确的物理结算动作
    // 倒序推入队列，保证原本的先后顺序
    static void chooseCards(GameEngine& engine, CombatFlow& flow, const std::vector<int>& choiceIndices);

    // 结束回合
    static void endTurn(GameEngine& engine, CombatFlow& flow);

    // 使用药水
    // 只在 currentPhase == PLAYING_CARD && isPlayerTurn == true 时可用
    // 注意：药水从 runState.potions 中全局删除
    static bool usePotion(GameEngine& engine, CombatFlow& flow,
                         std::shared_ptr<AbstractPotion> potion,
                         std::shared_ptr<Character> target = nullptr);
};
