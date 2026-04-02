#pragma once

#include "src/map/MapRenderer.h"
#include <random>

struct MapGeneratorParams {
    int height = 15;
    int width = 7;
    int pathDensity = 6;
    int minAncestorGap = 3;
    int maxAncestorGap = 5;
    int ascensionLevel = 0;
    uint32_t seed = 0;
};

class MapGenerator {
public:
    explicit MapGenerator(const MapGeneratorParams& params = MapGeneratorParams{});

    MapData generate() const;

private:
    void buildTopology(MapData& map) const;
    void assignRoomTypes(MapData& map) const;
    int countValidNodes(const MapData& map) const;
    void fillRoomList(std::vector<NodeType>& roomList, int totalNodes) const;
    NodeType getNodeType(int y) const;
    void applyFixedFloors(MapData& map) const;
    std::vector<MapNode*> findParents(const MapData& map, const MapNode& node) const;
    std::vector<MapNode*> findSiblings(const MapData& map, const MapNode& node) const;
    bool ruleAssignableToRow(const MapNode& node, NodeType room) const;
    bool ruleParentMatches(const MapData& map, const MapNode& node, NodeType room) const;
    bool ruleSiblingMatches(const MapData& map, const MapNode& node, NodeType room) const;
    NodeType getNextRoomTypeAccordingToRules(const MapData& map, MapNode& node, std::vector<NodeType>& roomList) const;

    MapGeneratorParams params_;
    mutable std::mt19937 rng_;
};
