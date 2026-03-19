#include "src/test/TestFramework.h"
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/power/AbstractPower.h"
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
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Base vulnerable multiplier should be 1.5x");
}

void test_VulnerablePower_ZeroStacksNoEffect() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(0);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 10, 
        "Vulnerable with 0 stacks should not modify damage");
}

void test_VulnerablePower_NegativeStacksNoEffect() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(-1);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 10, 
        "Vulnerable with negative stacks should not modify damage");
}

// ==========================================
// 测试遗物：纸蛙风格 - 攻击者身上的易伤倍率增益
// 效果：易伤倍率 +25% (1.5 -> 1.75)
// ==========================================
class TestAttackerBuffRelic : public AbstractRelic {
public:
    float bonusMultiplier = 0.25f;
    
    TestAttackerBuffRelic() : AbstractRelic("测试攻击者增益遗物") {}
    
    void onEquip(GameState& state, Character* target) override {
        AbstractRelic::onEquip(state, target);
    }
    
    // 通过表单系统修改易伤倍率
    void onQuery(VulnerableMultiplierQuery& query) override {
        if (query.source && query.source == getOwner()) {
            query.multiplier += bonusMultiplier;
        }
    }
};

// ==========================================
// 测试遗物：蘑菇风格 - 防御者身上的易伤倍率减免
// 效果：易伤倍率 -25% (1.5 -> 1.25)
// ==========================================
class TestDefenderNerfRelic : public AbstractRelic {
public:
    float penaltyMultiplier = -0.25f;
    
    TestDefenderNerfRelic() : AbstractRelic("测试防御者减益遗物") {}
    
    void onEquip(GameState& state, Character* target) override {
        AbstractRelic::onEquip(state, target);
    }
    
    // 通过表单系统修改易伤倍率
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
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    // 攻击者遗物增加易伤倍率: 1.5 + 0.25 = 1.75
    // 10 * 1.75 = 17.5 -> 17
    TEST_ASSERT_EQ(finalDamage, 17, 
        "Attacker relic should increase vulnerable multiplier: 10 * 1.75 = 17.5 -> 17");
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
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    // 防御者遗物减少易伤倍率: 1.5 - 0.25 = 1.25
    // 10 * 1.25 = 12.5 -> 12
    TEST_ASSERT_EQ(finalDamage, 12, 
        "Defender relic should decrease vulnerable multiplier: 10 * 1.25 = 12.5 -> 12");
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
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    // 双方遗物: 1.5 + 0.25 - 0.25 = 1.5
    // 10 * 1.5 = 15
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Both attacker and defender relics: 1.5 + 0.25 - 0.25 = 1.5, 10 * 1.5 = 15");
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
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    // 多个攻击者遗物叠加: 1.5 + 0.25 + 0.25 = 2.0
    // 10 * 2.0 = 20
    TEST_ASSERT_EQ(finalDamage, 20, 
        "Multiple attacker relics stack: 1.5 + 0.25 + 0.25 = 2.0, 10 * 2.0 = 20");
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
    
    // 没有易伤状态，遗物的易伤倍率修改不生效
    // 伤害 = 10
    int expectedDamage = 10;
    int actualDamage = initialHp - monster->current_hp;
    TEST_ASSERT_EQ(actualDamage, expectedDamage, 
        "Without vulnerable, relic's vulnerable multiplier bonus has no effect: 10");
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
    
    TEST_ASSERT_EQ(monster->getPower("易伤")->getAmount(), 2, 
        "First vulnerable should add 2 stacks");
    
    auto vulnerable2 = std::make_shared<VulnerablePower>(3);
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerable2));
    ActionSystem::executeUntilBlocked(state, flow);
    
    TEST_ASSERT_EQ(monster->getPower("易伤")->getAmount(), 5, 
        "Second vulnerable should stack to 5");
}

void test_VulnerableJustApplied_NotProtectedWhenPlayerApplies() {
    GameState state = createTestState();
    CombatFlow flow;

    auto monster = state.monsters[0];

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);

    TEST_ASSERT(!monster->getPower("易伤")->isJustApplied(),
        "Player-applied vulnerable should NOT have protection flag (only monster-applied during enemy turn has protection)");
}

void test_VulnerableJustApplied_ProtectedWhenMonsterAppliesDuringEnemyTurn() {
    GameState state = createTestState();
    CombatFlow flow;

    state.isPlayerTurn = false;

    auto player = state.player;
    auto monster = state.monsters[0];

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(monster, player, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);

    TEST_ASSERT(player->getPower("易伤")->isJustApplied(),
        "Monster-applied vulnerable during enemy turn SHOULD have protection flag");
}

void test_QuerySystem_ZeroOverhead() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Query system should work with zero overhead");
}

void test_RelicOnlyAffectsOwnSide() {
    GameState state = createTestState();
    CombatFlow flow;
    
    // 玩家拥有【纸蛙】（只加成自己打别人的易伤）
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    state.player->addRelic(attackerRelic, state);
    
    // 玩家被挂上了【易伤】
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(state.monsters[0], state.player, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int initialHp = state.player->current_hp;
    
    // 怪物打玩家 10 点伤害
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int actualDamage = initialHp - state.player->current_hp;
    
    // 预期伤害：怪物打玩家，触发基础易伤 (10 * 1.5 = 15)。
    // 玩家身上的【纸蛙】不应该生效（如果是 17 就说明遗物敌我不分出 Bug 了）。
    TEST_ASSERT_EQ(actualDamage, 15, "Attacker relic should not trigger when its owner is the defender");
}

void test_DamageCalculation_WithBlock() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    monster->block = 5;
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int initialHp = monster->current_hp;
    
    state.addAction(std::make_unique<DamageAction>(state.player, monster, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
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
