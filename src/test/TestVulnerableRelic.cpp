#include "src/test/TestFramework.h"
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/power/AbstractPower.h"
#include "src/relic/AbstractRelic.h"
#include "src/character/Character.h"
#include "src/core/Queries.h"
#include "src/rules/BasicRules.h"
#include <cmath>

using namespace TestFramework;

namespace VulnerableRelicTests {

GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("测试怪物", 100));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

void test_VulnerablePower_BaseMultiplier() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Base vulnerable multiplier should be 1.5x");
}

void test_VulnerablePower_ZeroStacksNoEffect() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(0);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 10, 
        "Vulnerable with 0 stacks should not modify damage");
}

void test_VulnerablePower_NegativeStacksNoEffect() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(-1);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 10, 
        "Vulnerable with negative stacks should not modify damage");
}

class TestAttackerBuffRelic : public AbstractRelic {
public:
    float bonusMultiplier = 0.25f;
    
    TestAttackerBuffRelic() : AbstractRelic("测试攻击者增益遗物") {}
    
    void onEquip(GameEngine& engine, Character* target) override {
        AbstractRelic::onEquip(engine, target);
    }
    
    void onQuery(VulnerableMultiplierQuery& query) override {
        if (query.source && query.source == getOwner()) {
            query.multiplier += bonusMultiplier;
        }
    }
};

class TestDefenderNerfRelic : public AbstractRelic {
public:
    float penaltyMultiplier = -0.25f;
    
    TestDefenderNerfRelic() : AbstractRelic("测试防御者减益遗物") {}
    
    void onEquip(GameEngine& engine, Character* target) override {
        AbstractRelic::onEquip(engine, target);
    }
    
    void onQuery(VulnerableMultiplierQuery& query) override {
        if (query.target && query.target == getOwner()) {
            query.multiplier += penaltyMultiplier;
        }
    }
};

void test_Relic_ModifiesVulnerableMultiplier_AttackerSide() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);
    
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 17, 
        "Attacker relic should increase vulnerable multiplier: 10 * 1.75 = 17.5 -> 17");
}

void test_Relic_ModifiesVulnerableMultiplier_DefenderSide() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    auto defenderRelic = std::make_shared<TestDefenderNerfRelic>();
    monster->addRelic(defenderRelic, engine);
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 12, 
        "Defender relic should decrease vulnerable multiplier: 10 * 1.25 = 12.5 -> 12");
}

void test_MultipleRelics_BothSides() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);
    
    auto monster = engine.combatState->monsters[0];
    auto defenderRelic = std::make_shared<TestDefenderNerfRelic>();
    monster->addRelic(defenderRelic, engine);
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Both attacker and defender relics: 1.5 + 0.25 - 0.25 = 1.5, 10 * 1.5 = 15");
}

void test_MultipleAttackerRelics_Stack() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto relic1 = std::make_shared<TestAttackerBuffRelic>();
    relic1->name = "测试攻击者增益遗物1";
    relic1->bonusMultiplier = 0.25f;
    engine.combatState->player->addRelic(relic1, engine);
    
    auto relic2 = std::make_shared<TestAttackerBuffRelic>();
    relic2->name = "测试攻击者增益遗物2";
    relic2->bonusMultiplier = 0.25f;
    engine.combatState->player->addRelic(relic2, engine);
    
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 20, 
        "Multiple attacker relics stack: 1.5 + 0.25 + 0.25 = 2.0, 10 * 2.0 = 20");
}

void test_NoVulnerable_NoMultiplierApplied() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);
    
    auto monster = engine.combatState->monsters[0];
    
    int initialHp = monster->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->player, monster, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int expectedDamage = 10;
    int actualDamage = initialHp - monster->current_hp;
    TEST_ASSERT_EQ(actualDamage, expectedDamage, 
        "Without vulnerable, relic's vulnerable multiplier bonus has no effect: 10");
}

void test_VulnerableAffectsActualDamage() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    int initialHp = monster->current_hp;
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->player, monster, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int expectedDamage = 15;
    int actualDamage = initialHp - monster->current_hp;
    TEST_ASSERT_EQ(actualDamage, expectedDamage, 
        "Actual damage should be increased by vulnerable");
}

void test_VulnerableStacking_IncreasesDuration() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    
    auto vulnerable1 = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerable1));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT_EQ(monster->getPower("易伤")->getAmount(), 2, 
        "First vulnerable should add 2 stacks");
    
    auto vulnerable2 = std::make_shared<VulnerablePower>(3);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerable2));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    TEST_ASSERT_EQ(monster->getPower("易伤")->getAmount(), 5, 
        "Second vulnerable should stack to 5");
}

void test_VulnerableJustApplied_NotProtectedWhenPlayerApplies() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto monster = engine.combatState->monsters[0];

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    TEST_ASSERT(!monster->getPower("易伤")->isJustApplied(),
        "Player-applied vulnerable should NOT have protection flag (only monster-applied during enemy turn has protection)");
}

void test_VulnerableJustApplied_ProtectedWhenMonsterAppliesDuringEnemyTurn() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    engine.combatState->isPlayerTurn = false;

    auto player = engine.combatState->player;
    auto monster = engine.combatState->monsters[0];

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(monster, player, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    TEST_ASSERT(player->getPower("易伤")->isJustApplied(),
        "Monster-applied vulnerable during enemy turn SHOULD have protection flag");
}

void test_QuerySystem_ZeroOverhead() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Query system should work with zero overhead");
}

void test_RelicOnlyAffectsOwnSide() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->monsters[0], engine.combatState->player, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualDamage = initialHp - engine.combatState->player->current_hp;
    
    TEST_ASSERT_EQ(actualDamage, 15, "Attacker relic should not trigger when its owner is the defender");
}

void test_DamageCalculation_WithBlock() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    engine.actionManager.addAction(std::make_unique<GainBlockAction>(monster, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int initialHp = monster->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->player, monster, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int expectedDamage = 15;
    int expectedBlockDamage = 5;
    int expectedHpDamage = expectedDamage - expectedBlockDamage;
    
    TEST_ASSERT_EQ(monster->block, 0, "Block should be depleted");
    TEST_ASSERT_EQ(initialHp - monster->current_hp, expectedHpDamage, 
        "HP damage should be (15-5)=10 after block");
}

void runAllTests() {
    TestSuite suite("易伤遗物倍率修改功能测试");
    
    RUN_TEST(suite, test_VulnerablePower_BaseMultiplier);
    RUN_TEST(suite, test_VulnerablePower_ZeroStacksNoEffect);
    RUN_TEST(suite, test_VulnerablePower_NegativeStacksNoEffect);
    RUN_TEST(suite, test_Relic_ModifiesVulnerableMultiplier_AttackerSide);
    RUN_TEST(suite, test_Relic_ModifiesVulnerableMultiplier_DefenderSide);
    RUN_TEST(suite, test_MultipleRelics_BothSides);
    RUN_TEST(suite, test_MultipleAttackerRelics_Stack);
    RUN_TEST(suite, test_NoVulnerable_NoMultiplierApplied);
    RUN_TEST(suite, test_VulnerableAffectsActualDamage);
    RUN_TEST(suite, test_VulnerableStacking_IncreasesDuration);
    RUN_TEST(suite, test_VulnerableJustApplied_NotProtectedWhenPlayerApplies);
    RUN_TEST(suite, test_VulnerableJustApplied_ProtectedWhenMonsterAppliesDuringEnemyTurn);
    RUN_TEST(suite, test_QuerySystem_ZeroOverhead);
    RUN_TEST(suite, test_RelicOnlyAffectsOwnSide);
    RUN_TEST(suite, test_DamageCalculation_WithBlock);
    
    suite.printReport();
}

}
