#include <catch_amalgamated.hpp>
#include "src/event/EventBus.h"
#include "src/engine/GameEngine.h"
#include "src/core/Types.h"

TEST_CASE("EventBus publishes and receives events", "[eventbus][unit]") {
    EventBus bus;
    GameEngine engine;
    int receivedCount = 0;

    bus.subscribe(EventType::PHASE_ROUND_START,
        [&receivedCount](GameEngine&, void*) {
            receivedCount++;
            return true;
        });

    bus.publish(EventType::PHASE_ROUND_START, engine);
    bus.publish(EventType::PHASE_ROUND_START, engine);

    REQUIRE(receivedCount == 2);
}

TEST_CASE("EventBus auto-removes listener returning false", "[eventbus][unit]") {
    EventBus bus;
    GameEngine engine;
    int callCount = 0;

    bus.subscribe(EventType::PHASE_ROUND_START,
        [&callCount](GameEngine&, void*) {
            callCount++;
            return (callCount < 2);
        });

    bus.publish(EventType::PHASE_ROUND_START, engine);
    bus.publish(EventType::PHASE_ROUND_START, engine);
    bus.publish(EventType::PHASE_ROUND_START, engine);

    REQUIRE(callCount == 2);
}

TEST_CASE("EventBus handles multiple event types", "[eventbus][unit]") {
    EventBus bus;
    GameEngine engine;
    int phaseStartCount = 0;
    int phaseEndCount = 0;

    bus.subscribe(EventType::PHASE_ROUND_START,
        [&phaseStartCount](GameEngine&, void*) {
            phaseStartCount++;
            return true;
        });

    bus.subscribe(EventType::PHASE_ROUND_END,
        [&phaseEndCount](GameEngine&, void*) {
            phaseEndCount++;
            return true;
        });

    bus.publish(EventType::PHASE_ROUND_START, engine);
    bus.publish(EventType::PHASE_ROUND_END, engine);
    bus.publish(EventType::PHASE_ROUND_START, engine);

    REQUIRE(phaseStartCount == 2);
    REQUIRE(phaseEndCount == 1);
}

TEST_CASE("EventBus handles empty subscriber list", "[eventbus][unit]") {
    EventBus bus;
    GameEngine engine;

    REQUIRE_NOTHROW(bus.publish(EventType::PHASE_ROUND_START, engine));
}
