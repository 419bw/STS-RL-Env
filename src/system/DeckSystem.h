#pragma once

#include "src/gamestate/GameState.h"
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
    // ==========================================
    // 洗牌：将弃牌堆洗入抽牌堆
    // 
    // 核心隔离：必须使用 shuffleRng
    // 严禁使用全局或其他 RNG
    // ==========================================
    static void shuffleDiscardIntoDraw(GameState& state) {
        if (state.discardPile.empty()) {
            return;
        }

        // 将弃牌堆插入抽牌堆
        state.drawPile.insert(
            state.drawPile.end(),
            std::make_move_iterator(state.discardPile.begin()),
            std::make_move_iterator(state.discardPile.end())
        );
        state.discardPile.clear();

        // 核心隔离洗牌：必须使用 shuffleRng
        std::shuffle(
            state.drawPile.begin(),
            state.drawPile.end(),
            state.rng.shuffleRng
        );

        // 发布洗牌事件
        state.eventBus.publish(EventType::ON_SHUFFLE, state);
        STS_LOG(state, "[DeckSystem] 弃牌堆已洗入抽牌堆\n");
    }

    // ==========================================
    // 抽牌：从抽牌堆抽取指定数量的牌
    // 
    // 性能铁律：永远使用 back() + pop_back()
    // 爆牌阻断：手牌 >= 10 时直接停止抽牌，卡牌保留在抽牌堆
    // 防死循环：抽牌堆和弃牌堆均空时停止
    // 
    // ★ 原版行为：抽牌导致的爆牌不会将卡牌移入弃牌堆！
    //    卡牌必须保留在抽牌堆（drawPile）中不被取出
    // ==========================================
    static void drawCards(GameState& state, int amount) {
        for (int i = 0; i < amount; ++i) {
            // ★ 爆牌阻断判定：手牌已满时直接停止抽牌
            // 卡牌保留在抽牌堆中不被取出
            if (state.hand.size() >= 10) {
                STS_LOG(state, "[DeckSystem] 手牌已满，停止抽牌\n");
                break;
            }

            // 防死循环检查：若抽牌堆和弃牌堆均空，停止抽牌
            if (state.drawPile.empty() && state.discardPile.empty()) {
                STS_LOG(state, "[DeckSystem] 抽牌堆和弃牌堆均空，停止抽牌\n");
                break;
            }

            // 洗牌触发：若抽牌堆空且弃牌堆不空
            if (state.drawPile.empty() && !state.discardPile.empty()) {
                shuffleDiscardIntoDraw(state);
                // ★ 防御性检查：遗物可能在 ON_SHUFFLE 事件中抽干了牌堆
                // 例如"每当你洗牌时，立刻抽 1 张牌"的遗物会导致重入
                if (state.drawPile.empty()) {
                    STS_LOG(state, "[DeckSystem] 洗牌后牌堆仍为空，停止抽牌\n");
                    break;
                }
            }

            // 性能铁律：使用 back() + pop_back() (O(1))
            auto card = state.drawPile.back();
            state.drawPile.pop_back();
            
            // 进入手牌
            state.hand.push_back(card);
            state.eventBus.publish(EventType::ON_CARD_DRAWN, state, card.get());
        }
    }

    // ==========================================
    // 移动到弃牌堆
    // ==========================================
    static void moveToDiscard(GameState& state, std::shared_ptr<AbstractCard> card) {
        // 从所有牌堆中查找并移除
        eraseFromAllPiles(state, card);
        
        // 放入弃牌堆
        state.discardPile.push_back(card);
        state.eventBus.publish(EventType::ON_CARD_DISCARDED, state, card.get());
        STS_LOG(state, "[DeckSystem] " << card->id << " 进入弃牌堆\n");
    }

    // ==========================================
    // 移动到消耗堆
    // ==========================================
    static void moveToExhaust(GameState& state, std::shared_ptr<AbstractCard> card) {
        // 从所有牌堆中查找并移除
        eraseFromAllPiles(state, card);
        
        // 放入消耗堆
        state.exhaustPile.push_back(card);
        state.eventBus.publish(EventType::ON_CARD_EXHAUSTED, state, card.get());
        STS_LOG(state, "[DeckSystem] " << card->id << " 进入消耗堆\n");
    }

    // ==========================================
    // 移动到手牌
    // 
    // 爆牌判定：手牌 >= 10 时进入弃牌堆
    // ==========================================
    static void moveToHand(GameState& state, std::shared_ptr<AbstractCard> card) {
        // 从所有牌堆中查找并移除
        eraseFromAllPiles(state, card);
        
        // 爆牌判定
        if (state.hand.size() >= 10) {
            state.discardPile.push_back(card);
            state.eventBus.publish(EventType::ON_CARD_DISCARDED, state, card.get());
            STS_LOG(state, "[DeckSystem] 爆牌！ " << card->id << " 进入弃牌堆\n");
        } else {
            state.hand.push_back(card);
            state.eventBus.publish(EventType::ON_CARD_DRAWN, state, card.get());
            STS_LOG(state, "[DeckSystem] " << card->id << " 进入手牌\n");
        }
    }

    // ==========================================
    // 从滞留区移除卡牌
    // ==========================================
    static void eraseFromLimbo(GameState& state, std::shared_ptr<AbstractCard> card) {
        auto it = std::find(state.limbo.begin(), state.limbo.end(), card);
        if (it != state.limbo.end()) {
            state.limbo.erase(it);
        }
    }

private:
    // ==========================================
    // 从所有牌堆中查找并移除卡牌
    // ==========================================
    static void eraseFromAllPiles(GameState& state, std::shared_ptr<AbstractCard> card) {
        // 从手牌移除
        auto it = std::find(state.hand.begin(), state.hand.end(), card);
        if (it != state.hand.end()) {
            state.hand.erase(it);
            return;
        }

        // 从抽牌堆移除
        it = std::find(state.drawPile.begin(), state.drawPile.end(), card);
        if (it != state.drawPile.end()) {
            state.drawPile.erase(it);
            return;
        }

        // 从弃牌堆移除
        it = std::find(state.discardPile.begin(), state.discardPile.end(), card);
        if (it != state.discardPile.end()) {
            state.discardPile.erase(it);
            return;
        }

        // 从消耗堆移除
        it = std::find(state.exhaustPile.begin(), state.exhaustPile.end(), card);
        if (it != state.exhaustPile.end()) {
            state.exhaustPile.erase(it);
            return;
        }

        // 从滞留区移除
        it = std::find(state.limbo.begin(), state.limbo.end(), card);
        if (it != state.limbo.end()) {
            state.limbo.erase(it);
            return;
        }
    }
};
