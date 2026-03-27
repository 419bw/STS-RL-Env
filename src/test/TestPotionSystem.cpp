#include "src/test/TestFramework.h"
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/potion/AbstractPotion.h"
#include "src/potion/Potions.h"
#include "src/power/Powers.h"
#include "src/character/Character.h"
#include "src/rules/BasicRules.h"
#include "src/system/ActionSystem.h"

namespace PotionSystemTests {

GameState createTestState();

void test_StrengthPotion_DefaultId();
void test_StrengthPotion_DefaultTarget();
void test_StrengthPotion_UseAddsAction();
void test_UsePotion_WrongPhase_ReturnsFalse();
void test_UsePotion_NotPlayerTurn_ReturnsFalse();
void test_UsePotion_PotionNotFound_ReturnsFalse();
void test_UsePotion_SelfTarget_RoutesToPlayer();
void test_UsePotion_EnemyTarget_BlocksSelfTarget();
void test_UsePotion_RemovesPotionFromState();

GameState createTestState() {
    GameState state(1337);
    state.enableLogging = false;
    state.isPlayerTurn = true;
    state.currentPhase = StatePhase::PLAYING_CARD;
    state.player = std::make_shared<Player>("TestPlayer", 80);
    BasicRules::registerRules(state);
    return state;
}

void runAllTests() {
    TestFramework::TestSuite suite("药水系统测试");

    RUN_TEST(suite, test_StrengthPotion_DefaultId);
    RUN_TEST(suite, test_StrengthPotion_DefaultTarget);
    RUN_TEST(suite, test_StrengthPotion_UseAddsAction);
    RUN_TEST(suite, test_UsePotion_WrongPhase_ReturnsFalse);
    RUN_TEST(suite, test_UsePotion_NotPlayerTurn_ReturnsFalse);
    RUN_TEST(suite, test_UsePotion_PotionNotFound_ReturnsFalse);
    RUN_TEST(suite, test_UsePotion_SelfTarget_RoutesToPlayer);
    RUN_TEST(suite, test_UsePotion_EnemyTarget_BlocksSelfTarget);
    RUN_TEST(suite, test_UsePotion_RemovesPotionFromState);

    suite.printReport();
}

void test_StrengthPotion_DefaultId() {
    StrengthPotion potion;
    TEST_ASSERT(potion.id == "strength_potion", "StrengthPotion id should be strength_potion");
}

void test_StrengthPotion_DefaultTarget() {
    StrengthPotion potion;
    TEST_ASSERT(potion.targetType == PotionTarget::SELF, "StrengthPotion target should be SELF");
}

void test_StrengthPotion_UseAddsAction() {
    GameState state = createTestState();
    auto potion = std::make_shared<StrengthPotion>();
    state.potions.push_back(potion);

    auto monster = std::make_shared<Monster>("TestMonster", 30);
    state.monsters.push_back(monster);

    potion->use(state, state.player);

    TEST_ASSERT(!state.isActionQueueEmpty(), "use() should add at least one action");
}

void test_UsePotion_WrongPhase_ReturnsFalse() {
    GameState state = createTestState();
    state.currentPhase = StatePhase::WAITING_FOR_CARD_SELECTION;
    auto potion = std::make_shared<StrengthPotion>();
    state.potions.push_back(potion);

    CombatFlow flow;
    bool result = PlayerActions::usePotion(state, flow, potion, state.player);
    TEST_ASSERT(result == false, "usePotion should return false when phase is not PLAYING_CARD");
}

void test_UsePotion_NotPlayerTurn_ReturnsFalse() {
    GameState state = createTestState();
    state.isPlayerTurn = false;
    auto potion = std::make_shared<StrengthPotion>();
    state.potions.push_back(potion);

    CombatFlow flow;
    bool result = PlayerActions::usePotion(state, flow, potion, state.player);
    TEST_ASSERT(result == false, "usePotion should return false when not player turn");
}

void test_UsePotion_PotionNotFound_ReturnsFalse() {
    GameState state = createTestState();
    auto potion = std::make_shared<StrengthPotion>();

    CombatFlow flow;
    bool result = PlayerActions::usePotion(state, flow, potion, state.player);
    TEST_ASSERT(result == false, "usePotion should return false when potion not in list");
}

void test_UsePotion_SelfTarget_RoutesToPlayer() {
    GameState state = createTestState();
    auto potion = std::make_shared<StrengthPotion>();
    potion->targetType = PotionTarget::SELF;
    state.potions.push_back(potion);

    potion->use(state, nullptr);

    TEST_ASSERT(!state.isActionQueueEmpty(), "SELF target should route to player");
}

void test_UsePotion_EnemyTarget_BlocksSelfTarget() {
    GameState state = createTestState();
    auto potion = std::make_shared<StrengthPotion>();
    potion->targetType = PotionTarget::ENEMY;
    state.potions.push_back(potion);

    auto monster = std::make_shared<Monster>("TestMonster", 30);
    state.monsters.push_back(monster);

    CombatFlow flow;
    bool result = PlayerActions::usePotion(state, flow, potion, state.player);

    TEST_ASSERT(result == false, "ENEMY target should block self-targeting");
}

void test_UsePotion_RemovesPotionFromState() {
    GameState state = createTestState();
    auto potion = std::make_shared<StrengthPotion>();
    state.potions.push_back(potion);

    CombatFlow flow;
    PlayerActions::usePotion(state, flow, potion, state.player);

    bool found = false;
    for (const auto& p : state.potions) {
        if (p == potion) {
            found = true;
            break;
        }
    }
    TEST_ASSERT(!found, "Potion should be removed from state after use");
}

}
