#include "src/test/TestFramework.h"
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/action/Actions.h"
#include "src/card/Cards.h"
#include "src/rules/BasicRules.h"
#include "src/system/DeckSystem.h"
#include <cmath>

using namespace TestFramework;

namespace CardSelectionTests {

GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("测试怪物", 50));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

void test_RequestCardSelectionAction_SetsPhase() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->currentPhase == StatePhase::WAITING_FOR_CARD_SELECTION,
        "Phase should be WAITING_FOR_CARD_SELECTION after RequestCardSelectionAction");
    TEST_ASSERT(engine.combatState->selectionCtx.has_value(),
        "selectionCtx should have value after RequestCardSelectionAction");
}

void test_RequestCardSelectionAction_SetsMinMaxSelection() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());
    engine.combatState->hand.push_back(std::make_shared<WhirlwindCard>());
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 0, 2
    ));
    
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(engine.combatState->selectionCtx->minSelection, 0, "minSelection should be 0");
    TEST_ASSERT_EQ(engine.combatState->selectionCtx->maxSelection, 2, "maxSelection should be 2");
}

void test_RequestCardSelectionAction_EmptyPileSkips() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->currentPhase == StatePhase::PLAYING_CARD,
        "Phase should remain PLAYING_CARD when source pile is empty");
    TEST_ASSERT(!engine.combatState->selectionCtx.has_value(),
        "selectionCtx should not have value when source pile is empty");
}

void test_ChooseCard_SingleSelection() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    engine.combatState->hand.push_back(strike);
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->currentPhase == StatePhase::WAITING_FOR_CARD_SELECTION, 
        "Should be waiting for selection");
    
    size_t initialHandSize = engine.combatState->hand.size();
    
    PlayerActions::chooseCard(engine, flow, 0);
    
    TEST_ASSERT(engine.combatState->currentPhase == StatePhase::PLAYING_CARD,
        "Phase should return to PLAYING_CARD after selection");
    TEST_ASSERT(!engine.combatState->selectionCtx.has_value(),
        "selectionCtx should be cleared after selection");
    TEST_ASSERT(engine.combatState->hand.size() < initialHandSize,
        "Hand size should decrease after card is exhausted");
}

void test_ChooseCards_MultipleSelection() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());
    engine.combatState->hand.push_back(std::make_shared<WhirlwindCard>());
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 0, 2
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    std::vector<int> choices = {0, 2};
    PlayerActions::chooseCards(engine, flow, choices);
    
    TEST_ASSERT(engine.combatState->currentPhase == StatePhase::PLAYING_CARD,
        "Phase should return to PLAYING_CARD after multi-selection");
}

void test_ChooseCards_InvalidCount() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    std::vector<int> tooManyChoices = {0, 1};
    PlayerActions::chooseCards(engine, flow, tooManyChoices);
    
    TEST_ASSERT(engine.combatState->currentPhase == StatePhase::WAITING_FOR_CARD_SELECTION,
        "Phase should remain WAITING when selection count is invalid");
}

void test_ChooseCard_WrongPhaseIgnored() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;
    engine.combatState->selectionCtx = std::nullopt;
    
    PlayerActions::chooseCard(engine, flow, 0);
    
    TEST_ASSERT(engine.combatState->currentPhase == StatePhase::PLAYING_CARD,
        "chooseCard should be ignored when not in WAITING_FOR_CARD_SELECTION phase");
}

void test_SelectionPurpose_Routing_ExhaustFromHand() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    engine.combatState->hand.push_back(card);
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    size_t initialExhaustPile = engine.combatState->exhaustPile.size();
    
    PlayerActions::chooseCard(engine, flow, 0);
    
    TEST_ASSERT(engine.combatState->exhaustPile.size() > initialExhaustPile, 
        "Card should be in exhaust pile after EXHAUST_FROM_HAND routing");
    TEST_ASSERT(engine.combatState->exhaustPile.back() == card,
        "The exact selected card (Strike) should be in exhaust pile");
}

void test_SelectionPurpose_Routing_DiscardFromHand() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    engine.combatState->hand.push_back(card);
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    size_t initialDiscardPile = engine.combatState->discardPile.size();
    
    PlayerActions::chooseCard(engine, flow, 0);
    
    TEST_ASSERT(engine.combatState->discardPile.size() > initialDiscardPile, 
        "Card should be in discard pile after DISCARD_FROM_HAND routing");
    TEST_ASSERT(engine.combatState->discardPile.back() == card,
        "The exact selected card (Strike) should be in discard pile");
}

void test_SelectionPurpose_Routing_MoveToHand() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    engine.combatState->discardPile.push_back(card);
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::DISCARD_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    size_t initialHandSize = engine.combatState->hand.size();
    
    PlayerActions::chooseCard(engine, flow, 0);
    
    TEST_ASSERT(engine.combatState->hand.size() > initialHandSize, 
        "Card should be in hand after MOVE_TO_HAND routing");
}

void test_MaxSelection_ClampedToPileSize() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 5
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(engine.combatState->selectionCtx->maxSelection, 1, 
        "maxSelection should be clamped to pile size (1)");
}

void test_SelectionContext_PreservesPurpose() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT(engine.combatState->selectionCtx->purpose == SelectionPurpose::MOVE_TO_HAND,
        "Purpose should be preserved in selectionCtx");
}

void test_PileType_DrawPileSelection() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    engine.combatState->drawPile.push_back(card);
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::DRAW_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(engine.combatState->selectionCtx->choices.size(), 1, "Should have 1 card to choose from draw pile");
    
    PlayerActions::chooseCard(engine, flow, 0);
    
    TEST_ASSERT(engine.combatState->hand.size() == 1, "Card should be moved from draw pile to hand");
}

void test_PileType_ExhaustPileSelection() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    engine.combatState->exhaustPile.push_back(card);
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::EXHAUST_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(engine.combatState->selectionCtx->choices.size(), 1, "Should have 1 card to choose from exhaust pile");
    
    PlayerActions::chooseCard(engine, flow, 0);
    
    TEST_ASSERT(engine.combatState->hand.size() == 1, "Card should be moved from exhaust pile to hand");
}

void test_TimingIssue_DrawThenSelect() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    for (int i = 0; i < 4; i++) {
        engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    }
    
    engine.combatState->hand.push_back(std::make_shared<DeadlyPoisonCard>());
    
    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(4));
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->selectionCtx.has_value(), "selectionCtx should have value after draw");
    TEST_ASSERT_EQ(engine.combatState->selectionCtx->choices.size(), 5, 
        "Should have 5 cards to choose from (1 original + 4 drawn)");
    
    PlayerActions::chooseCard(engine, flow, 0);
    
    TEST_ASSERT_EQ(engine.combatState->hand.size(), 4, "Should have 4 cards remaining after discarding 1");
    TEST_ASSERT_EQ(engine.combatState->discardPile.size(), 1, "Should have 1 card in discard pile");
}

void test_TimingIssue_MultipleDrawsBeforeSelect() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    for (int i = 0; i < 10; i++) {
        engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    }
    
    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(3));
    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(2));
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT(engine.combatState->selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(engine.combatState->selectionCtx->choices.size(), 5, 
        "Should have 5 cards (3 + 2 drawn)");
}

void test_TimingIssue_RealTimeHandUpdate() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    engine.combatState->drawPile.push_back(std::make_shared<StrikeCard>());
    engine.combatState->drawPile.push_back(std::make_shared<DeadlyPoisonCard>());
    engine.combatState->drawPile.push_back(std::make_shared<WhirlwindCard>());
    
    engine.combatState->hand.push_back(std::make_shared<StrikeCard>());
    
    engine.actionManager.addAction(std::make_unique<DrawCardsAction>(2));
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT_EQ(engine.combatState->selectionCtx->choices.size(), 3, 
        "Should have 3 cards (1 original + 2 drawn)");
    
    size_t initialExhaustPile = engine.combatState->exhaustPile.size();
    PlayerActions::chooseCard(engine, flow, 2);
    
    TEST_ASSERT(engine.combatState->exhaustPile.size() > initialExhaustPile, 
        "Selected card should be in exhaust pile");
    TEST_ASSERT_EQ(engine.combatState->hand.size(), 2, "Should have 2 cards remaining in hand");
}

void test_SelectedCardIdentity_ExhaustCorrectCard() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    auto whirlwind = std::make_shared<WhirlwindCard>();
    
    engine.combatState->hand.push_back(strike);
    engine.combatState->hand.push_back(poison);
    engine.combatState->hand.push_back(whirlwind);
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    PlayerActions::chooseCard(engine, flow, 1);
    
    TEST_ASSERT(engine.combatState->exhaustPile.back() == poison,
        "The selected DeadlyPoisonCard should be in exhaust pile, not Strike or Whirlwind");
    TEST_ASSERT(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), poison) == engine.combatState->hand.end(),
        "DeadlyPoisonCard should no longer be in hand");
    TEST_ASSERT(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), strike) != engine.combatState->hand.end(),
        "Strike should still be in hand");
    TEST_ASSERT(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), whirlwind) != engine.combatState->hand.end(),
        "Whirlwind should still be in hand");
}

void test_SelectedCardIdentity_DiscardCorrectCard() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    
    engine.combatState->hand.push_back(strike);
    engine.combatState->hand.push_back(poison);
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    PlayerActions::chooseCard(engine, flow, 0);
    
    TEST_ASSERT(engine.combatState->discardPile.back() == strike,
        "The selected Strike should be in discard pile, not DeadlyPoison");
    TEST_ASSERT(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), strike) == engine.combatState->hand.end(),
        "Strike should no longer be in hand");
    TEST_ASSERT(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), poison) != engine.combatState->hand.end(),
        "DeadlyPoison should still be in hand");
}

void test_SelectedCardIdentity_MoveToHandCorrectCard() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    
    engine.combatState->discardPile.push_back(strike);
    engine.combatState->discardPile.push_back(poison);
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::DISCARD_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    PlayerActions::chooseCard(engine, flow, 1);
    
    TEST_ASSERT(engine.combatState->hand.back() == poison,
        "The selected DeadlyPoison should be moved to hand, not Strike");
    TEST_ASSERT(std::find(engine.combatState->discardPile.begin(), engine.combatState->discardPile.end(), poison) == engine.combatState->discardPile.end(),
        "DeadlyPoison should no longer be in discard pile");
    TEST_ASSERT(std::find(engine.combatState->discardPile.begin(), engine.combatState->discardPile.end(), strike) != engine.combatState->discardPile.end(),
        "Strike should still be in discard pile");
}

void test_SelectedCardIdentity_MultipleCardsExhaust() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    auto whirlwind = std::make_shared<WhirlwindCard>();
    
    engine.combatState->hand.push_back(strike);
    engine.combatState->hand.push_back(poison);
    engine.combatState->hand.push_back(whirlwind);
    
    engine.actionManager.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 2, 2
    ));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    std::vector<int> choices = {0, 2};
    PlayerActions::chooseCards(engine, flow, choices);
    
    TEST_ASSERT_EQ(engine.combatState->exhaustPile.size(), 2, "Should have 2 cards in exhaust pile");
    TEST_ASSERT(std::find(engine.combatState->exhaustPile.begin(), engine.combatState->exhaustPile.end(), strike) != engine.combatState->exhaustPile.end(),
        "Strike should be in exhaust pile");
    TEST_ASSERT(std::find(engine.combatState->exhaustPile.begin(), engine.combatState->exhaustPile.end(), whirlwind) != engine.combatState->exhaustPile.end(),
        "Whirlwind should be in exhaust pile");
    TEST_ASSERT(std::find(engine.combatState->exhaustPile.begin(), engine.combatState->exhaustPile.end(), poison) == engine.combatState->exhaustPile.end(),
        "DeadlyPoison should NOT be in exhaust pile");
    TEST_ASSERT(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), poison) != engine.combatState->hand.end(),
        "DeadlyPoison should still be in hand");
}

void test_SelectedCardIdentity_DrawThenSelectCorrectCard() {
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
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT_EQ(engine.combatState->selectionCtx->choices.size(), 3, "Should have 3 cards to choose");
    
    PlayerActions::chooseCard(engine, flow, 2);
    
    TEST_ASSERT(engine.combatState->discardPile.back() == drawnCard1,
        "The selected drawn card (DeadlyPoison at index 2) should be in discard pile");
    TEST_ASSERT(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), originalCard) != engine.combatState->hand.end(),
        "Original Strike should still be in hand");
    TEST_ASSERT(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), drawnCard2) != engine.combatState->hand.end(),
        "Drawn Whirlwind should still be in hand");
    TEST_ASSERT(std::find(engine.combatState->hand.begin(), engine.combatState->hand.end(), drawnCard1) == engine.combatState->hand.end(),
        "Discarded DeadlyPoison should NOT be in hand");
}

void runAllTests() {
    TestSuite suite("卡牌选牌逻辑测试");
    
    RUN_TEST(suite, test_RequestCardSelectionAction_SetsPhase);
    RUN_TEST(suite, test_RequestCardSelectionAction_SetsMinMaxSelection);
    RUN_TEST(suite, test_RequestCardSelectionAction_EmptyPileSkips);
    RUN_TEST(suite, test_ChooseCard_SingleSelection);
    RUN_TEST(suite, test_ChooseCards_MultipleSelection);
    RUN_TEST(suite, test_ChooseCards_InvalidCount);
    RUN_TEST(suite, test_ChooseCard_WrongPhaseIgnored);
    RUN_TEST(suite, test_SelectionPurpose_Routing_ExhaustFromHand);
    RUN_TEST(suite, test_SelectionPurpose_Routing_DiscardFromHand);
    RUN_TEST(suite, test_SelectionPurpose_Routing_MoveToHand);
    RUN_TEST(suite, test_MaxSelection_ClampedToPileSize);
    RUN_TEST(suite, test_SelectionContext_PreservesPurpose);
    RUN_TEST(suite, test_PileType_DrawPileSelection);
    RUN_TEST(suite, test_PileType_ExhaustPileSelection);
    RUN_TEST(suite, test_TimingIssue_DrawThenSelect);
    RUN_TEST(suite, test_TimingIssue_MultipleDrawsBeforeSelect);
    RUN_TEST(suite, test_TimingIssue_RealTimeHandUpdate);
    RUN_TEST(suite, test_SelectedCardIdentity_ExhaustCorrectCard);
    RUN_TEST(suite, test_SelectedCardIdentity_DiscardCorrectCard);
    RUN_TEST(suite, test_SelectedCardIdentity_MoveToHandCorrectCard);
    RUN_TEST(suite, test_SelectedCardIdentity_MultipleCardsExhaust);
    RUN_TEST(suite, test_SelectedCardIdentity_DrawThenSelectCorrectCard);
    
    suite.printReport();
}

}
