#pragma once

#include "src/core/ForwardDeclarations.h"
#include "src/card/AbstractCard.h"
#include "src/utils/Logger.h"
#include <algorithm>
#include <random>

// ==========================================
// DeckSystem - 牌库物理马达 (无状态静态类)
//
// ECS/DOD 架构核心组件：
// - 纯静态方法，无状态
// - 负责卡牌物理指针流转
// - 所有随机操作必须使用隔离 RNG
//
// 铁律：
// 1. 永远使用 back() + pop_back() 抽牌 (O(1) 复杂度)
// 2. 洗牌必须使用 shuffleRng (核心隔离)
// 3. 爆牌判定：手牌 >= 10 时进入弃牌堆
// ==========================================

class DeckSystem {
public:
    static void shuffleDiscardIntoDraw(GameEngine& engine);

    static void drawCards(GameEngine& engine, int amount);

    static void moveToDiscard(GameEngine& engine, std::shared_ptr<AbstractCard> card);

    static void moveToExhaust(GameEngine& engine, std::shared_ptr<AbstractCard> card);

    static void moveToHand(GameEngine& engine, std::shared_ptr<AbstractCard> card);

    static void eraseFromLimbo(GameEngine& engine, std::shared_ptr<AbstractCard> card);

    static void discardHand(GameEngine& engine);

private:
    static void eraseFromAllPiles(GameEngine& engine, std::shared_ptr<AbstractCard> card);
};
