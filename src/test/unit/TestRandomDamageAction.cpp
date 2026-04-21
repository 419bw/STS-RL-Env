#include <catch_amalgamated.hpp>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/Actions.h"
#include "src/action/LambdaAction.h"
#include "src/rules/BasicRules.h"
#include "src/core/Types.h"
#include <map>

static GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("Monster A", 100));
    engine.combatState->monsters.push_back(std::make_shared<Monster>("Monster B", 100));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

static GameEngine createEmptyEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("Dummy", 1));
    engine.combatState->monsters.clear();
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

TEST_CASE("RandomDamageAction single target", "[randomdamage][unit]") {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("Lone Monster", 100));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);

    auto monster = engine.combatState->monsters[0];
    int initialHp = monster->current_hp;

    engine.actionManager.addAction(std::make_unique<RandomDamageAction>(
        engine.combatState->player, 10, DamageType::ATTACK));
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;

    CombatFlow flow;
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->current_hp == initialHp - 10);
}

TEST_CASE("RandomDamageAction multi-target random selection", "[randomdamage][unit]") {
    std::map<std::string, int> hitCount;
    const int iterations = 100;

    for (int i = 0; i < iterations; ++i) {
        GameEngine localEngine;
        localEngine.startNewRun(1337 + i);
        localEngine.startCombat(std::make_shared<Monster>("Monster A", 100));
        localEngine.combatState->monsters.push_back(std::make_shared<Monster>("Monster B", 100));
        localEngine.combatState->enableLogging = false;
        BasicRules::registerRules(localEngine);

        localEngine.actionManager.addAction(std::make_unique<RandomDamageAction>(
            localEngine.combatState->player, 10, DamageType::ATTACK));
        localEngine.combatState->currentPhase = StatePhase::PLAYING_CARD;

        CombatFlow flow;
        localEngine.actionManager.executeUntilBlocked(localEngine, flow);

        for (auto& m : localEngine.combatState->monsters) {
            if (m->current_hp < 100) {
                hitCount[m->name]++;
                break;
            }
        }
    }

    // 两个目标都至少命中过一次
    REQUIRE(hitCount["Monster A"] > 0);
    REQUIRE(hitCount["Monster B"] > 0);

    // 近似均匀分布：每个目标的命中率应在 30%-70% 之间
    // 期望是 50%，允许合理的随机波动
    double ratioA = static_cast<double>(hitCount["Monster A"]) / iterations;
    double ratioB = static_cast<double>(hitCount["Monster B"]) / iterations;
    REQUIRE(ratioA > 0.30);
    REQUIRE(ratioA < 0.70);
    REQUIRE(ratioB > 0.30);
    REQUIRE(ratioB < 0.70);
}

TEST_CASE("RandomDamageAction empty target list no crash", "[randomdamage][unit]") {
    GameEngine engine = createEmptyEngine();
    engine.actionManager.addAction(std::make_unique<RandomDamageAction>(
        engine.combatState->player, 10, DamageType::ATTACK));
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;

    CombatFlow flow;
    REQUIRE_NOTHROW(engine.actionManager.executeUntilBlocked(engine, flow));
}

TEST_CASE("RandomDamageAction all monsters dead no crash", "[randomdamage][unit]") {
    GameEngine engine = createEmptyEngine();
    engine.combatState->monsters.push_back(std::make_shared<Monster>("Dead Monster", 100));
    engine.combatState->monsters[0]->current_hp = 0;

    engine.actionManager.addAction(std::make_unique<RandomDamageAction>(
        engine.combatState->player, 10, DamageType::ATTACK));
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;

    CombatFlow flow;
    REQUIRE_NOTHROW(engine.actionManager.executeUntilBlocked(engine, flow));
}

TEST_CASE("RandomDamageAction dead source no crash", "[randomdamage][unit]") {
    GameEngine engine = createTestEngine();
    auto deadPlayer = std::make_shared<Monster>("Dead Player", 100);
    deadPlayer->current_hp = 0;

    engine.actionManager.addAction(std::make_unique<RandomDamageAction>(
        deadPlayer, 10, DamageType::ATTACK));
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;

    CombatFlow flow;
    REQUIRE_NOTHROW(engine.actionManager.executeUntilBlocked(engine, flow));
}

TEST_CASE("RandomDamageAction integrates with LambdaAction", "[randomdamage][unit]") {
    GameEngine engine = createTestEngine();
    auto sharedFlag = std::make_shared<bool>(false);

    engine.actionManager.addAction(LambdaAction::make(
        std::weak_ptr<Character>(engine.combatState->player),
        [sharedFlag](GameEngine&, Character*) {
            *sharedFlag = true;
        }
    ));

    engine.actionManager.addAction(std::make_unique<RandomDamageAction>(
        engine.combatState->player, 10, DamageType::ATTACK));
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;

    CombatFlow flow;
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(*sharedFlag == true);
}
