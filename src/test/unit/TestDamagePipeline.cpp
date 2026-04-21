#include <catch_amalgamated.hpp>
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

static GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("TestMonster", 100));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

class TestStrengthPower : public AbstractPower {
public:
    TestStrengthPower(int amount)
        : AbstractPower("Strength", amount, PowerType::BUFF) {}

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
        : AbstractPower("Weak", amount, PowerType::DEBUFF) {}

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
        : AbstractPower("Intangible", amount, PowerType::BUFF) {}

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

    TestAttackerBuffRelic() : AbstractRelic("TestAttackerRelic") {}

    float atDamageGive(float damage, DamageType type) override {
        return damage * (1.0f + bonusMultiplier);
    }
};

// ===== Basic Damage Tests =====

TEST_CASE("Basic damage with no modifiers", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    auto monster = engine.combatState->monsters[0];
    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 10);
}

TEST_CASE("No source returns base damage", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    auto monster = engine.combatState->monsters[0];
    REQUIRE(monster->calculateFinalDamage(10, nullptr) == 10);
}

// ===== Vulnerable Tests =====

TEST_CASE("Vulnerable power increases damage", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);

    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 15);
}

TEST_CASE("Vulnerable power zero stacks no effect", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(0);

    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 10);
}

TEST_CASE("Vulnerable power negative stacks no effect", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(-1);

    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 10);
}

TEST_CASE("Vulnerable multiplier stacking", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(3);

    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->getPower("易伤")->getAmount() == 3);
    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 15);
}

TEST_CASE("Vulnerable multiplier round trip", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);

    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 15);
}

// ===== Stage 1 Power/Relic Tests =====

TEST_CASE("Stage1 attacker power modifies damage", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    auto strengthPower = std::make_shared<TestStrengthPower>(3);
    engine.combatState->player->addPower(strengthPower);
    auto monster = engine.combatState->monsters[0];

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 13);
}

TEST_CASE("Stage1 attacker relic modifies damage", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    auto attackerRelic = std::make_shared<TestAttackerBuffRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);
    auto monster = engine.combatState->monsters[0];

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 12);
}

// ===== Weak Tests =====

TEST_CASE("Weak power reduces damage", "[damage][weak][unit]") {
    GameEngine engine = createTestEngine();
    auto weakPower = std::make_shared<TestWeakPower>(2);
    engine.combatState->player->addPower(weakPower);
    auto monster = engine.combatState->monsters[0];

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 7);
}

TEST_CASE("Weak power combines with vulnerable", "[damage][weak][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto weakPower = std::make_shared<TestWeakPower>(1);
    engine.combatState->player->addPower(weakPower);

    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 11);
}

// ===== Stage 4 Intangible Tests =====

TEST_CASE("Stage4 intangible caps damage at 1", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    auto monster = engine.combatState->monsters[0];
    auto intangiblePower = std::make_shared<TestIntangiblePower>(1);
    monster->addPower(intangiblePower);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 1);
}

TEST_CASE("All stages combine correctly", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto strengthPower = std::make_shared<TestStrengthPower>(3);
    engine.combatState->player->addPower(strengthPower);

    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 19);
}

// ===== Torii Relic Tests =====

TEST_CASE("Torii relic reduces damage <=5 to 1", "[damage][torii][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    int initialHp = engine.combatState->player->current_hp;

    SECTION("Damage 3 -> 1") {
        engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 3));
        engine.actionManager.executeUntilBlocked(engine, flow);
        REQUIRE(initialHp - engine.combatState->player->current_hp == 1);
    }

    SECTION("Damage 10 -> 10") {
        engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 10));
        engine.actionManager.executeUntilBlocked(engine, flow);
        REQUIRE(initialHp - engine.combatState->player->current_hp == 10);
    }

    SECTION("Damage 5 -> 1") {
        engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 5));
        engine.actionManager.executeUntilBlocked(engine, flow);
        REQUIRE(initialHp - engine.combatState->player->current_hp == 1);
    }

    SECTION("Damage 1 -> 1") {
        engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 1));
        engine.actionManager.executeUntilBlocked(engine, flow);
        REQUIRE(initialHp - engine.combatState->player->current_hp == 1);
    }
}

TEST_CASE("Torii relic does not trigger with block", "[damage][torii][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);

    engine.actionManager.addAction(std::make_unique<GainBlockAction>(engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    int initialHp = engine.combatState->player->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 0);
    REQUIRE(engine.combatState->player->block == 5);
}

TEST_CASE("Torii relic triggers after partial block", "[damage][torii][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);

    engine.actionManager.addAction(std::make_unique<GainBlockAction>(engine.combatState->player, 3));
    engine.actionManager.executeUntilBlocked(engine, flow);
    int initialHp = engine.combatState->player->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 8));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 1);
    REQUIRE(engine.combatState->player->block == 0);
}

TEST_CASE("Torii edge case: damage 0", "[damage][torii][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    int initialHp = engine.combatState->player->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 0));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 0);
}

// ===== TungstenRod Relic Tests =====

TEST_CASE("TungstenRod reduces damage by 1", "[damage][tungsten][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto rod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(rod, engine);
    int initialHp = engine.combatState->player->current_hp;

    SECTION("Damage 10 -> 9") {
        engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 10));
        engine.actionManager.executeUntilBlocked(engine, flow);
        REQUIRE(initialHp - engine.combatState->player->current_hp == 9);
    }

    SECTION("Damage 1 -> 0") {
        engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 1));
        engine.actionManager.executeUntilBlocked(engine, flow);
        REQUIRE(initialHp - engine.combatState->player->current_hp == 0);
    }
}

TEST_CASE("TungstenRod does not trigger with block", "[damage][tungsten][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto rod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(rod, engine);

    engine.actionManager.addAction(std::make_unique<GainBlockAction>(engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);
    int initialHp = engine.combatState->player->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 0);
    REQUIRE(engine.combatState->player->block == 5);
}

TEST_CASE("TungstenRod reduces loseHp by 1", "[damage][tungsten][unit]") {
    GameEngine engine = createTestEngine();
    auto rod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(rod, engine);
    int initialHp = engine.combatState->player->current_hp;

    REQUIRE(engine.combatState->player->loseHp(5) == 4);
    REQUIRE(engine.combatState->player->current_hp == initialHp - 4);

    REQUIRE(engine.combatState->player->loseHp(1) == 0);
    REQUIRE(engine.combatState->player->current_hp == initialHp - 4);
}

TEST_CASE("TungstenRod damage 1 reduced to 0", "[damage][tungsten][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto rod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(rod, engine);
    int initialHp = engine.combatState->player->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 0);
}

// ===== Torii Additional Tests =====

TEST_CASE("Torii relic damage 1 unchanged", "[damage][torii][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    int initialHp = engine.combatState->player->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 1));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 1);
}

TEST_CASE("Torii relic damage above 5 unchanged", "[damage][torii][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    int initialHp = engine.combatState->player->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 10);
}

// ===== Actual Damage Tests =====

TEST_CASE("Actual damage with vulnerable", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    int initialHp = monster->current_hp;

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->player, monster, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - monster->current_hp == 15);
}

TEST_CASE("Actual damage with Torii", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto toriiRelic = std::make_shared<ToriiRelic>();
    engine.combatState->player->addRelic(toriiRelic, engine);
    int initialHp = engine.combatState->player->current_hp;

    auto monster = engine.combatState->monsters[0];
    engine.actionManager.addAction(std::make_unique<DamageAction>(monster, engine.combatState->player, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 1);
}

TEST_CASE("Actual damage with TungstenRod", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto tungstenRod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(tungstenRod, engine);
    int initialHp = engine.combatState->player->current_hp;

    auto monster = engine.combatState->monsters[0];
    engine.actionManager.addAction(std::make_unique<DamageAction>(monster, engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 9);
}

// ===== Combination Tests =====

TEST_CASE("Torii and TungstenRod combine", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto torii = std::make_shared<ToriiRelic>();
    auto rod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(torii, engine);
    engine.combatState->player->addRelic(rod, engine);
    int initialHp = engine.combatState->player->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 5));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 0);
}

TEST_CASE("Vulnerable and TungstenRod combine", "[damage][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto rod = std::make_shared<TungstenRodRelic>();
    engine.combatState->player->addRelic(rod, engine);

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->monsters[0], engine.combatState->player, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);
    int initialHp = engine.combatState->player->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 14);
}

// ===== Block Pipeline Tests =====

TEST_CASE("Block pipeline: basic block no modifiers", "[block][unit]") {
    GameEngine engine = createTestEngine();
    REQUIRE(engine.combatState->player->calculateFinalBlock(5) == 5);
}

class TestDexterityPower : public AbstractPower {
public:
    TestDexterityPower(int amount)
        : AbstractPower("Dexterity", amount, PowerType::BUFF) {}

    float atBlockGive(float block) override {
        return block + getAmount();
    }
};

class TestBlockBuffRelic : public AbstractRelic {
public:
    float bonusMultiplier = 0.25f;
    TestBlockBuffRelic() : AbstractRelic("TestBlockRelic") {}

    float atBlockGive(float block) override {
        return block * (1.0f + bonusMultiplier);
    }
};

TEST_CASE("Dexterity increases block", "[block][unit]") {
    GameEngine engine = createTestEngine();
    auto dex = std::make_shared<TestDexterityPower>(2);
    engine.combatState->player->addPower(dex);
    REQUIRE(engine.combatState->player->calculateFinalBlock(5) == 7);
}

TEST_CASE("Block relic modifies block", "[block][unit]") {
    GameEngine engine = createTestEngine();
    auto relic = std::make_shared<TestBlockBuffRelic>();
    engine.combatState->player->addRelic(relic, engine);
    REQUIRE(engine.combatState->player->calculateFinalBlock(4) == 5);
}

TEST_CASE("Dexterity and relic combine", "[block][unit]") {
    GameEngine engine = createTestEngine();
    auto dex = std::make_shared<TestDexterityPower>(2);
    engine.combatState->player->addPower(dex);
    auto relic = std::make_shared<TestBlockBuffRelic>();
    engine.combatState->player->addRelic(relic, engine);
    REQUIRE(engine.combatState->player->calculateFinalBlock(5) == 8);
}
