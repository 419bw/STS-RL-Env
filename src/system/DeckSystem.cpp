#include "src/system/DeckSystem.h"
#include "src/engine/GameEngine.h"
#include <algorithm>
#include <random>

// ==========================================
// shuffleDiscardIntoDraw - 洗牌并入抽牌堆
//
// 将弃牌堆全部移入抽牌堆，然后洗牌
// 发布 ON_SHUFFLE 事件
// ==========================================
void DeckSystem::shuffleDiscardIntoDraw(GameEngine& engine) {
    auto& combat = *engine.combatState;
    if (combat.discardPile.empty()) {
        return;
    }

    combat.drawPile.insert(
        combat.drawPile.end(),
        std::make_move_iterator(combat.discardPile.begin()),
        std::make_move_iterator(combat.discardPile.end())
    );
    combat.discardPile.clear();

    std::shuffle(
        combat.drawPile.begin(),
        combat.drawPile.end(),
        combat.combatRng.shuffleRng
    );

    engine.eventBus.publish(EventType::ON_SHUFFLE, engine);
    STS_LOG(combat, "[DeckSystem] 弃牌堆已洗入抽牌堆\n");
}

// ==========================================
// drawCards - 抽牌
//
// 抽牌流程：
// 1. 检查手牌是否已满（>= 10）
// 2. 若抽牌堆空且弃牌堆非空，自动洗牌
// 3. 抽指定数量
// 4. 每抽一张发布 ON_CARD_DRAWN
//
// 异常处理：
// - 手牌已满：停止抽牌
// - 两堆都空：停止抽牌
// - 洗牌后仍空：停止抽牌
// ==========================================
void DeckSystem::drawCards(GameEngine& engine, int amount) {
    auto& combat = *engine.combatState;
    for (int i = 0; i < amount; ++i) {
        if (combat.hand.size() >= 10) {
            STS_LOG(combat, "[DeckSystem] 手牌已满，停止抽牌\n");
            break;
        }

        if (combat.drawPile.empty() && combat.discardPile.empty()) {
            STS_LOG(combat, "[DeckSystem] 抽牌堆和弃牌堆均空，停止抽牌\n");
            break;
        }

        if (combat.drawPile.empty() && !combat.discardPile.empty()) {
            shuffleDiscardIntoDraw(engine);
            if (combat.drawPile.empty()) {
                STS_LOG(combat, "[DeckSystem] 洗牌后牌堆仍为空，停止抽牌\n");
                break;
            }
        }

        auto card = combat.drawPile.back();
        combat.drawPile.pop_back();

        combat.hand.push_back(card);
        engine.eventBus.publish(EventType::ON_CARD_DRAWN, engine, card.get());
    }
}

// ==========================================
// moveToDiscard - 移入弃牌堆
//
// 1. 从所有牌堆中移除该卡牌
// 2. 移入弃牌堆
// 3. 发布 ON_CARD_DISCARDED 事件
// ==========================================
void DeckSystem::moveToDiscard(GameEngine& engine, std::shared_ptr<AbstractCard> card) {
    auto& combat = *engine.combatState;
    eraseFromAllPiles(engine, card);

    combat.discardPile.push_back(card);
    engine.eventBus.publish(EventType::ON_CARD_DISCARDED, engine, card.get());
    STS_LOG(combat, "[DeckSystem] " << card->id << " 进入弃牌堆\n");
}

// ==========================================
// moveToExhaust - 移入消耗堆
//
// 1. 从所有牌堆中移除该卡牌
// 2. 移入消耗堆
// 3. 发布 ON_CARD_EXHAUSTED 事件
// ==========================================
void DeckSystem::moveToExhaust(GameEngine& engine, std::shared_ptr<AbstractCard> card) {
    auto& combat = *engine.combatState;
    eraseFromAllPiles(engine, card);

    combat.exhaustPile.push_back(card);
    engine.eventBus.publish(EventType::ON_CARD_EXHAUSTED, engine, card.get());
    STS_LOG(combat, "[DeckSystem] " << card->id << " 进入消耗堆\n");
}

// ==========================================
// moveToHand - 移入手牌
//
// 爆牌判定：手牌 >= 10 时进入弃牌堆
// 爆牌时发布 ON_CARD_DISCARDED，而非 ON_CARD_DRAWN
// ==========================================
void DeckSystem::moveToHand(GameEngine& engine, std::shared_ptr<AbstractCard> card) {
    auto& combat = *engine.combatState;
    eraseFromAllPiles(engine, card);

    if (combat.hand.size() >= 10) {
        combat.discardPile.push_back(card);
        engine.eventBus.publish(EventType::ON_CARD_DISCARDED, engine, card.get());
        STS_LOG(combat, "[DeckSystem] 爆牌！ " << card->id << " 进入弃牌堆\n");
    } else {
        combat.hand.push_back(card);
        engine.eventBus.publish(EventType::ON_CARD_DRAWN, engine, card.get());
        STS_LOG(combat, "[DeckSystem] " << card->id << " 进入手牌\n");
    }
}

// ==========================================
// eraseFromLimbo - 从滞留区移除
//
// 只从 limbo 移除，不发布任何事件
// ==========================================
void DeckSystem::eraseFromLimbo(GameEngine& engine, std::shared_ptr<AbstractCard> card) {
    auto& combat = *engine.combatState;
    auto it = std::find(combat.limbo.begin(), combat.limbo.end(), card);
    if (it != combat.limbo.end()) {
        combat.limbo.erase(it);
    }
}

// ==========================================
// discardHand - 弃置所有手牌
//
// 将 hand 全部移入 discardPile
// ==========================================
void DeckSystem::discardHand(GameEngine& engine) {
    auto& combat = *engine.combatState;
    while (!combat.hand.empty()) {
        auto card = combat.hand.back();
        combat.hand.pop_back();
        combat.discardPile.push_back(card);
        engine.eventBus.publish(EventType::ON_CARD_DISCARDED, engine, card.get());
    }
    STS_LOG(combat, "[DeckSystem] 弃置所有手牌\n");
}

// ==========================================
// eraseFromAllPiles - 从所有牌堆中移除
//
// 从 drawPile / hand / discardPile / limbo 中移除
// 私有方法，被 moveToDiscard/moveToExhaust/moveToHand 调用
// ==========================================
void DeckSystem::eraseFromAllPiles(GameEngine& engine, std::shared_ptr<AbstractCard> card) {
    auto& combat = *engine.combatState;

    auto eraseFrom = [&](std::vector<std::shared_ptr<AbstractCard>>& pile) {
        auto it = std::find(pile.begin(), pile.end(), card);
        if (it != pile.end()) {
            pile.erase(it);
        }
    };

    eraseFrom(combat.drawPile);
    eraseFrom(combat.hand);
    eraseFrom(combat.discardPile);
    eraseFrom(combat.limbo);
}
