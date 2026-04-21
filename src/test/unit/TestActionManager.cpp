#include <catch_amalgamated.hpp>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/Actions.h"
#include "src/action/LambdaAction.h"
#include "src/character/Character.h"
#include "src/rules/BasicRules.h"

static GameEngine createTestEngine() {
    GameEngine engine;
    engine.startNewRun(42);
    engine.startCombat(std::make_shared<Monster>("TestMonster", 100));
    engine.combatState->enableLogging = false;
    engine.combatState->currentPhase = StatePhase::PLAYING_CARD;
    BasicRules::registerRules(engine);
    return engine;
}

TEST_CASE("ActionManager executes actions in FIFO order", "[actionmanager][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    std::vector<int> order;

    auto player = engine.combatState->player;
    engine.actionManager.addAction(
        LambdaAction::make(std::weak_ptr<Character>(player),
            [&order](GameEngine&, Character*) { order.push_back(1); }));
    engine.actionManager.addAction(
        LambdaAction::make(std::weak_ptr<Character>(player),
            [&order](GameEngine&, Character*) { order.push_back(2); }));
    engine.actionManager.addAction(
        LambdaAction::make(std::weak_ptr<Character>(player),
            [&order](GameEngine&, Character*) { order.push_back(3); }));

    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(order == std::vector<int>{1, 2, 3});
}

TEST_CASE("ActionManager respects blocking action", "[actionmanager][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    std::vector<int> order;

    auto player = engine.combatState->player;
    engine.actionManager.addAction(LambdaAction::make(
        std::weak_ptr<Character>(player),
        [&order](GameEngine& eng, Character*) {
            order.push_back(1);
            eng.combatState->currentPhase = StatePhase::WAITING_FOR_CARD_SELECTION;
        }));
    engine.actionManager.addAction(LambdaAction::make(
        std::weak_ptr<Character>(player),
        [&order](GameEngine&, Character*) { order.push_back(2); }));

    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(order == std::vector<int>{1});
}

TEST_CASE("ActionManager addActionToFront has priority", "[actionmanager][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;
    std::vector<int> order;

    auto player = engine.combatState->player;
    engine.actionManager.addAction(LambdaAction::make(
        std::weak_ptr<Character>(player),
        [&order](GameEngine&, Character*) { order.push_back(2); }));

    engine.actionManager.addActionToFront(std::make_unique<DrawCardsAction>(0));

    engine.actionManager.addAction(LambdaAction::make(
        std::weak_ptr<Character>(player),
        [&order](GameEngine&, Character*) { order.push_back(3); }));

    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(order == std::vector<int>{2, 3});
}

TEST_CASE("ActionManager queue empties after execution", "[actionmanager][unit]") {
    GameEngine engine = createTestEngine();
    CombatFlow flow;

    REQUIRE(engine.actionManager.isQueueEmpty());

    auto player = engine.combatState->player;
    engine.actionManager.addAction(LambdaAction::make(
        std::weak_ptr<Character>(player),
        [](GameEngine&, Character*) {}));

    REQUIRE(!engine.actionManager.isQueueEmpty());

    engine.actionManager.executeUntilBlocked(engine, flow);

    REQUIRE(engine.actionManager.isQueueEmpty());
}
