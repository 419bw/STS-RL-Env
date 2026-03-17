#include "src/test/TestFramework.h"
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/relic/AbstractRelic.h"
#include "src/character/Character.h"
#include "src/core/Queries.h"
#include "src/rules/BasicRules.h"
#include "src/system/ActionSystem.h"
#include <cmath>

using namespace TestFramework;

namespace VulnerableRelicTests {

GameState createTestState() {
    GameState state;
    state.enableLogging = false;
    state.monsters.push_back(std::make_shared<Monster>("测试怪物", 100));
    BasicRules::registerRules(state);
    return state;
}

void test_VulnerablePower_BaseMultiplier() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    float baseDamage = 10.0f;
    float modifiedDamage = monster->getPower("易伤")->modifyDamageTaken(baseDamage, state.player.get());
    
    TEST_ASSERT_NEAR(modifiedDamage, 15.0f, 0.01f, 
        "Base vulnerable multiplier should be 1.5x");
}

void test_VulnerablePower_ZeroStacksNoEffect() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(0);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    float baseDamage = 10.0f;
    float modifiedDamage = monster->getPower("易伤")->modifyDamageTaken(baseDamage, state.player.get());
    
    TEST_ASSERT_NEAR(modifiedDamage, 10.0f, 0.01f, 
        "Vulnerable with 0 stacks should not modify damage");
}

void test_VulnerablePower_NegativeStacksNoEffect() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(-1);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    float baseDamage = 10.0f;
    float modifiedDamage = monster->getPower("易伤")->modifyDamageTaken(baseDamage, state.player.get());
    
    TEST_ASSERT_NEAR(modifiedDamage, 10.0f, 0.01f, 
        "Vulnerable with negative stacks should not modify damage");
}

class TestAttackerBuffRelic : public AbstractRelic {
public:
    float bonusMultiplier = 0.25f;
    
    TestAttackerBuffRelic() : AbstractRelic("测试攻击者增益遗物") {}
    
    void onEquip(GameState& state, Character* target) override {
        AbstractRelic::onEquip(state, target);
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
    
    void onEquip(GameState& state, Character* target) override {
        AbstractRelic::onEquip(state, target);
    }
    
    void onQuery(VulnerableMultiplierQuery& query) override {
        if (query.target && query.target == getOwner()) {
            query.multiplier += penaltyMultiplier;
        }
    }
};

void test_Relic_ModifiesVulnerableMultiplier_AttackerSide() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    state.player->addRelic(attackerRelic, state);
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    float baseDamage = 10.0f;
    float modifiedDamage = monster->getPower("易伤")->modifyDamageTaken(baseDamage, state.player.get());
    
    float expected = 10.0f * (1.5f + 0.25f);
    TEST_ASSERT_NEAR(modifiedDamage, expected, 0.01f, 
        "Attacker relic should increase vulnerable multiplier");
}

void test_Relic_ModifiesVulnerableMultiplier_DefenderSide() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    auto defenderRelic = std::make_shared<TestDefenderNerfRelic>();
    monster->addRelic(defenderRelic, state);
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    float baseDamage = 10.0f;
    float modifiedDamage = monster->getPower("易伤")->modifyDamageTaken(baseDamage, state.player.get());
    
    float expected = 10.0f * (1.5f - 0.25f);
    TEST_ASSERT_NEAR(modifiedDamage, expected, 0.01f, 
        "Defender relic should decrease vulnerable multiplier");
}

void test_MultipleRelics_BothSides() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    state.player->addRelic(attackerRelic, state);
    
    auto monster = state.monsters[0];
    auto defenderRelic = std::make_shared<TestDefenderNerfRelic>();
    monster->addRelic(defenderRelic, state);
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    float baseDamage = 10.0f;
    float modifiedDamage = monster->getPower("易伤")->modifyDamageTaken(baseDamage, state.player.get());
    
    float expected = 10.0f * (1.5f + 0.25f - 0.25f);
    TEST_ASSERT_NEAR(modifiedDamage, expected, 0.01f, 
        "Both attacker and defender relics should stack");
}

void test_MultipleAttackerRelics_Stack() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto relic1 = std::make_shared<TestAttackerBuffRelic>();
    relic1->name = "测试攻击者增益遗物1";
    relic1->bonusMultiplier = 0.25f;
    state.player->addRelic(relic1, state);
    
    auto relic2 = std::make_shared<TestAttackerBuffRelic>();
    relic2->name = "测试攻击者增益遗物2";
    relic2->bonusMultiplier = 0.25f;
    state.player->addRelic(relic2, state);
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    float baseDamage = 10.0f;
    float modifiedDamage = monster->getPower("易伤")->modifyDamageTaken(baseDamage, state.player.get());
    
    float expected = 10.0f * (1.5f + 0.25f + 0.25f);
    TEST_ASSERT_NEAR(modifiedDamage, expected, 0.01f, 
        "Multiple attacker relics should stack additively");
}

void test_NoVulnerable_NoMultiplierApplied() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    state.player->addRelic(attackerRelic, state);
    
    auto monster = state.monsters[0];
    
    int initialHp = monster->current_hp;
    
    state.addAction(std::make_unique<DamageAction>(state.player, monster, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int expectedDamage = 10;
    int actualDamage = initialHp - monster->current_hp;
    TEST_ASSERT_EQ(actualDamage, expectedDamage, 
        "Damage should be base value when no vulnerable is applied");
}

void test_VulnerableAffectsActualDamage() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    int initialHp = monster->current_hp;
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    state.addAction(std::make_unique<DamageAction>(state.player, monster, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int expectedDamage = 15;
    int actualDamage = initialHp - monster->current_hp;
    TEST_ASSERT_EQ(actualDamage, expectedDamage, 
        "Actual damage should be increased by vulnerable");
}

void test_VulnerableStacking_IncreasesDuration() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    
    auto vulnerable1 = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerable1));
    ActionSystem::executeUntilBlocked(state, flow);
    
    auto vulnerable2 = std::make_shared<VulnerablePower>(3);
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerable2));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int totalStacks = 0;
    monster->forEachPower([&](const std::shared_ptr<AbstractPower>& power) {
        if (power->name == "易伤") {
            totalStacks = power->getAmount();
        }
    });
    
    TEST_ASSERT_EQ(totalStacks, 5, "Vulnerable should stack by adding amounts");
}

void test_VulnerableJustApplied_ProtectedFromReduction() {
    GameState state = createTestState();
    CombatFlow flow;
    
    state.isPlayerTurn = false;
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.monsters[0], monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    auto appliedPower = monster->getPower("易伤");
    TEST_ASSERT(appliedPower && appliedPower->isJustApplied(), 
        "Newly applied vulnerable should have justApplied = true when applied by monster during monster turn");
}

void test_QuerySystem_ZeroOverhead() {
    GameState state = createTestState();
    
    VulnerableMultiplierQuery query{state.player.get(), state.monsters[0].get()};
    
    TEST_ASSERT_NEAR(query.multiplier, 1.5f, 0.01f, 
        "Default query multiplier should be 1.5");
    
    state.player->processQuery(query);
    state.monsters[0]->processQuery(query);
    
    TEST_ASSERT_NEAR(query.multiplier, 1.5f, 0.01f, 
        "Query multiplier should remain 1.5 when no relics modify it");
}

void test_RelicOnlyAffectsOwnSide() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    state.player->addRelic(attackerRelic, state);
    
    auto monster = state.monsters[0];
    
    VulnerableMultiplierQuery query{state.player.get(), monster.get()};
    
    query.multiplier = 1.5f;
    state.player->processQuery(query);
    float afterAttacker = query.multiplier;
    
    query.multiplier = 1.5f;
    monster->processQuery(query);
    float afterDefender = query.multiplier;
    
    TEST_ASSERT_NEAR(afterAttacker, 1.75f, 0.01f, 
        "Attacker relic should modify when source matches");
    TEST_ASSERT_NEAR(afterDefender, 1.5f, 0.01f, 
        "Attacker relic should not modify when source doesn't match");
}

void test_DamageCalculation_WithBlock() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    monster->addBlockFinal(5);
    int initialHp = monster->current_hp;
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    state.addAction(std::make_unique<DamageAction>(state.player, monster, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int expectedHpDamage = 10;
    
    TEST_ASSERT_EQ(monster->block, 0, "Block should be consumed");
    int actualHpDamage = initialHp - monster->current_hp;
    TEST_ASSERT_EQ(actualHpDamage, expectedHpDamage, 
        "Excess damage after block should be affected by vulnerable (15 - 5 = 10)");
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
    RUN_TEST(suite, test_VulnerableJustApplied_ProtectedFromReduction);
    RUN_TEST(suite, test_QuerySystem_ZeroOverhead);
    RUN_TEST(suite, test_RelicOnlyAffectsOwnSide);
    RUN_TEST(suite, test_DamageCalculation_WithBlock);
    
    suite.printReport();
}

}
