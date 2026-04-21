#include <catch_amalgamated.hpp>
#include "src/action/LambdaAction.h"
#include "src/character/Character.h"
#include "src/engine/GameEngine.h"

TEST_CASE("LambdaAction executes when source is alive", "[lambdaaction][unit]") {
    int value = 0;
    auto player = std::make_shared<Player>("Test", 100);
    auto weak = std::weak_ptr<Character>(player);

    auto action = LambdaAction::make(weak,
        [&value](GameEngine&, Character*) {
            value = 42;
        });

    GameEngine engine;
    bool done = action->update(engine);

    REQUIRE(done);
    REQUIRE(value == 42);
}

TEST_CASE("LambdaAction skips when source is dead (empty weak_ptr)", "[lambdaaction][unit]") {
    int value = 0;
    auto weak = std::weak_ptr<Character>();

    auto action = LambdaAction::make(weak,
        [&value](GameEngine&, Character*) {
            value = 42;
        });

    GameEngine engine;
    bool done = action->update(engine);

    REQUIRE(done);
    REQUIRE(value == 0);
}

TEST_CASE("LambdaAction skips when source is dead (isDead)", "[lambdaaction][unit]") {
    int value = 0;
    auto deadPlayer = std::make_shared<Player>("Dead", 0);
    deadPlayer->current_hp = 0;
    auto weak = std::weak_ptr<Character>(deadPlayer);

    auto action = LambdaAction::make(weak,
        [&value](GameEngine&, Character*) {
            value = 42;
        });

    GameEngine engine;
    bool done = action->update(engine);

    REQUIRE(done);
    REQUIRE(value == 0);
}

TEST_CASE("LambdaAction captures by value not reference", "[lambdaaction][unit]") {
    int capturedValue = 100;
    int result = 0;

    auto player = std::make_shared<Player>("Test", 100);
    auto weak = std::weak_ptr<Character>(player);
    auto action = LambdaAction::make(weak,
        [capturedValue, &result](GameEngine&, Character*) {
            result = capturedValue;
        });

    capturedValue = 999;

    GameEngine engine;
    action->update(engine);

    REQUIRE(result == 100);
}
