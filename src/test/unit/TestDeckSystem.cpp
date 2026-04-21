#include <catch_amalgamated.hpp>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/Actions.h"
#include "src/system/DeckSystem.h"
#include "src/card/Cards.h"
#include "src/rules/BasicRules.h"

static GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(42);
    engine.startCombat(std::make_shared<Monster>("TestMonster", 100));
    engine.combatState->enableLogging = false;
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;
    BasicRules::registerRules(engine);
    return engine;
}

TEST_CASE("DrawCardsAction draws correct number of cards", "[deck][unit]") {
    GameEngine engine = createTestEngine();
    engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());

    CombatFlow flow;
    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(3));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->hand.size() == 3);
    REQUIRE(engine.combatState->drawPile.empty());
}

TEST_CASE("Shuffle discard into draw pile", "[deck][unit]") {
    GameEngine engine = createTestEngine();

    for (int i = 0; i < 5; ++i) {
        engine.combatState->discardPile.push_back(std::make_shared<StrikeCard>());
    }

    CombatFlow flow;
    engine.actionManager.addAction(std::make_unique<ShuffleDiscardIntoDrawAction>());
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->discardPile.empty());
    REQUIRE(engine.combatState->drawPile.size() == 5);
}

TEST_CASE("Draw more than available triggers shuffle", "[deck][unit]") {
    GameEngine engine = createTestEngine();

    for (int i = 0; i < 3; ++i) {
        engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    }
    for (int i = 0; i < 5; ++i) {
        engine.combatState->discardPile.push_back(std::make_shared<StrikeCard>());
    }

    CombatFlow flow;
    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(5));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->hand.size() == 5);
}

TEST_CASE("Draw from empty piles does not crash", "[deck][unit]") {
    GameEngine engine = createTestEngine();

    REQUIRE_NOTHROW([&]() {
        CombatFlow flow;
        engine.actionManager.addAction(std::make_unique<DrawCardsAction>(5));
        engine.actionManager.executeUntilBlocked(engine, flow);
    }());
}

TEST_CASE("DiscardHandAction empties hand", "[deck][unit]") {
    GameEngine engine = createTestEngine();
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());

    CombatFlow flow;
    engine.actionManager.addAction(std::make_unique<DiscardHandAction>());
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->hand.empty());
    REQUIRE(engine.combatState->discardPile.size() == 2);
}

TEST_CASE("Hand cap prevents drawing more than 10", "[deck][unit]") {
    GameEngine engine = createTestEngine();

    for (int i = 0; i < 8; ++i) {
        engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    }
    for (int i = 0; i < 10; ++i) {
        engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    }

    CombatFlow flow;
    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(10));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->hand.size() <= 10);
}
