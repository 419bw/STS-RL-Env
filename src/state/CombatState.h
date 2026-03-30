#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <random>
#include "src/core/Types.h"
#include "src/core/RandomManager.h"
#include "src/character/Character.h"
#include "src/card/AbstractCard.h"

// ==========================================
// CombatState - 易失层（单场战斗）
//
// 生命周期：一场战斗（战斗结束/死亡/逃跑后销毁）
//
// 设计原则：
// - 不再继承 GameState，成为独立数据类
// - player 是引用而非拷贝，HP 直接修改 RunState
// - 卡牌从 masterDeck 深拷贝而来
// - EventBus 在 GameEngine 层（全局），不在 CombatState 层
// ==========================================

class CombatState {
public:
    // ==========================================
    // 日志开关
    // ==========================================
    bool enableLogging = true;

    // ==========================================
    // 回合计数
    // ==========================================
    int turnCount = 0;
    bool isPlayerDead = false;
    bool isMonsterDead = false;

    // ==========================================
    // 宏观时间开关：记录当前是玩家回合还是怪物回合
    // ==========================================
    bool isPlayerTurn = false;

    // ==========================================
    // 游戏实体
    // ==========================================
    std::shared_ptr<Player> player;
    std::vector<std::shared_ptr<Monster>> monsters;

    // ==========================================
    // 战斗 RNG（与 RunState.rng 隔离）
    // ==========================================
    RandomManager combatRng;

    // ==========================================
    // 牌库系统 (5 个物理牌堆)
    //
    // drawPile:    抽牌堆
    // hand:        手牌
    // discardPile: 弃牌堆
    // exhaustPile: 消耗堆
    // limbo:       滞留区
    // ==========================================
    std::vector<std::shared_ptr<AbstractCard>> drawPile;
    std::vector<std::shared_ptr<AbstractCard>> hand;
    std::vector<std::shared_ptr<AbstractCard>> discardPile;
    std::vector<std::shared_ptr<AbstractCard>> exhaustPile;
    std::vector<std::shared_ptr<AbstractCard>> limbo;
    std::vector<std::shared_ptr<AbstractPotion>> potions;

    // ==========================================
    // 玩家行动子状态
    // ==========================================
    StatePhase currentPhase = StatePhase::PLAYING_CARD;

    // ==========================================
    // 选牌上下文（可选）
    // ==========================================
    std::optional<CardSelectionContext> selectionCtx = std::nullopt;

    // ==========================================
    // 构造函数
    // ==========================================
    CombatState(unsigned int seed = 1337);

    // ==========================================
    // 工厂方法：从 RunState 创建 CombatState
    // ==========================================
    static std::unique_ptr<CombatState> createFromRun(
        RunState& runState,
        std::shared_ptr<Monster> monster,
        unsigned int seed);

    // ==========================================
    // 战斗结束：清理临时状态
    // ==========================================
    void cleanup();

private:
    // 深拷贝卡牌（关键！）
    static std::vector<std::shared_ptr<AbstractCard>> deepCopyDeck(
        const std::vector<std::shared_ptr<AbstractCard>>& masterDeck);
};
