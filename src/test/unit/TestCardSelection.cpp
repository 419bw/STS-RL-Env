#include <catch_amalgamated.hpp>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/action/Actions.h"
#include "src/card/Cards.h"
#include "src/rules/BasicRules.h"
#include "src/system/DeckSystem.h"
#include <algorithm>

static GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("TestMonster", 50));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

TEST_CASE("RequestCardSelectionAction sets phase", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->currentPhase == StatePhase::WAITING_FOR_CARD_SELECTION);
    REQUIRE(engine.combatState->selectionCtx.has_value());
}

TEST_CASE("RequestCardSelectionAction sets minMax selection", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());
    engine.combatState->hand.push_back(std::make_shared<WhirlwindCard>());

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 0, 2));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->selectionCtx.has_value());
    REQUIRE(engine.combatState->selectionCtx->minSelection == 0);
    REQUIRE(engine.combatState->selectionCtx->maxSelection == 2);
}

TEST_CASE("RequestCardSelectionAction empty pile skips", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->currentPhase == StatePhase::PLAYING_CARD);
    REQUIRE(!engine.combatState->selectionCtx.has_value());
}

TEST_CASE("ChooseCard single selection", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto strike = std::make_shared<StrikeCard>();
    engine.combatState->hand.push_back(strike);
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->currentPhase == StatePhase::WAITING_FOR_CARD_SELECTION);
    size_t initialHandSize = engine.combatState->hand.size();

    PlayerActions::chooseCard(engine, flow, 0);

    REQUIRE(engine.combatState->currentPhase == StatePhase::PLAYING_CARD);
    REQUIRE(!engine.combatState->selectionCtx.has_value());
    REQUIRE(engine.combatState->hand.size() < initialHandSize);
}

TEST_CASE("ChooseCards multiple selection", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());
    engine.combatState->hand.push_back(std::make_shared<WhirlwindCard>());

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 0, 2));
    engine.actionManager.executeUntilBlocked(engine, flow);

    std::vector<int> choices = {0, 2};
    PlayerActions::chooseCards(engine, flow, choices);

    REQUIRE(engine.combatState->currentPhase == StatePhase::PLAYING_CARD);
}

TEST_CASE("ChooseCards invalid count rejected", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    std::vector<int> tooManyChoices = {0, 1};
    PlayerActions::chooseCards(engine, flow, tooManyChoices);

    REQUIRE(engine.combatState->currentPhase == StatePhase::WAITING_FOR_CARD_SELECTION);
}

TEST_CASE("ChooseCard wrong phase ignored", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;
    engine.combatState->selectionCtx = std::nullopt;

    PlayerActions::chooseCard(engine, flow, 0);

    REQUIRE(engine.combatState->currentPhase == StatePhase::PLAYING_CARD);
}

TEST_CASE("Selection purpose routing: exhaust from hand", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto card = std::make_shared<StrikeCard>();
    engine.combatState->hand.push_back(card);

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    size_t initialExhaustPile = engine.combatState->exhaustPile.size();
    PlayerActions::chooseCard(engine, flow, 0);

    REQUIRE(engine.combatState->exhaustPile.size() > initialExhaustPile);
    REQUIRE(engine.combatState->exhaustPile.back() == card);
}

TEST_CASE("Selection purpose routing: discard from hand", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto card = std::make_shared<StrikeCard>();
    engine.combatState->hand.push_back(card);

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    size_t initialDiscardPile = engine.combatState->discardPile.size();
    PlayerActions::chooseCard(engine, flow, 0);

    REQUIRE(engine.combatState->discardPile.size() > initialDiscardPile);
    REQUIRE(engine.combatState->discardPile.back() == card);
}

TEST_CASE("Selection purpose routing: move to hand", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto card = std::make_shared<StrikeCard>();
    engine.combatState->discardPile.push_back(card);

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::DISCARD_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    size_t initialHandSize = engine.combatState->hand.size();
    PlayerActions::chooseCard(engine, flow, 0);

    REQUIRE(engine.combatState->hand.size() > initialHandSize);
}

TEST_CASE("Max selection clamped to pile size", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->selectionCtx.has_value());
    REQUIRE(engine.combatState->selectionCtx->maxSelection == 1);
}

TEST_CASE("Selection context preserves purpose", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::MOVE_TO_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->selectionCtx.has_value());
    REQUIRE(engine.combatState->selectionCtx->purpose == SelectionPurpose::MOVE_TO_HAND);
}

TEST_CASE("Pile type: draw pile selection", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto card = std::make_shared<StrikeCard>();
    engine.combatState->drawPile.push_back(card);

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::DRAW_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->selectionCtx.has_value());
    REQUIRE(engine.combatState->selectionCtx->choices.size() == 1);

    PlayerActions::chooseCard(engine, flow, 0);

    REQUIRE(engine.combatState->hand.size() == 1);
}

TEST_CASE("Pile type: exhaust pile selection", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto card = std::make_shared<StrikeCard>();
    engine.combatState->exhaustPile.push_back(card);

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::EXHAUST_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->selectionCtx.has_value());
    REQUIRE(engine.combatState->selectionCtx->choices.size() == 1);

    PlayerActions::chooseCard(engine, flow, 0);

    REQUIRE(engine.combatState->hand.size() == 1);
}

TEST_CASE("Timing: draw then select", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    for (int i = 0; i < 4; i++) {
        engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    }
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());

    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(4));
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->selectionCtx.has_value());
    REQUIRE(engine.combatState->selectionCtx->choices.size() == 5);

    PlayerActions::chooseCard(engine, flow, 0);

    REQUIRE(engine.combatState->hand.size() == 4);
    REQUIRE(engine.combatState->discardPile.size() == 1);
}

TEST_CASE("Timing: multiple draws before select", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    for (int i = 0; i < 10; i++) {
        engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    }

    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(3));
    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(2));
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->selectionCtx.has_value());
    REQUIRE(engine.combatState->selectionCtx->choices.size() == 5);
}

TEST_CASE("Timing: real-time hand update", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    engine.combatState->drawPile.push_back(std::make_shared<DeadlyPoisonCard>());
    engine.combatState->drawPile.push_back(std::make_shared<WhirlwindCard>());
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());

    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(2));
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->selectionCtx->choices.size() == 3);

    size_t initialExhaustPile = engine.combatState->exhaustPile.size();
    PlayerActions::chooseCard(engine, flow, 2);

    REQUIRE(engine.combatState->exhaustPile.size() > initialExhaustPile);
    REQUIRE(engine.combatState->hand.size() == 2);
}

TEST_CASE("Selected card identity: exhaust correct card", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    auto whirlwind = std::make_shared<WhirlwindCard>();
    engine.combatState->hand.push_back(strike);
    engine.combatState->hand.push_back(poison);
    engine.combatState->hand.push_back(whirlwind);

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    PlayerActions::chooseCard(engine, flow, 1);

    REQUIRE(engine.combatState->exhaustPile.back() == poison);
    REQUIRE(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), poison) == engine.combatState->hand.end());
    REQUIRE(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), strike) != engine.combatState->hand.end());
    REQUIRE(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), whirlwind) != engine.combatState->hand.end());
}

TEST_CASE("Selected card identity: discard correct card", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    engine.combatState->hand.push_back(strike);
    engine.combatState->hand.push_back(poison);

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    PlayerActions::chooseCard(engine, flow, 0);

    REQUIRE(engine.combatState->discardPile.back() == strike);
    REQUIRE(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), strike) == engine.combatState->hand.end());
    REQUIRE(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), poison) != engine.combatState->hand.end());
}

TEST_CASE("Selected card identity: move to hand correct card", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    engine.combatState->discardPile.push_back(strike);
    engine.combatState->discardPile.push_back(poison);

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::DISCARD_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    PlayerActions::chooseCard(engine, flow, 1);

    REQUIRE(engine.combatState->hand.back() == poison);
    REQUIRE(std::find(engine.combatState->discardPile.begin(), engine.combatState->discardPile.end(), poison) == engine.combatState->discardPile.end());
    REQUIRE(std::find(engine.combatState->discardPile.begin(), engine.combatState->discardPile.end(), strike) != engine.combatState->discardPile.end());
}

TEST_CASE("Selected card identity: multiple cards exhaust", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    auto whirlwind = std::make_shared<WhirlwindCard>();
    engine.combatState->hand.push_back(strike);
    engine.combatState->hand.push_back(poison);
    engine.combatState->hand.push_back(whirlwind);

    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 2, 2));
    engine.actionManager.executeUntilBlocked(engine, flow);

    std::vector<int> choices = {0, 2};
    PlayerActions::chooseCards(engine, flow, choices);

    REQUIRE(engine.combatState->exhaustPile.size() == 2);
    REQUIRE(std::find(engine.combatState->exhaustPile.begin(), engine.combatState->exhaustPile.end(), strike) != engine.combatState->exhaustPile.end());
    REQUIRE(std::find(engine.combatState->exhaustPile.begin(), engine.combatState->exhaustPile.end(), whirlwind) != engine.combatState->exhaustPile.end());
    REQUIRE(std::find(engine.combatState->exhaustPile.begin(), engine.combatState->exhaustPile.end(), poison) == engine.combatState->exhaustPile.end());
    REQUIRE(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), poison) != engine.combatState->hand.end());
}

TEST_CASE("Selected card identity: draw then select correct card", "[cardselection][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto originalCard = std::make_shared<StrikeCard>();
    auto drawnCard1 = std::make_shared<DeadlyPoisonCard>();
    auto drawnCard2 = std::make_shared<WhirlwindCard>();
    engine.combatState->hand.push_back(originalCard);
    engine.combatState->drawPile.push_back(drawnCard1);
    engine.combatState->drawPile.push_back(drawnCard2);

    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(2));
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.combatState->selectionCtx->choices.size() == 3);

    PlayerActions::chooseCard(engine, flow, 2);

    REQUIRE(engine.combatState->discardPile.back() == drawnCard1);
    REQUIRE(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), originalCard) != engine.combatState->hand.end());
    REQUIRE(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), drawnCard2) != engine.combatState->hand.end());
    REQUIRE(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), drawnCard1) == engine.combatState->hand.end());
}
