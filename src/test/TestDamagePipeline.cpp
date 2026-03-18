#include "src/test/TestFramework.h"
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/power/AbstractPower.h"
#include "src/relic/Relics.h"
#include "src/relic/AbstractRelic.h"
#include "src/character/Character.h"
#include "src/rules/BasicRules.h"
#include "src/system/ActionSystem.h"
#include <cmath>

using namespace TestFramework;

namespace DamagePipelineTests {

GameState createTestState() {
    GameState state;
    state.enableLogging = false;
    state.monsters.push_back(std::make_shared<Monster>("测试怪物", 100));
    BasicRules::registerRules(state);
    return state;
}

// ==========================================
// 测试用的力量状态
// ==========================================
class TestStrengthPower : public AbstractPower {
public:
    TestStrengthPower(int amount) 
        : AbstractPower("力量", amount, PowerType::BUFF) {}
    
    float atDamageGive(float damage, DamageType type) override {
        if (type == DamageType::ATTACK) {
            return damage + getAmount();
        }
        return damage;
    }
};

// ==========================================
// 测试用的虚弱状态
// ==========================================
class TestWeakPower : public AbstractPower {
public:
    TestWeakPower(int amount) 
        : AbstractPower("虚弱", amount, PowerType::DEBUFF) {}
    
    float atDamageGive(float damage, DamageType type) override {
        if (type == DamageType::ATTACK) {
            return damage * 0.75f;
        }
        return damage;
    }
};

// ==========================================
// 测试用的无实体状态
// ==========================================
class TestIntangiblePower : public AbstractPower {
public:
    TestIntangiblePower(int amount) 
        : AbstractPower("无实体", amount, PowerType::BUFF) {}
    
    float atDamageFinalReceive(float damage, DamageType type) override {
        if (damage > 1.0f) {
            return 1.0f;
        }
        return damage;
    }
};

// ==========================================
// 测试用的攻击者增益遗物
// ==========================================
class TestAttackerBuffRelic : public AbstractRelic {
public:
    float bonusMultiplier = 0.25f;
    
    TestAttackerBuffRelic() : AbstractRelic("测试攻击者增益遗物") {}
    
    float atDamageGive(float damage, DamageType type) override {
        return damage * (1.0f + bonusMultiplier);
    }
};

// ==========================================
// 基础伤害计算测试
// ==========================================

void test_BasicDamage_NoModifiers() {
    GameState state = createTestState();
    
    auto monster = state.monsters[0];
    int baseDamage = 10;
    
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 10, 
        "Damage should be unchanged with no modifiers");
}

void test_VulnerablePower_IncreasesDamage() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Vulnerable should increase damage by 50%");
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

// ==========================================
// 四阶段管线测试
// ==========================================

void test_Stage1_AttackerPower_ModifiesDamage() {
    GameState state = createTestState();
    
    auto strengthPower = std::make_shared<TestStrengthPower>(3);
    state.player->addPower(strengthPower);
    
    auto monster = state.monsters[0];
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 13, 
        "Strength should add to damage in stage 1");
}

void test_Stage1_AttackerRelic_ModifiesDamage() {
    GameState state = createTestState();
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    state.player->addRelic(attackerRelic, state);
    
    auto monster = state.monsters[0];
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 12, 
        "Attacker relic should modify damage in stage 1 (10 * 1.25 = 12.5 -> 12)");
}

void test_Stage2_DefenderPower_ModifiesDamage() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Vulnerable should multiply damage in stage 2");
}

void test_Stage4_DefenderPower_ModifiesDamage() {
    GameState state = createTestState();
    
    auto monster = state.monsters[0];
    auto intangiblePower = std::make_shared<TestIntangiblePower>(1);
    monster->addPower(intangiblePower);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 1, 
        "Intangible should cap damage at 1 in stage 4");
}

void test_AllStages_CombineCorrectly() {
    GameState state = createTestState();
    CombatFlow flow;
    
    // 攻击者有力量 +3
    auto strengthPower = std::make_shared<TestStrengthPower>(3);
    state.player->addPower(strengthPower);
    
    // 防御者有易伤
    auto monster = state.monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 计算: 10 + 3 = 13 (阶段1) -> 13 * 1.5 = 19.5 -> 19 (阶段2)
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, state.player.get());
    
    TEST_ASSERT_EQ(finalDamage, 19, 
        "All stages should combine: (10+3)*1.5 = 19.5 -> 19");
}

// ==========================================
// 鸟居遗物测试
// 
// 注意：鸟居在实际扣血阶段生效（护甲之后）
// ==========================================

void test_ToriiRelic_DamageBelow5ReducedTo1() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    state.player->addRelic(toriiRelic, state);
    
    int initialHp = state.player->current_hp;
    
    // 受到 3 点伤害，没有护甲
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 3));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 鸟居：3 -> 1
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 1, 
        "Torii should reduce actual HP loss 3 to 1");
}

void test_ToriiRelic_DamageAbove5Unchanged() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    state.player->addRelic(toriiRelic, state);
    
    int initialHp = state.player->current_hp;
    
    // 受到 10 点伤害，没有护甲
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 鸟居不触发：10 > 5
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 10, 
        "Torii should not affect damage > 5");
}

void test_ToriiRelic_DamageExactly5ReducedTo1() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    state.player->addRelic(toriiRelic, state);
    
    int initialHp = state.player->current_hp;
    
    // 受到 5 点伤害
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 5));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 鸟居：5 -> 1
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 1, 
        "Torii should reduce HP loss 5 to 1");
}

void test_ToriiRelic_Damage1Unchanged() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    state.player->addRelic(toriiRelic, state);
    
    int initialHp = state.player->current_hp;
    
    // 受到 1 点伤害
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 1));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 鸟居：1 -> 1（不变）
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 1, 
        "Torii should keep HP loss 1 as 1");
}

void test_ToriiRelic_NoTriggerWithBlock() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    state.player->addRelic(toriiRelic, state);
    
    // 给玩家 10 点护甲
    state.player->block = 10;
    int initialHp = state.player->current_hp;
    
    // 受到 5 点伤害，护甲完全吸收
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 5));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 鸟居不触发：没有实际扣血
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Torii should not trigger when block absorbs all damage");
    TEST_ASSERT_EQ(state.player->block, 5, 
        "Block should be reduced to 5");
}

void test_ToriiRelic_TriggerAfterBlock() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    state.player->addRelic(toriiRelic, state);
    
    // 给玩家 3 点护甲
    state.player->block = 3;
    int initialHp = state.player->current_hp;
    
    // 受到 8 点伤害，护甲吸收 3，实际扣血 5
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 8));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 鸟居触发：实际扣血 5 -> 1
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 1, 
        "Torii should reduce actual HP loss 5 to 1 after block");
    TEST_ASSERT_EQ(state.player->block, 0, 
        "Block should be depleted");
}

// ==========================================
// 钨合金棍遗物测试
// 
// 注意：钨合金棍在实际扣血阶段生效（护甲之后）
// ==========================================

void test_TungstenRodReducesDamageBy1() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    state.player->addRelic(tungstenRod, state);
    
    int initialHp = state.player->current_hp;
    
    // 受到 10 点伤害
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 钨合金棍：10 - 1 = 9
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 9, 
        "Tungsten Rod should reduce actual HP loss by 1");
}

void test_TungstenRod_Damage1ReducedTo0() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    state.player->addRelic(tungstenRod, state);
    
    int initialHp = state.player->current_hp;
    
    // 受到 1 点伤害
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 1));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 钨合金棍：1 - 1 = 0
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Tungsten Rod should reduce HP loss 1 to 0");
}

void test_TungstenRod_NoTriggerWithBlock() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    state.player->addRelic(tungstenRod, state);
    
    // 给玩家 10 点护甲
    state.player->block = 10;
    int initialHp = state.player->current_hp;
    
    // 受到 5 点伤害，护甲完全吸收
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 5));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 钨合金棍不触发：没有实际扣血
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Tungsten Rod should not trigger when block absorbs all damage");
    TEST_ASSERT_EQ(state.player->block, 5, 
        "Block should be reduced to 5");
}

void test_TungstenRod_ModifiesHpLoss() {
    GameState state = createTestState();
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    state.player->addRelic(tungstenRod, state);
    
    int initialHp = state.player->current_hp;
    
    // 直接掉血 5 点
    int actualHpLost = state.player->loseHp(5);
    
    // 钨合金棍：5 - 1 = 4
    TEST_ASSERT_EQ(actualHpLost, 4, 
        "Tungsten Rod should reduce HP loss by 1");
    TEST_ASSERT_EQ(state.player->current_hp, initialHp - 4, 
        "HP should be reduced by 4");
}

void test_TungstenRod_HpLoss1ReducedTo0() {
    GameState state = createTestState();
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    state.player->addRelic(tungstenRod, state);
    
    int initialHp = state.player->current_hp;
    
    // 直接掉血 1 点
    int actualHpLost = state.player->loseHp(1);
    
    // 钨合金棍：1 - 1 = 0
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Tungsten Rod should reduce HP loss 1 to 0");
    TEST_ASSERT_EQ(state.player->current_hp, initialHp, 
        "HP should not be reduced");
}

// ==========================================
// 组合测试
// ==========================================

void test_ToriiAndTungstenRod_CombineCorrectly() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    state.player->addRelic(toriiRelic, state);
    state.player->addRelic(tungstenRod, state);
    
    int initialHp = state.player->current_hp;
    
    // 受到 5 点伤害
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 5));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 鸟居先触发：5 -> 1
    // 钨合金棍再触发：1 - 1 = 0
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Torii (5->1) then Tungsten Rod (1->0) should result in 0");
}

void test_VulnerableAndTungstenRod_CombineCorrectly() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    state.player->addRelic(tungstenRod, state);
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(state.monsters[0], state.player, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int initialHp = state.player->current_hp;
    
    // 受到 10 点伤害
    state.addAction(std::make_unique<DamageAction>(state.monsters[0], state.player, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
    // 易伤先触发：10 * 1.5 = 15
    // 钨合金棍再触发：15 - 1 = 14
    int actualHpLost = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 14, 
        "Vulnerable (10->15) then Tungsten Rod (15->14) should result in 14");
}

// ==========================================
// 无攻击者场景测试
// ==========================================

void test_NoSource_ReturnsBaseDamage() {
    GameState state = createTestState();
    
    auto monster = state.monsters[0];
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, nullptr);
    
    TEST_ASSERT_EQ(finalDamage, 10, 
        "Damage with no source should return base damage");
}

// ==========================================
// 实际伤害测试
// ==========================================

void test_ActualDamage_WithVulnerable() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto monster = state.monsters[0];
    int initialHp = monster->current_hp;
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    state.addAction(std::make_unique<ApplyPowerAction>(state.player, monster, vulnerablePower));
    ActionSystem::executeUntilBlocked(state, flow);
    
    state.addAction(std::make_unique<DamageAction>(state.player, monster, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int actualDamage = initialHp - monster->current_hp;
    TEST_ASSERT_EQ(actualDamage, 15, 
        "Actual damage should be 15 with vulnerable (10 * 1.5)");
}

void test_ActualDamage_WithTorii() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    state.player->addRelic(toriiRelic, state);
    
    int initialHp = state.player->current_hp;
    
    auto monster = state.monsters[0];
    state.addAction(std::make_unique<DamageAction>(monster, state.player, 5));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int actualDamage = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualDamage, 1, 
        "Actual damage should be 1 with Torii (5 -> 1)");
}

void test_ActualDamage_WithTungstenRod() {
    GameState state = createTestState();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    state.player->addRelic(tungstenRod, state);
    
    int initialHp = state.player->current_hp;
    
    auto monster = state.monsters[0];
    state.addAction(std::make_unique<DamageAction>(monster, state.player, 10));
    ActionSystem::executeUntilBlocked(state, flow);
    
    int actualDamage = initialHp - state.player->current_hp;
    TEST_ASSERT_EQ(actualDamage, 9, 
        "Actual damage should be 9 with Tungsten Rod (10 - 1)");
}

void runAllTests() {
    TestSuite suite("伤害计算管线测试");
    
    // 基础测试
    RUN_TEST(suite, test_BasicDamage_NoModifiers);
    RUN_TEST(suite, test_VulnerablePower_IncreasesDamage);
    RUN_TEST(suite, test_VulnerablePower_ZeroStacksNoEffect);
    
    // 四阶段测试
    RUN_TEST(suite, test_Stage1_AttackerPower_ModifiesDamage);
    RUN_TEST(suite, test_Stage1_AttackerRelic_ModifiesDamage);
    RUN_TEST(suite, test_Stage2_DefenderPower_ModifiesDamage);
    RUN_TEST(suite, test_Stage4_DefenderPower_ModifiesDamage);
    RUN_TEST(suite, test_AllStages_CombineCorrectly);
    
    // 鸟居测试
    RUN_TEST(suite, test_ToriiRelic_DamageBelow5ReducedTo1);
    RUN_TEST(suite, test_ToriiRelic_DamageAbove5Unchanged);
    RUN_TEST(suite, test_ToriiRelic_DamageExactly5ReducedTo1);
    RUN_TEST(suite, test_ToriiRelic_Damage1Unchanged);
    RUN_TEST(suite, test_ToriiRelic_NoTriggerWithBlock);
    RUN_TEST(suite, test_ToriiRelic_TriggerAfterBlock);
    
    // 钨合金棍测试
    RUN_TEST(suite, test_TungstenRodReducesDamageBy1);
    RUN_TEST(suite, test_TungstenRod_Damage1ReducedTo0);
    RUN_TEST(suite, test_TungstenRod_NoTriggerWithBlock);
    RUN_TEST(suite, test_TungstenRod_ModifiesHpLoss);
    RUN_TEST(suite, test_TungstenRod_HpLoss1ReducedTo0);
    
    // 组合测试
    RUN_TEST(suite, test_ToriiAndTungstenRod_CombineCorrectly);
    RUN_TEST(suite, test_VulnerableAndTungstenRod_CombineCorrectly);
    
    // 无攻击者测试
    RUN_TEST(suite, test_NoSource_ReturnsBaseDamage);
    
    // 实际伤害测试
    RUN_TEST(suite, test_ActualDamage_WithVulnerable);
    RUN_TEST(suite, test_ActualDamage_WithTorii);
    RUN_TEST(suite, test_ActualDamage_WithTungstenRod);
    
    suite.printReport();
}

}

namespace BlockPipelineTests {

GameState createTestState() {
    GameState state;
    state.enableLogging = false;
    state.monsters.push_back(std::make_shared<Monster>("测试怪物", 100));
    BasicRules::registerRules(state);
    return state;
}

// ==========================================
// 测试用的敏捷状态
// ==========================================
class TestDexterityPower : public AbstractPower {
public:
    TestDexterityPower(int amount) 
        : AbstractPower("敏捷", amount, PowerType::BUFF) {}
    
    float atBlockGive(float block) override {
        return block + getAmount();
    }
};

// ==========================================
// 测试用的格挡增益遗物
// ==========================================
class TestBlockBuffRelic : public AbstractRelic {
public:
    float bonusMultiplier = 0.25f;
    
    TestBlockBuffRelic() : AbstractRelic("测试格挡增益遗物") {}
    
    float atBlockGive(float block) override {
        return block * (1.0f + bonusMultiplier);
    }
};

void test_BasicBlock_NoModifiers() {
    GameState state = createTestState();
    
    int baseBlock = 5;
    int finalBlock = state.player->calculateFinalBlock(baseBlock);
    
    TEST_ASSERT_EQ(finalBlock, 5, 
        "Block should be unchanged with no modifiers");
}

void test_DexterityPower_IncreasesBlock() {
    GameState state = createTestState();
    
    auto dexterityPower = std::make_shared<TestDexterityPower>(2);
    state.player->addPower(dexterityPower);
    
    int baseBlock = 5;
    int finalBlock = state.player->calculateFinalBlock(baseBlock);
    
    TEST_ASSERT_EQ(finalBlock, 7, 
        "Dexterity should add to block");
}

void test_BlockRelic_ModifiesBlock() {
    GameState state = createTestState();
    
    auto blockRelic = std::make_shared<TestBlockBuffRelic>();
    state.player->addRelic(blockRelic, state);
    
    int baseBlock = 4;
    int finalBlock = state.player->calculateFinalBlock(baseBlock);
    
    TEST_ASSERT_EQ(finalBlock, 5, 
        "Block relic should modify block (4 * 1.25 = 5)");
}

void test_DexterityAndRelic_CombineCorrectly() {
    GameState state = createTestState();
    
    auto dexterityPower = std::make_shared<TestDexterityPower>(2);
    state.player->addPower(dexterityPower);
    
    auto blockRelic = std::make_shared<TestBlockBuffRelic>();
    state.player->addRelic(blockRelic, state);
    
    // 计算: 5 + 2 = 7 (阶段1) -> 7 * 1.25 = 8.75 -> 8
    int baseBlock = 5;
    int finalBlock = state.player->calculateFinalBlock(baseBlock);
    
    TEST_ASSERT_EQ(finalBlock, 8, 
        "Dexterity and relic should combine: (5+2)*1.25 = 8.75 -> 8");
}

void runAllTests() {
    TestSuite suite("防御计算管线测试");
    
    RUN_TEST(suite, test_BasicBlock_NoModifiers);
    RUN_TEST(suite, test_DexterityPower_IncreasesBlock);
    RUN_TEST(suite, test_BlockRelic_ModifiesBlock);
    RUN_TEST(suite, test_DexterityAndRelic_CombineCorrectly);
    
    suite.printReport();
}

}
