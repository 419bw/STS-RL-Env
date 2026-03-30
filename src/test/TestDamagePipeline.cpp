#include "src/test/TestFramework.h"
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/power/AbstractPower.h"
#include "src/relic/Relics.h"
#include "src/relic/AbstractRelic.h"
#include "src/character/Character.h"
#include "src/rules/BasicRules.h"
#include <cmath>

using namespace TestFramework;

namespace DamagePipelineTests {

GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("测试怪物", 100));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

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

class TestAttackerBuffRelic : public AbstractRelic {
public:
    float bonusMultiplier = 0.25f;
    
    TestAttackerBuffRelic() : AbstractRelic("测试攻击者增益遗物") {}
    
    float atDamageGive(float damage, DamageType type) override {
        return damage * (1.0f + bonusMultiplier);
    }
};

void test_BasicDamage_NoModifiers() {
    GameEngine engine = createTestEngine();
    
    auto monster = engine.combatState->monsters[0];
    int baseDamage = 10;
    
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 10, 
        "Damage should be unchanged with no modifiers");
}

void test_VulnerablePower_IncreasesDamage() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Vulnerable should increase damage by 50%");
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

void test_Stage1_AttackerPower_ModifiesDamage() {
    GameEngine engine = createTestEngine();
    
    auto strengthPower = std::make_shared<TestStrengthPower>(3);
    engine.combatState->player->addPower(strengthPower);
    
    auto monster = engine.combatState->monsters[0];
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 13, 
        "Strength should add to damage in stage 1");
}

void test_Stage1_AttackerRelic_ModifiesDamage() {
    GameEngine engine = createTestEngine();
    
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);
    
    auto monster = engine.combatState->monsters[0];
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 12, 
        "Attacker relic should modify damage in stage 1 (10 * 1.25 = 12.5 -> 12)");
}

void test_Stage2_DefenderPower_ModifiesDamage() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 15, 
        "Vulnerable should multiply damage in stage 2");
}

void test_Stage4_DefenderPower_ModifiesDamage() {
    GameEngine engine = createTestEngine();
    
    auto monster = engine.combatState->monsters[0];
    auto intangiblePower = std::make_shared<TestIntangiblePower>(1);
    monster->addPower(intangiblePower);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 1, 
        "Intangible should cap damage at 1 in stage 4");
}

void test_AllStages_CombineCorrectly() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto strengthPower = std::make_shared<TestStrengthPower>(3);
    engine.combatState->player->addPower(strengthPower);
    
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, engine.combatState->player.get());
    
    TEST_ASSERT_EQ(finalDamage, 19, 
        "All stages should combine: (10+3)*1.5 = 19.5 -> 19");
}

void test_ToriiRelic_DamageBelow5ReducedTo1() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 3));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 1, 
        "Torii should reduce actual HP loss 3 to 1");
}

void test_ToriiRelic_DamageAbove5Unchanged() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 10, 
        "Torii should not affect damage > 5");
}

void test_ToriiRelic_DamageExactly5ReducedTo1() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 1, 
        "Torii should reduce HP loss 5 to 1");
}

void test_ToriiRelic_Damage1Unchanged() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 1, 
        "Torii should keep HP loss 1 as 1");
}

void test_ToriiRelic_NoTriggerWithBlock() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    
    engine.actionManager.addAction(std::make_unique<GainBlockAction>(engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Torii should not trigger when block absorbs all damage");
    TEST_ASSERT_EQ(engine.combatState->player->block, 5, 
        "Block should be reduced to 5");
}

void test_ToriiRelic_TriggerAfterBlock() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    
    engine.actionManager.addAction(std::make_unique<GainBlockAction>(engine.combatState->player, 3));
    engine.actionManager.executeUntilBlocked(engine, flow);
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 8));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 1, 
        "Torii should reduce actual HP loss 5 to 1 after block");
    TEST_ASSERT_EQ(engine.combatState->player->block, 0, 
        "Block should be depleted");
}

void test_TungstenRodReducesDamageBy1() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(tungstenRod, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 9, 
        "Tungsten Rod should reduce actual HP loss by 1");
}

void test_TungstenRod_Damage1ReducedTo0() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(tungstenRod, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Tungsten Rod should reduce HP loss 1 to 0");
}

void test_TungstenRod_NoTriggerWithBlock() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(tungstenRod, engine);
    
    engine.actionManager.addAction(std::make_unique<GainBlockAction>(engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Tungsten Rod should not trigger when block absorbs all damage");
    TEST_ASSERT_EQ(engine.combatState->player->block, 5, 
        "Block should be reduced to 5");
}

void test_TungstenRod_ModifiesHpLoss() {
    GameEngine engine = createTestEngine();
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(tungstenRod, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    int actualHpLost = engine.combatState->player->loseHp(5);
    
    TEST_ASSERT_EQ(actualHpLost, 4, 
        "Tungsten Rod should reduce HP loss by 1");
    TEST_ASSERT_EQ(engine.combatState->player->current_hp, initialHp - 4, 
        "HP should be reduced by 4");
}

void test_TungstenRod_HpLoss1ReducedTo0() {
    GameEngine engine = createTestEngine();
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(tungstenRod, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    int actualHpLost = engine.combatState->player->loseHp(1);
    
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Tungsten Rod should reduce HP loss 1 to 0");
    TEST_ASSERT_EQ(engine.combatState->player->current_hp, initialHp, 
        "HP should not be reduced");
}

void test_ToriiAndTungstenRod_CombineCorrectly() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    engine.combatState->player->addRelic(tungstenRod, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 0, 
        "Torii (5->1) then Tungsten Rod (1->0) should result in 0");
}

void test_VulnerableAndTungstenRod_CombineCorrectly() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(tungstenRod, engine);
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->monsters[0], engine.combatState->player, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int initialHp = engine.combatState->player->current_hp;
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualHpLost = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualHpLost, 14, 
        "Vulnerable (10->15) then Tungsten Rod (15->14) should result in 14");
}

void test_NoSource_ReturnsBaseDamage() {
    GameEngine engine = createTestEngine();
    
    auto monster = engine.combatState->monsters[0];
    int baseDamage = 10;
    int finalDamage = monster->calculateFinalDamage(baseDamage, nullptr);
    
    TEST_ASSERT_EQ(finalDamage, 10, 
        "Damage with no source should return base damage");
}

void test_ActualDamage_WithVulnerable() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto monster = engine.combatState->monsters[0];
    int initialHp = monster->current_hp;
    
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->player, monster, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualDamage = initialHp - monster->current_hp;
    TEST_ASSERT_EQ(actualDamage, 15, 
        "Actual damage should be 15 with vulnerable (10 * 1.5)");
}

void test_ActualDamage_WithTorii() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    auto monster = engine.combatState->monsters[0];
    engine.actionManager.addAction(std::make_unique<DamageAction>(monster, engine.combatState->player, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualDamage = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualDamage, 1, 
        "Actual damage should be 1 with Torii (5 -> 1)");
}

void test_ActualDamage_WithTungstenRod() {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(tungstenRod, engine);
    
    int initialHp = engine.combatState->player->current_hp;
    
    auto monster = engine.combatState->monsters[0];
    engine.actionManager.addAction(std::make_unique<DamageAction>(monster, engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    
    int actualDamage = initialHp - engine.combatState->player->current_hp;
    TEST_ASSERT_EQ(actualDamage, 9, 
        "Actual damage should be 9 with Tungsten Rod (10 - 1)");
}

void runAllTests() {
    TestSuite suite("伤害计算管线测试");
    
    RUN_TEST(suite, test_BasicDamage_NoModifiers);
    RUN_TEST(suite, test_VulnerablePower_IncreasesDamage);
    RUN_TEST(suite, test_VulnerablePower_ZeroStacksNoEffect);
    
    RUN_TEST(suite, test_Stage1_AttackerPower_ModifiesDamage);
    RUN_TEST(suite, test_Stage1_AttackerRelic_ModifiesDamage);
    RUN_TEST(suite, test_Stage2_DefenderPower_ModifiesDamage);
    RUN_TEST(suite, test_Stage4_DefenderPower_ModifiesDamage);
    RUN_TEST(suite, test_AllStages_CombineCorrectly);
    
    RUN_TEST(suite, test_ToriiRelic_DamageBelow5ReducedTo1);
    RUN_TEST(suite, test_ToriiRelic_DamageAbove5Unchanged);
    RUN_TEST(suite, test_ToriiRelic_DamageExactly5ReducedTo1);
    RUN_TEST(suite, test_ToriiRelic_Damage1Unchanged);
    RUN_TEST(suite, test_ToriiRelic_NoTriggerWithBlock);
    RUN_TEST(suite, test_ToriiRelic_TriggerAfterBlock);
    
    RUN_TEST(suite, test_TungstenRodReducesDamageBy1);
    RUN_TEST(suite, test_TungstenRod_Damage1ReducedTo0);
    RUN_TEST(suite, test_TungstenRod_NoTriggerWithBlock);
    RUN_TEST(suite, test_TungstenRod_ModifiesHpLoss);
    RUN_TEST(suite, test_TungstenRod_HpLoss1ReducedTo0);
    
    RUN_TEST(suite, test_ToriiAndTungstenRod_CombineCorrectly);
    RUN_TEST(suite, test_VulnerableAndTungstenRod_CombineCorrectly);
    
    RUN_TEST(suite, test_NoSource_ReturnsBaseDamage);
    
    RUN_TEST(suite, test_ActualDamage_WithVulnerable);
    RUN_TEST(suite, test_ActualDamage_WithTorii);
    RUN_TEST(suite, test_ActualDamage_WithTungstenRod);
    
    suite.printReport();
}

}

namespace BlockPipelineTests {

GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("测试怪物", 100));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

class TestDexterityPower : public AbstractPower {
public:
    TestDexterityPower(int amount) 
        : AbstractPower("敏捷", amount, PowerType::BUFF) {}
    
    float atBlockGive(float block) override {
        return block + getAmount();
    }
};

class TestBlockBuffRelic : public AbstractRelic {
public:
    float bonusMultiplier = 0.25f;
    
    TestBlockBuffRelic() : AbstractRelic("测试格挡增益遗物") {}
    
    float atBlockGive(float block) override {
        return block * (1.0f + bonusMultiplier);
    }
};

void test_BasicBlock_NoModifiers() {
    GameEngine engine = createTestEngine();
    
    int baseBlock = 5;
    int finalBlock = engine.combatState->player->calculateFinalBlock(baseBlock);
    
    TEST_ASSERT_EQ(finalBlock, 5, 
        "Block should be unchanged with no modifiers");
}

void test_DexterityPower_IncreasesBlock() {
    GameEngine engine = createTestEngine();
    
    auto dexterityPower = std::make_shared<TestDexterityPower>(2);
    engine.combatState->player->addPower(dexterityPower);
    
    int baseBlock = 5;
    int finalBlock = engine.combatState->player->calculateFinalBlock(baseBlock);
    
    TEST_ASSERT_EQ(finalBlock, 7, 
        "Dexterity should add to block");
}

void test_BlockRelic_ModifiesBlock() {
    GameEngine engine = createTestEngine();
    
    auto blockRelic = std::make_shared<TestBlockBuffRelic>();
    engine.combatState->player->addRelic(blockRelic, engine);
    
    int baseBlock = 4;
    int finalBlock = engine.combatState->player->calculateFinalBlock(baseBlock);
    
    TEST_ASSERT_EQ(finalBlock, 5, 
        "Block relic should modify block (4 * 1.25 = 5)");
}

void test_DexterityAndRelic_CombineCorrectly() {
    GameEngine engine = createTestEngine();
    
    auto dexterityPower = std::make_shared<TestDexterityPower>(2);
    engine.combatState->player->addPower(dexterityPower);
    
    auto blockRelic = std::make_shared<TestBlockBuffRelic>();
    engine.combatState->player->addRelic(blockRelic, engine);
    
    int baseBlock = 5;
    int finalBlock = engine.combatState->player->calculateFinalBlock(baseBlock);
    
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
