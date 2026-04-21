#include <catch_amalgamated.hpp>
#include "src/engine/GameEngine.h"
#include "src/character/Character.h"
#include "src/character/monster/JawWorm.h"
#include "src/intent/Intent.h"
#include "src/intent/FixedBrain.h"
#include "src/intent/RandomBrain.h"
#include "src/intent/brains/JawWormBrain.h"
#include "src/action/Actions.h"
#include "src/action/PlayerActions.h"
#include "src/power/Powers.h"
#include "src/flow/CombatFlow.h"
#include "src/rules/BasicRules.h"

static GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("Test Monster", 100));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);
    return engine;
}

static GameEngine createTestEngineWithPlayerHp(int hp) {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("Test Monster", 100));
    engine.combatState->enableLogging = false;
    engine.combatState->player->current_hp = hp;
    engine.combatState->player->max_hp = hp;
    BasicRules::registerRules(engine);
    return engine;
}

// ===== Basic Monster Tests =====

TEST_CASE("Monster basic construction", "[monster][unit]") {
    Monster monster("Basic Monster", 50);
    REQUIRE(monster.name == "Basic Monster");
    REQUIRE(monster.max_hp == 50);
    REQUIRE(monster.current_hp == 50);
}

TEST_CASE("Monster takes damage", "[monster][unit]") {
    Monster monster("Test Monster", 100);
    monster.takeDamage(30);
    REQUIRE(monster.current_hp == 70);
    REQUIRE(!monster.isDead());
}

TEST_CASE("Monster lethal damage kills", "[monster][unit]") {
    Monster monster("Test Monster", 100);
    monster.takeDamage(100);
    REQUIRE(monster.current_hp == 0);
    REQUIRE(monster.isDead());
}

TEST_CASE("Monster excess damage does not go below 0", "[monster][unit]") {
    Monster monster("Test Monster", 100);
    monster.takeDamage(150);
    REQUIRE(monster.current_hp == 0);
    REQUIRE(monster.isDead());
}

TEST_CASE("Monster gains block", "[monster][unit]") {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(10);
    REQUIRE(monster.block == 10);
}

TEST_CASE("Monster block stacks", "[monster][unit]") {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(10);
    monster.addBlockFinal(15);
    REQUIRE(monster.block == 25);
}

TEST_CASE("Monster block prevents damage", "[monster][unit]") {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(10);
    monster.takeDamage(5);
    REQUIRE(monster.current_hp == 100);
    REQUIRE(monster.block == 5);
}

TEST_CASE("Monster block absorbs full damage", "[monster][unit]") {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(10);
    monster.takeDamage(10);
    REQUIRE(monster.current_hp == 100);
    REQUIRE(monster.block == 0);
}

TEST_CASE("Monster block overwhelmed", "[monster][unit]") {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(5);
    monster.takeDamage(12);
    REQUIRE(monster.current_hp == 93);
    REQUIRE(monster.block == 0);
}

TEST_CASE("Monster death", "[monster][unit]") {
    Monster monster("Test Monster", 100);
    monster.takeDamage(100);
    REQUIRE(monster.isDead());
    REQUIRE(monster.current_hp == 0);
}

TEST_CASE("Monster roll intent with brain", "[monster][unit]") {
    GameEngine engine = createTestEngine();
    auto monster = engine.combatState->monsters[0];

    std::vector<Intent> sequence = {
        Intent{IntentType::ATTACK, 10, 1, 0, engine.combatState->player}
    };
    auto brain = std::make_shared<FixedBrain>(sequence);
    monster->setBrain(brain);
    monster->rollIntent(engine);

    const Intent& intent = monster->getRealIntent();
    REQUIRE(static_cast<int>(intent.type) == static_cast<int>(IntentType::ATTACK));
    REQUIRE(intent.base_damage == 10);
}

TEST_CASE("Monster roll intent without brain", "[monster][unit]") {
    GameEngine engine = createTestEngine();
    auto monster = engine.combatState->monsters[0];
    monster->rollIntent(engine);

    const Intent& intent = monster->getRealIntent();
    REQUIRE(static_cast<int>(intent.type) == static_cast<int>(IntentType::ATTACK));
}

TEST_CASE("Monster take turn: attack", "[monster][unit]") {
    GameEngine engine = createTestEngineWithPlayerHp(100);
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto player = engine.combatState->player.get();

    std::vector<Intent> sequence = {
        Intent{IntentType::ATTACK, 10, 1, 0, engine.combatState->player}
    };
    monster->setBrain(std::make_shared<FixedBrain>(sequence));
    monster->rollIntent(engine);

    int hpBefore = player->current_hp;
    monster->takeTurn(engine);
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(player->current_hp == hpBefore - 10);
}

TEST_CASE("Monster take turn: multi-hit attack", "[monster][unit]") {
    GameEngine engine = createTestEngineWithPlayerHp(100);
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto player = engine.combatState->player.get();

    std::vector<Intent> sequence = {
        Intent{IntentType::ATTACK, 10, 3, 0, engine.combatState->player}
    };
    monster->setBrain(std::make_shared<FixedBrain>(sequence));
    monster->rollIntent(engine);

    int hpBefore = player->current_hp;
    monster->takeTurn(engine);
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(player->current_hp == hpBefore - 30);
}

TEST_CASE("Monster take turn: defend", "[monster][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];

    std::vector<Intent> sequence = {
        Intent{IntentType::DEFEND, 0, 1, 15, {}}
    };
    monster->setBrain(std::make_shared<FixedBrain>(sequence));
    monster->rollIntent(engine);
    monster->takeTurn(engine);
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(monster->block == 15);
}

TEST_CASE("Monster take turn: attack defend", "[monster][unit]") {
    GameEngine engine = createTestEngineWithPlayerHp(100);
    CombatFlow flow;
    auto monster = engine.combatState->monsters[0];
    auto player = engine.combatState->player.get();

    std::vector<Intent> sequence = {
        Intent{IntentType::ATTACK_DEFEND, 10, 1, 8, engine.combatState->player}
    };
    monster->setBrain(std::make_shared<FixedBrain>(sequence));
    monster->rollIntent(engine);

    int hpBefore = player->current_hp;
    monster->takeTurn(engine);
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(player->current_hp == hpBefore - 10);
    REQUIRE(monster->block == 8);
}

// ===== JawWorm Tests =====

TEST_CASE("JawWorm construction at ascension 0", "[monster][jawworm][unit]") {
    JawWorm worm(0);
    REQUIRE(worm.name == "Jaw Worm");
    REQUIRE(worm.max_hp == 42);
    REQUIRE(worm.current_hp == 42);
}

TEST_CASE("JawWorm ascension scaling: asc 0", "[monster][jawworm][unit]") {
    JawWorm worm(0);
    REQUIRE(worm.max_hp == 42);
}

TEST_CASE("JawWorm ascension scaling: asc 7", "[monster][jawworm][unit]") {
    JawWorm worm(7);
    REQUIRE(worm.max_hp == 44);
}

TEST_CASE("JawWorm ascension scaling: asc 17", "[monster][jawworm][unit]") {
    JawWorm worm(17);
    REQUIRE(worm.max_hp == 44);
}

TEST_CASE("JawWorm first move always Chomp", "[monster][jawworm][unit]") {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<JawWorm>(0));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);

    auto wormPtr = std::dynamic_pointer_cast<JawWorm>(engine.combatState->monsters[0]);
    wormPtr->rollIntent(engine);
    Intent intent = wormPtr->getRealIntent();

    REQUIRE(static_cast<int>(intent.type) == static_cast<int>(IntentType::ATTACK));
    REQUIRE(intent.move_id == JawWormBrain::CHOMP);
    REQUIRE(intent.base_damage == 11);
}

TEST_CASE("JawWorm Thrash: attack and block", "[monster][jawworm][unit]") {
    GameEngine engine = createTestEngineWithPlayerHp(100);
    CombatFlow flow;

    auto wormPtr = std::make_shared<JawWorm>(0);
    engine.combatState->monsters[0] = wormPtr;

    auto player = engine.combatState->player.get();
    std::vector<Intent> thrashIntent = {
        Intent{IntentType::ATTACK_DEFEND, 7, 1, 5, engine.combatState->player}
    };
    auto thrashBrain = std::make_shared<FixedBrain>(thrashIntent);
    wormPtr->setBrain(thrashBrain);
    wormPtr->rollIntent(engine);

    int hpBefore = player->current_hp;
    wormPtr->takeTurn(engine);
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(player->current_hp == hpBefore - 7);
    REQUIRE(wormPtr->block == 5);
}

TEST_CASE("JawWorm Thrash: attack only", "[monster][jawworm][unit]") {
    GameEngine engine = createTestEngineWithPlayerHp(100);
    CombatFlow flow;

    auto wormPtr = std::make_shared<JawWorm>(0);
    engine.combatState->monsters[0] = wormPtr;

    auto player = engine.combatState->player.get();
    std::vector<Intent> thrashIntent = {
        Intent{IntentType::ATTACK_DEFEND, 7, 1, 5, engine.combatState->player}
    };
    auto thrashBrain = std::make_shared<FixedBrain>(thrashIntent);
    wormPtr->setBrain(thrashBrain);
    wormPtr->rollIntent(engine);

    REQUIRE(wormPtr->getRealIntent().base_damage == 7);
}

TEST_CASE("JawWorm Thrash: block only", "[monster][jawworm][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    auto wormPtr = std::make_shared<JawWorm>(0);
    engine.combatState->monsters[0] = wormPtr;
    BasicRules::registerRules(engine);

    std::vector<Intent> thrashIntent = {
        Intent{IntentType::ATTACK_DEFEND, 7, 1, 5, engine.combatState->player}
    };
    auto thrashBrain = std::make_shared<FixedBrain>(thrashIntent);
    wormPtr->setBrain(thrashBrain);
    wormPtr->rollIntent(engine);

    REQUIRE(wormPtr->getRealIntent().effect_value == 5);
}

TEST_CASE("JawWorm Thrash: different intent", "[monster][jawworm][unit]") {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<JawWorm>(0));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);

    auto wormPtr = std::dynamic_pointer_cast<JawWorm>(engine.combatState->monsters[0]);

    std::vector<Intent> attackIntent = {
        Intent{IntentType::ATTACK, 10, 1, 0, engine.combatState->player}
    };
    wormPtr->setBrain(std::make_shared<FixedBrain>(attackIntent));
    wormPtr->rollIntent(engine);
    REQUIRE(static_cast<int>(wormPtr->getRealIntent().type) == static_cast<int>(IntentType::ATTACK));

    std::vector<Intent> defendIntent = {
        Intent{IntentType::DEFEND, 0, 1, 8, {}}
    };
    wormPtr->setBrain(std::make_shared<FixedBrain>(defendIntent));
    wormPtr->rollIntent(engine);
    REQUIRE(static_cast<int>(wormPtr->getRealIntent().type) == static_cast<int>(IntentType::DEFEND));
}

TEST_CASE("JawWorm Bellow: buff and block", "[monster][jawworm][unit]") {
    GameEngine engine = createTestEngineWithPlayerHp(100);
    CombatFlow flow;

    auto wormPtr = std::make_shared<JawWorm>(0);
    engine.combatState->monsters[0] = wormPtr;

    Intent bellowIntent = Intent(IntentType::BUFF, 0, 1, 6, {}).withMove(JawWormBrain::BELLOW, "Bellow");
    std::vector<Intent> bellowSequence = { bellowIntent };
    auto bellowBrain = std::make_shared<FixedBrain>(bellowSequence);
    wormPtr->setBrain(bellowBrain);
    wormPtr->rollIntent(engine);

    wormPtr->takeTurn(engine);
    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(wormPtr->getRealIntent().effect_value == 6);
    REQUIRE(wormPtr->block == 6);
    REQUIRE(wormPtr->getPower("力量") != nullptr);
    REQUIRE(wormPtr->getPower("力量")->getAmount() == 3);
}

// ===== IntentBrain Tests =====

TEST_CASE("IntentBrain move history: lastMove", "[monster][brain][unit]") {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<JawWorm>(0));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);

    JawWorm worm(0);
    std::vector<Intent> sequence = {
        Intent(IntentType::ATTACK, 10, 1, 0, engine.combatState->player).withMove(1, "Attack1"),
        Intent(IntentType::DEFEND, 0, 1, 5, {}).withMove(2, "Defend"),
        Intent(IntentType::ATTACK, 10, 1, 0, engine.combatState->player).withMove(3, "Attack2")
    };
    auto brain = std::make_shared<FixedBrain>(sequence);

    brain->decide(*engine.combatState, &worm);
    REQUIRE(brain->lastMove(1) == true);
    REQUIRE(brain->lastMove(2) == false);

    brain->decide(*engine.combatState, &worm);
    REQUIRE(brain->lastMove(2) == true);

    brain->decide(*engine.combatState, &worm);
    REQUIRE(brain->lastMove(3) == true);
}

TEST_CASE("IntentBrain move history: lastTwoMoves", "[monster][brain][unit]") {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<JawWorm>(0));
    engine.combatState->enableLogging = false;
    BasicRules::registerRules(engine);

    JawWorm worm(0);
    std::vector<Intent> sequence = {
        Intent(IntentType::ATTACK, 10, 1, 0, engine.combatState->player).withMove(1, "Attack1"),
        Intent(IntentType::DEFEND, 0, 1, 5, {}).withMove(2, "Defend"),
        Intent(IntentType::ATTACK, 10, 1, 0, engine.combatState->player).withMove(3, "Attack2")
    };
    auto brain = std::make_shared<FixedBrain>(sequence);

    brain->decide(*engine.combatState, &worm);
    brain->decide(*engine.combatState, &worm);
    REQUIRE(brain->lastTwoMoves(2) == false);

    brain->decide(*engine.combatState, &worm);
    REQUIRE(brain->lastTwoMoves(3) == false);
    REQUIRE(brain->lastTwoMoves(2) == false);
    REQUIRE(brain->lastTwoMoves(1) == false);
}
