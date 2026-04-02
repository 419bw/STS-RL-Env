#include "src/map/MapGenerator.h"
#include <iostream>
#include <sstream>

std::string captureMap(const MapGeneratorParams& params) {
    std::ostringstream oss;
    auto oldBuf = std::cout.rdbuf(oss.rdbuf());

    MapGenerator gen(params);
    MapData map = gen.generate();
    MapRenderer::render(map);

    std::cout.rdbuf(oldBuf);
    return oss.str();
}

int main() {
    std::cout << "========== Determinism Check ==========\n\n";
    MapGeneratorParams params;
    params.height = 15;
    params.width = 7;
    params.pathDensity = 6;
    params.ascensionLevel = 0;
    params.seed = 12345;

    std::cout << "Generating map with seed=12345 twice...\n\n";
    std::string mapA = captureMap(params);
    std::string mapB = captureMap(params);

    if (mapA == mapB) {
        std::cout << "[PASS] Both maps are identical (deterministic)\n\n";
    } else {
        std::cout << "[FAIL] Maps differ!\n\n";
    }

    std::cout << "========== Map 1 (seed=1001) ==========\n" << mapA;

    const uint32_t seeds[] = {2048, 3333, 5678, 9999};
    for (int i = 0; i < 4; ++i) {
        MapGeneratorParams p;
        p.height = 15;
        p.width = 7;
        p.pathDensity = 6;
        p.ascensionLevel = 0;
        p.seed = seeds[i];

        MapGenerator gen(p);
        MapData map = gen.generate();

        std::cout << "\n========== Map " << (i + 2) << " (seed=" << seeds[i] << ") ==========\n";
        MapRenderer::render(map);
    }
    return 0;
}
