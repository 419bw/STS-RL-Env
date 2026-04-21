#include <catch_amalgamated.hpp>
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

static GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("TestMonster", 100));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

TEST_CASE("VulnerablePower base multiplier", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);

    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 15);
}

TEST_CASE("VulnerablePower zero stacks no effect", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(0);

    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 10);
}

TEST_CASE("VulnerablePower negative stacks no effect", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(-1);

    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 10);
}

class TestAttackerRelic : public AbstractRelic {
public:
    float bonusMultiplier = 0.25f;
    TestAttackerRelic() : AbstractRelic("TestAttackerRelic") {}

    void onQuery(VulnerableMultiplierQuery& query) override {
        if (query.source && query.source == getOwner()) {
            query.multiplier += bonusMultiplier;
        }
    }
};

class TestDefenderNerfRelic : public AbstractRelic {
public:
    float penaltyMultiplier = -0.25f;
    TestDefenderNerfRelic() : AbstractRelic("TestDefenderRelic") {}

    void onQuery(VulnerableMultiplierQuery& query) override {
        if (query.target && query.target == getOwner()) {
            query.multiplier += penaltyMultiplier;
        }
    }
};

TEST_CASE("Relic modifies vulnerable multiplier attacker side", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto attackerRelic = std::make_shared<TestAttackerRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);

    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 17);
}

TEST_CASE("Relic modifies vulnerable multiplier defender side", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto defenderRelic = std::make_shared<TestDefenderNerfRelic>();
    monster->addRelic(defenderRelic, engine);

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 12);
}

TEST_CASE("Multiple relics both sides", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto attackerRelic = std::make_shared<TestAttackerRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);

    auto monster = engine.combatState->monsters[0];
    auto defenderRelic = std::make_shared<TestDefenderNerfRelic>();
    monster->addRelic(defenderRelic, engine);

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 15);
}

TEST_CASE("Multiple attacker relics stack", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto relic1 = std::make_shared<TestAttackerRelic>();
    relic1->name = "AttackerRelic1";
    relic1->bonusMultiplier = 0.25f;
    engine.combatState->player->addRelic(relic1, engine);

    auto relic2 = std::make_shared<TestAttackerRelic>();
    relic2->name = "AttackerRelic2";
    relic2->bonusMultiplier = 0.25f;
    engine.combatState->player->addRelic(relic2, engine);

    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 20);
}

TEST_CASE("No vulnerable no multiplier applied", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto attackerRelic = std::make_shared<TestAttackerRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);

    auto monster = engine.combatState->monsters[0];
    int initialHp = monster->current_hp;

    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->player, monster, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - monster->current_hp == 10);
}

TEST_CASE("Vulnerable affects actual damage", "[vulnerable][unit]") {
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

TEST_CASE("Vulnerable stacking increases duration", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];

    auto v1 = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, v1));
    engine.actionManager.executeUntilBlocked(engine, flow);
    REQUIRE(monster->getPower("易伤")->getAmount() == 2);

    auto v2 = std::make_shared<VulnerablePower>(3);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, v2));
    engine.actionManager.executeUntilBlocked(engine, flow);
    REQUIRE(monster->getPower("易伤")->getAmount() == 5);
}

TEST_CASE("Vulnerable just applied: player apply no protection", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(!monster->getPower("易伤")->isJustApplied());
}

TEST_CASE("Vulnerable just applied: monster apply during enemy turn protected", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    engine.combatState->isPlayerTurn = false;

    auto player = engine.combatState->player;
    auto monster = engine.combatState->monsters[0];

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(monster, player, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(player->getPower("易伤")->isJustApplied());
}

TEST_CASE("Relic only affects own side", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto attackerRelic = std::make_shared<TestAttackerRelic>();
    engine.combatState->player->addRelic(attackerRelic, engine);

    auto vulnerablePower = std::make_shared<VulnerablePower>(2);
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->monsters[0], engine.combatState->player, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    int initialHp = engine.combatState->player->current_hp;
    engine.actionManager.addAction(std::make_unique<DamageAction>(engine.combatState->monsters[0], engine.combatState->player, 10));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(initialHp - engine.combatState->player->current_hp == 15);
}

TEST_CASE("Damage calculation with block and vulnerable", "[vulnerable][unit]") {
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

    REQUIRE(monster->block == 0);
    REQUIRE(initialHp - monster->current_hp == 10);
}

TEST_CASE("Query system zero overhead", "[vulnerable][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto vulnerablePower = std::make_shared<VulnerablePower>(2);

    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(engine.combatState->player, monster, vulnerablePower));
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->calculateFinalDamage(10, engine.combatState->player.get()) == 15);
}
