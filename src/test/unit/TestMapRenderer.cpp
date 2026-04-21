#include <catch_amalgamated.hpp>
#include <sstream>
#include <iostream>
#include "src/map/MapGenerator.h"

static std::string captureMap(const MapGeneratorParams& params) {
    std::ostringstream oss;
    auto oldBuf = std::cout.rdbuf(oss.rdbuf());

    MapGenerator gen(params);
    MapData map = gen.generate();
    MapRenderer::render(map);

    std::cout.rdbuf(oldBuf);
    return oss.str();
}

TEST_CASE("Map generation is deterministic", "[map][determinism][unit]") {
    MapGeneratorParams params;
    params.height = 15;
    params.width = 7;
    params.pathDensity = 6;
    params.ascensionLevel = 0;
    params.seed = 12345;

    std::string mapA = captureMap(params);
    std::string mapB = captureMap(params);

    REQUIRE(mapA == mapB);
}

TEST_CASE("Different seeds produce different maps", "[map][unit]") {
    MapGeneratorParams p1, p2;
    p1.height = 15; p1.width = 7; p1.pathDensity = 6; p1.ascensionLevel = 0; p1.seed = 1001;
    p2.height = 15; p2.width = 7; p2.pathDensity = 6; p2.ascensionLevel = 0; p2.seed = 2001;

    std::string mapA = captureMap(p1);
    std::string mapB = captureMap(p2);

    REQUIRE(mapA != mapB);
}

TEST_CASE("Multiple seed map renders are non-empty", "[map][smoke][unit]") {
    const int seedCount = 5;
    for (int i = 0; i < seedCount; ++i) {
        MapGeneratorParams params;
        params.height = 15;
        params.width = 7;
        params.pathDensity = 6;
        params.ascensionLevel = 0;
        params.seed = 10000 + i;

        std::string rendered = captureMap(params);

        REQUIRE(!rendered.empty());
        REQUIRE(rendered.size() > 50);
    }
}
