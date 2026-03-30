#include "src/state/CombatState.h"
#include "src/action/AbstractAction.h"
#include "src/core/RandomManager.h"
#include "src/utils/Logger.h"
#include "src/state/RunState.h"
#include <algorithm>

// ==========================================
// CombatState 构造函数
// ==========================================
CombatState::CombatState(unsigned int seed)
    : combatRng(seed) {
}

// ==========================================
// cleanup - 战斗结束清理
//
// 清理所有战斗临时状态：
// - 重置玩家 block
// - 清空所有牌堆
// - 重置战斗状态
//
// 注意：
// - HP 已在战斗中被直接修改，无需重置
// - 不清理 player 引用（指向 RunState）
// - powers 在 CombatState 销毁时自动清理
// ==========================================
void CombatState::cleanup() {
    player->block = 0;

    hand.clear();
    discardPile.clear();
    exhaustPile.clear();
    drawPile.clear();
    limbo.clear();
    monsters.clear();
    currentPhase = StatePhase::BATTLE_START;
    isPlayerDead = false;
    isMonsterDead = false;
    turnCount = 0;
    isPlayerTurn = false;
}

// ==========================================
// createFromRun - 从 RunState 创建 CombatState
//
// 战斗初始化协议：
// 1. 创建 CombatState，初始化战斗 RNG
// 2. 玩家引用指向 RunState.player（不拷贝！）
// 3. 添加怪物
// 4. 深拷贝 masterDeck 到 discardPile
//
// 关键设计：
// - player 是引用而非拷贝，HP 直接写回 RunState
// - 卡牌必须深拷贝，防止战斗修改污染 masterDeck
// ==========================================
std::unique_ptr<CombatState> CombatState::createFromRun(
    RunState& runState,
    std::shared_ptr<Monster> monster,
    unsigned int seed) {

    auto combat = std::make_unique<CombatState>(seed);

    // 1. 玩家引用（不拷贝！）
    combat->player = runState.player;

    // 2. 怪物
    combat->monsters.push_back(monster);

    // 3. 深拷贝 masterDeck → 弃牌堆
    combat->discardPile = deepCopyDeck(runState.masterDeck);

    return combat;
}

// ==========================================
// deepCopyDeck - 深拷贝卡牌
//
// 使用 CloneableCard::clone() 进行深拷贝
// 确保每张卡牌都是独立的副本
// ==========================================
std::vector<std::shared_ptr<AbstractCard>> CombatState::deepCopyDeck(
    const std::vector<std::shared_ptr<AbstractCard>>& masterDeck) {
    std::vector<std::shared_ptr<AbstractCard>> result;
    result.reserve(masterDeck.size());
    for (const auto& card : masterDeck) {
        result.push_back(card->clone());
    }
    return result;
}
