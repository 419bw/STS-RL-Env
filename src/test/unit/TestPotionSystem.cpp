#include <catch_amalgamated.hpp>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/potion/AbstractPotion.h"
#include "src/potion/Potions.h"
#include "src/power/Powers.h"
#include "src/character/Character.h"
#include "src/rules/BasicRules.h"

static GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(1337);
    engine.startCombat(std::make_shared<Monster>("TestMonster", 30));
    engine.combatState->enableLogging = false;
    engine.combatState->isPlayerTurn = true;
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;
    BasicRules::registerRules(engine);
    return engine;
}

TEST_CASE("StrengthPotion default id", "[potion][unit]") {
    StrengthPotion potion;
    REQUIRE(potion.id == "strength_potion");
}

TEST_CASE("StrengthPotion default target is SELF", "[potion][unit]") {
    StrengthPotion potion;
    REQUIRE(potion.targetType == PotionTarget::SELF);
}

TEST_CASE("StrengthPotion use adds action", "[potion][unit]") {
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();
    engine.runState->potions.push_back(potion);

    potion->use(engine, engine.combatState->player);

    REQUIRE(!engine.actionManager.isQueueEmpty());
}

TEST_CASE("UsePotion wrong phase returns false", "[potion][unit]") {
    GameEngine engine = createTestEngine();
    engine.combatState->currentPhase = StatePhase::WAITING_FOR_CARD_SELECTION;
    auto potion = std::make_shared<StrengthPotion>();
    engine.runState->potions.push_back(potion);

    CombatFlow flow;
    REQUIRE(PlayerActions::usePotion(engine, flow, potion, engine.combatState->player) == false);
}

TEST_CASE("UsePotion not player turn returns false", "[potion][unit]") {
    GameEngine engine = createTestEngine();
    engine.combatState->isPlayerTurn = false;
    auto potion = std::make_shared<StrengthPotion>();
    engine.runState->potions.push_back(potion);

    CombatFlow flow;
    REQUIRE(PlayerActions::usePotion(engine, flow, potion, engine.combatState->player) == false);
}

TEST_CASE("UsePotion potion not found returns false", "[potion][unit]") {
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();

    CombatFlow flow;
    REQUIRE(PlayerActions::usePotion(engine, flow, potion, engine.combatState->player) == false);
}

TEST_CASE("UsePotion self target routes to player", "[potion][unit]") {
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();
    potion->targetType = PotionTarget::SELF;
    engine.runState->potions.push_back(potion);

    CombatFlow flow;
    bool result = PlayerActions::usePotion(engine, flow, potion, engine.combatState->player);

    REQUIRE(result == true);
    REQUIRE(engine.combatState->player->getPower("力量") != nullptr);
    REQUIRE(engine.combatState->player->getPower("力量")->getAmount() == 3);
}

TEST_CASE("UsePotion enemy target blocks self-targeting", "[potion][unit]") {
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();
    potion->targetType = PotionTarget::ENEMY;
    engine.runState->potions.push_back(potion);

    CombatFlow flow;
    REQUIRE(PlayerActions::usePotion(engine, flow, potion, engine.combatState->player) == false);
}

TEST_CASE("UsePotion removes potion from state", "[potion][unit]") {
    GameEngine engine = createTestEngine();
    auto potion = std::make_shared<StrengthPotion>();
    engine.runState->potions.push_back(potion);

    CombatFlow flow;
    bool result = PlayerActions::usePotion(engine, flow, potion, engine.combatState->player);

    REQUIRE(result == true);

    bool found = false;
    for (const auto& p : engine.runState->potions) {
        if (p == potion) { found = true; break; }
    }
    REQUIRE(!found);
}
