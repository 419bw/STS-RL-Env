#include "src/test/TestFramework.h"
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/action/Actions.h"
#include "src/card/Cards.h"
#include "src/rules/BasicRules.h"
#include "src/system/ActionSystem.h"
#include "src/system/DeckSystem.h"
#include <cmath>

using namespace TestFramework;

namespace CardSelectionTests {

GameState createTestState() {
    GameState state;
    state.enableLogging = false;
    state.monsters.push_back(std::make_shared<Monster>("测试怪物", 50));
    BasicRules::registerRules(state);
    return state;
}

void test_RequestCardSelectionAction_SetsPhase() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.hand.push_back(std::make_shared<StrikeCard>());
    state.hand.push_back(std::make_shared<DeadlyPoisonCard>());
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.currentPhase == StatePhase::WAITING_FOR_CARD_SELECTION,
        "Phase should be WAITING_FOR_CARD_SELECTION after RequestCardSelectionAction");
    TEST_ASSERT(state.selectionCtx.has_value(),
        "selectionCtx should have value after RequestCardSelectionAction");
}

void test_RequestCardSelectionAction_SetsMinMaxSelection() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.hand.push_back(std::make_shared<StrikeCard>());
    state.hand.push_back(std::make_shared<DeadlyPoisonCard>());
    state.hand.push_back(std::make_shared<WhirlwindCard>());
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 0, 2
    ));
    
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(state.selectionCtx->minSelection, 0, "minSelection should be 0");
    TEST_ASSERT_EQ(state.selectionCtx->maxSelection, 2, "maxSelection should be 2");
}

void test_RequestCardSelectionAction_EmptyPileSkips() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.currentPhase == StatePhase::PLAYING_CARD,
        "Phase should remain PLAYING_CARD when source pile is empty");
    TEST_ASSERT(!state.selectionCtx.has_value(),
        "selectionCtx should not have value when source pile is empty");
}

void test_ChooseCard_SingleSelection() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    state.hand.push_back(strike);
    state.hand.push_back(std::make_shared<DeadlyPoisonCard>());
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.currentPhase == StatePhase::WAITING_FOR_CARD_SELECTION, 
        "Should be waiting for selection");
    
    size_t initialHandSize = state.hand.size();
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.currentPhase == StatePhase::PLAYING_CARD,
        "Phase should return to PLAYING_CARD after selection");
    TEST_ASSERT(!state.selectionCtx.has_value(),
        "selectionCtx should be cleared after selection");
    TEST_ASSERT(state.hand.size() < initialHandSize,
        "Hand size should decrease after card is exhausted");
}

void test_ChooseCards_MultipleSelection() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.hand.push_back(std::make_shared<StrikeCard>());
    state.hand.push_back(std::make_shared<DeadlyPoisonCard>());
    state.hand.push_back(std::make_shared<WhirlwindCard>());
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 0, 2
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    std::vector<int> choices = {0, 2};
    PlayerActions::chooseCards(state, flow, choices);
    
    TEST_ASSERT(state.currentPhase == StatePhase::PLAYING_CARD,
        "Phase should return to PLAYING_CARD after multi-selection");
}

void test_ChooseCards_InvalidCount() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.hand.push_back(std::make_shared<StrikeCard>());
    state.hand.push_back(std::make_shared<DeadlyPoisonCard>());
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    std::vector<int> tooManyChoices = {0, 1};
    PlayerActions::chooseCards(state, flow, tooManyChoices);
    
    TEST_ASSERT(state.currentPhase == StatePhase::WAITING_FOR_CARD_SELECTION,
        "Phase should remain WAITING when selection count is invalid");
}

void test_ChooseCard_WrongPhaseIgnored() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.currentPhase = StatePhase::PLAYING_CARD;
    state.selectionCtx = std::nullopt;
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.currentPhase == StatePhase::PLAYING_CARD,
        "chooseCard should be ignored when not in WAITING_FOR_CARD_SELECTION phase");
}

void test_SelectionPurpose_Routing_ExhaustFromHand() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    state.hand.push_back(card);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    size_t initialExhaustPile = state.exhaustPile.size();
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.exhaustPile.size() > initialExhaustPile, 
        "Card should be in exhaust pile after EXHAUST_FROM_HAND routing");
    TEST_ASSERT(state.exhaustPile.back() == card,
        "The exact selected card (Strike) should be in exhaust pile");
}

void test_SelectionPurpose_Routing_DiscardFromHand() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    state.hand.push_back(card);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    size_t initialDiscardPile = state.discardPile.size();
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.discardPile.size() > initialDiscardPile, 
        "Card should be in discard pile after DISCARD_FROM_HAND routing");
    TEST_ASSERT(state.discardPile.back() == card,
        "The exact selected card (Strike) should be in discard pile");
}

void test_SelectionPurpose_Routing_MoveToHand() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    state.discardPile.push_back(card);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::DISCARD_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    size_t initialHandSize = state.hand.size();
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.hand.size() > initialHandSize, 
        "Card should be in hand after MOVE_TO_HAND routing");
}

void test_MaxSelection_ClampedToPileSize() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.hand.push_back(std::make_shared<StrikeCard>());
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 5
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(state.selectionCtx->maxSelection, 1, 
        "maxSelection should be clamped to pile size (1)");
}

void test_SelectionContext_PreservesPurpose() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.hand.push_back(std::make_shared<StrikeCard>());
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT(state.selectionCtx->purpose == SelectionPurpose::MOVE_TO_HAND,
        "Purpose should be preserved in selectionCtx");
}

void test_PileType_DrawPileSelection() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    state.drawPile.push_back(card);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::DRAW_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(state.selectionCtx->choices.size(), 1, "Should have 1 card to choose from draw pile");
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.hand.size() == 1, "Card should be moved from draw pile to hand");
}

void test_PileType_ExhaustPileSelection() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    state.exhaustPile.push_back(card);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::EXHAUST_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(state.selectionCtx->choices.size(), 1, "Should have 1 card to choose from exhaust pile");
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.hand.size() == 1, "Card should be moved from exhaust pile to hand");
}

void test_TimingIssue_DrawThenSelect() {
    GameState state = createTestState();
    CombatFlow flow;
    
    for (int i = 0; i < 4; i++) {
        state.drawPile.push_back(std::make_shared<StrikeCard>());
    }
    
    state.hand.push_back(std::make_shared<DeadlyPoisonCard>());
    
    state.addAction(std::make_unique<DrawCardsAction>(4));
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.selectionCtx.has_value(), "selectionCtx should have value after draw");
    TEST_ASSERT_EQ(state.selectionCtx->choices.size(), 5, 
        "Should have 5 cards to choose from (1 original + 4 drawn)");
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT_EQ(state.hand.size(), 4, "Should have 4 cards remaining after discarding 1");
    TEST_ASSERT_EQ(state.discardPile.size(), 1, "Should have 1 card in discard pile");
}

void test_TimingIssue_MultipleDrawsBeforeSelect() {
    GameState state = createTestState();
    CombatFlow flow;
    
    for (int i = 0; i < 10; i++) {
        state.drawPile.push_back(std::make_shared<StrikeCard>());
    }
    
    state.addAction(std::make_unique<DrawCardsAction>(3));
    state.addAction(std::make_unique<DrawCardsAction>(2));
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(state.selectionCtx->choices.size(), 5, 
        "Should have 5 cards (3 + 2 drawn)");
}

void test_TimingIssue_RealTimeHandUpdate() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.drawPile.push_back(std::make_shared<StrikeCard>());
    state.drawPile.push_back(std::make_shared<DeadlyPoisonCard>());
    state.drawPile.push_back(std::make_shared<WhirlwindCard>());
    
    state.hand.push_back(std::make_shared<StrikeCard>());
    
    state.addAction(std::make_unique<DrawCardsAction>(2));
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT_EQ(state.selectionCtx->choices.size(), 3, 
        "Should have 3 cards (1 original + 2 drawn)");
    
    size_t initialExhaustPile = state.exhaustPile.size();
    PlayerActions::chooseCard(state, flow, 2);
    
    TEST_ASSERT(state.exhaustPile.size() > initialExhaustPile, 
        "Selected card should be in exhaust pile");
    TEST_ASSERT_EQ(state.hand.size(), 2, "Should have 2 cards remaining in hand");
}

void test_SelectedCardIdentity_ExhaustCorrectCard() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    auto whirlwind = std::make_shared<WhirlwindCard>();
    
    state.hand.push_back(strike);
    state.hand.push_back(poison);
    state.hand.push_back(whirlwind);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    PlayerActions::chooseCard(state, flow, 1);
    
    TEST_ASSERT(state.exhaustPile.back() == poison,
        "The selected DeadlyPoisonCard should be in exhaust pile, not Strike or Whirlwind");
    TEST_ASSERT(std::find(state.hand.begin(), state.hand.end(), poison) == state.hand.end(),
        "DeadlyPoisonCard should no longer be in hand");
    TEST_ASSERT(std::find(state.hand.begin(), state.hand.end(), strike) != state.hand.end(),
        "Strike should still be in hand");
    TEST_ASSERT(std::find(state.hand.begin(), state.hand.end(), whirlwind) != state.hand.end(),
        "Whirlwind should still be in hand");
}

void test_SelectedCardIdentity_DiscardCorrectCard() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    
    state.hand.push_back(strike);
    state.hand.push_back(poison);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.discardPile.back() == strike,
        "The selected Strike should be in discard pile, not DeadlyPoison");
    TEST_ASSERT(std::find(state.hand.begin(), state.hand.end(), strike) == state.hand.end(),
        "Strike should no longer be in hand");
    TEST_ASSERT(std::find(state.hand.begin(), state.hand.end(), poison) != state.hand.end(),
        "DeadlyPoison should still be in hand");
}

void test_SelectedCardIdentity_MoveToHandCorrectCard() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    
    state.discardPile.push_back(strike);
    state.discardPile.push_back(poison);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::DISCARD_PILE, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    PlayerActions::chooseCard(state, flow, 1);
    
    TEST_ASSERT(state.hand.back() == poison,
        "The selected DeadlyPoison should be moved to hand, not Strike");
    TEST_ASSERT(std::find(state.discardPile.begin(), state.discardPile.end(), poison) == state.discardPile.end(),
        "DeadlyPoison should no longer be in discard pile");
    TEST_ASSERT(std::find(state.discardPile.begin(), state.discardPile.end(), strike) != state.discardPile.end(),
        "Strike should still be in discard pile");
}

void test_SelectedCardIdentity_MultipleCardsExhaust() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto strike = std::make_shared<StrikeCard>();
    auto poison = std::make_shared<DeadlyPoisonCard>();
    auto whirlwind = std::make_shared<WhirlwindCard>();
    
    state.hand.push_back(strike);
    state.hand.push_back(poison);
    state.hand.push_back(whirlwind);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::EXHAUST_FROM_HAND, 2, 2
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    std::vector<int> choices = {0, 2};
    PlayerActions::chooseCards(state, flow, choices);
    
    TEST_ASSERT_EQ(state.exhaustPile.size(), 2, "Should have 2 cards in exhaust pile");
    TEST_ASSERT(std::find(state.exhaustPile.begin(), state.exhaustPile.end(), strike) != state.exhaustPile.end(),
        "Strike should be in exhaust pile");
    TEST_ASSERT(std::find(state.exhaustPile.begin(), state.exhaustPile.end(), whirlwind) != state.exhaustPile.end(),
        "Whirlwind should be in exhaust pile");
    TEST_ASSERT(std::find(state.exhaustPile.begin(), state.exhaustPile.end(), poison) == state.exhaustPile.end(),
        "DeadlyPoison should NOT be in exhaust pile");
    TEST_ASSERT(std::find(state.hand.begin(), state.hand.end(), poison) != state.hand.end(),
        "DeadlyPoison should still be in hand");
}

void test_SelectedCardIdentity_DrawThenSelectCorrectCard() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto originalCard = std::make_shared<StrikeCard>();
    auto drawnCard1 = std::make_shared<DeadlyPoisonCard>();
    auto drawnCard2 = std::make_shared<WhirlwindCard>();
    
    state.hand.push_back(originalCard);
    state.drawPile.push_back(drawnCard1);
    state.drawPile.push_back(drawnCard2);
    
    state.addAction(std::make_unique<DrawCardsAction>(2));
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        PileType::HAND, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT_EQ(state.selectionCtx->choices.size(), 3, "Should have 3 cards to choose");
    
    // 抽牌顺序：从 drawPile.back() 抽取
    // drawPile: [drawnCard1, drawnCard2] -> back 是 drawnCard2
    // 先抽 drawnCard2，再抽 drawnCard1
    // hand 最终顺序: [originalCard, drawnCard2, drawnCard1]
    // 索引 0 = originalCard (Strike)
    // 索引 1 = drawnCard2 (Whirlwind)
    // 索引 2 = drawnCard1 (DeadlyPoison)
    
    PlayerActions::chooseCard(state, flow, 2);
    
    TEST_ASSERT(state.discardPile.back() == drawnCard1,
        "The selected drawn card (DeadlyPoison at index 2) should be in discard pile");
    TEST_ASSERT(std::find(state.hand.begin(), state.hand.end(), originalCard) != state.hand.end(),
        "Original Strike should still be in hand");
    TEST_ASSERT(std::find(state.hand.begin(), state.hand.end(), drawnCard2) != state.hand.end(),
        "Drawn Whirlwind should still be in hand");
    TEST_ASSERT(std::find(state.hand.begin(), state.hand.end(), drawnCard1) == state.hand.end(),
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
