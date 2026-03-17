#include "src/test/TestFramework.h"
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/action/Actions.h"
#include "src/card/Cards.h"
#include "src/rules/BasicRules.h"
#include "src/system/ActionSystem.h"
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
        state.hand, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
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
        state.hand, SelectionPurpose::EXHAUST_FROM_HAND, 0, 2
    ));
    
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT_EQ(state.selectionCtx->minSelection, 0, "minSelection should be 0");
    TEST_ASSERT_EQ(state.selectionCtx->maxSelection, 2, "maxSelection should be 2");
}

void test_RequestCardSelectionAction_EmptyPileSkips() {
    GameState state = createTestState();
    CombatFlow flow;
    
    std::vector<std::shared_ptr<AbstractCard>> emptyPile;
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        emptyPile, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
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
        state.hand, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
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
        state.hand, SelectionPurpose::EXHAUST_FROM_HAND, 0, 2
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
        state.hand, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
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
        state.hand, SelectionPurpose::EXHAUST_FROM_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    size_t initialExhaustPile = state.exhaustPile.size();
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.exhaustPile.size() > initialExhaustPile, 
        "Card should be in exhaust pile after EXHAUST_FROM_HAND routing");
}

void test_SelectionPurpose_Routing_DiscardFromHand() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    state.hand.push_back(card);
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        state.hand, SelectionPurpose::DISCARD_FROM_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    size_t initialDiscardPile = state.discardPile.size();
    
    PlayerActions::chooseCard(state, flow, 0);
    
    TEST_ASSERT(state.discardPile.size() > initialDiscardPile, 
        "Card should be in discard pile after DISCARD_FROM_HAND routing");
}

void test_SelectionPurpose_Routing_MoveToHand() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto card = std::make_shared<StrikeCard>();
    state.discardPile.push_back(card);
    
    std::vector<std::shared_ptr<AbstractCard>> sourcePile = {card};
    
    state.addAction(std::make_unique<RequestCardSelectionAction>(
        sourcePile, SelectionPurpose::MOVE_TO_HAND, 1, 1
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
        state.hand, SelectionPurpose::EXHAUST_FROM_HAND, 1, 5
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
        state.hand, SelectionPurpose::MOVE_TO_HAND, 1, 1
    ));
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT(state.selectionCtx.has_value(), "selectionCtx should have value");
    TEST_ASSERT(state.selectionCtx->purpose == SelectionPurpose::MOVE_TO_HAND,
        "Purpose should be preserved in selectionCtx");
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
    
    suite.printReport();
}

}
