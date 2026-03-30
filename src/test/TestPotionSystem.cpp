#include "src/test/TestFramework.h"
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/potion/AbstractPotion.h"
#include "src/potion/Potions.h"
#include "src/power/Powers.h"
#include "src/character/Character.h"
#include "src/rules/BasicRules.h"

namespace PotionSystemTests {

GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("TestMonster", 30));
    engine.combatState->enableLogging = false;
    engine.combatState->isPlayerTurn = true;
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;
    BasicRules::registerRules(engine);
    return engine;
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
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();
    engine.runState->potions.push_back(potion);

    auto monster = std::make_shared<Monster>("TestMonster", 30);
    engine.combatState->monsters.push_back(monster);

    potion->use(engine, engine.combatState->player);

    TEST_ASSERT(!engine.actionManager.isQueueEmpty(), "use() should add at least one action");
}

void test_UsePotion_WrongPhase_ReturnsFalse() {
    GameEngine engine = createTestEngine();
    engine.combatState->currentPhase = StatePhase::WAITING_FOR_CARD_SELECTION;
    auto potion = std::make_shared<StrengthPotion>();
    engine.runState->potions.push_back(potion);

    CombatFlow flow;
    bool result = PlayerActions::usePotion(engine, flow, potion, engine.combatState->player);
    TEST_ASSERT(result == false, "usePotion should return false when phase is not PLAYING_CARD");
}

void test_UsePotion_NotPlayerTurn_ReturnsFalse() {
    GameEngine engine = createTestEngine();
    engine.combatState->isPlayerTurn = false;
    auto potion = std::make_shared<StrengthPotion>();
    engine.runState->potions.push_back(potion);

    CombatFlow flow;
    bool result = PlayerActions::usePotion(engine, flow, potion, engine.combatState->player);
    TEST_ASSERT(result == false, "usePotion should return false when not player turn");
}

void test_UsePotion_PotionNotFound_ReturnsFalse() {
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();

    CombatFlow flow;
    bool result = PlayerActions::usePotion(engine, flow, potion, engine.combatState->player);
    TEST_ASSERT(result == false, "usePotion should return false when potion not in list");
}

void test_UsePotion_SelfTarget_RoutesToPlayer() {
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();
    potion->targetType = PotionTarget::SELF;
    engine.runState->potions.push_back(potion);

    potion->use(engine, nullptr);

    TEST_ASSERT(!engine.actionManager.isQueueEmpty(), "SELF target should route to player");
}

void test_UsePotion_EnemyTarget_BlocksSelfTarget() {
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();
    potion->targetType = PotionTarget::ENEMY;
    engine.runState->potions.push_back(potion);

    auto monster = std::make_shared<Monster>("TestMonster", 30);
    engine.combatState->monsters.push_back(monster);

    CombatFlow flow;
    bool result = PlayerActions::usePotion(engine, flow, potion, engine.combatState->player);

    TEST_ASSERT(result == false, "ENEMY target should block self-targeting");
}

void test_UsePotion_RemovesPotionFromState() {
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();
    engine.runState->potions.push_back(potion);

    CombatFlow flow;
    PlayerActions::usePotion(engine, flow, potion, engine.combatState->player);

    bool found = false;
    for (const auto& p : engine.runState->potions) {
        if (p == potion) {
            found = true;
            break;
        }
    }
    TEST_ASSERT(!found, "Potion should be removed from state after use");
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

}
