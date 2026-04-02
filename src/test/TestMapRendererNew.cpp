#include "src/map/MapRenderer.h"

int main() {
    MapData map(15);
    map[0]  = std::vector<MapNode>{{3,1,NodeType::MONSTER,{2,3,4}}};
    map[1]  = std::vector<MapNode>{{2,2,NodeType::MONSTER,{1,2,3}}, {3,2,NodeType::ELITE,{3,4,5}}, {4,2,NodeType::REST,{4,5}}};
    map[2]  = std::vector<MapNode>{{1,3,NodeType::REST,{0,1,2}}, {2,3,NodeType::MERCHANT,{2,3}}, {3,3,NodeType::MONSTER,{3,4}}, {4,3,NodeType::TREASURE,{4,5}}, {5,3,NodeType::ELITE,{4,5,6}}};
    map[3]  = std::vector<MapNode>{{0,4,NodeType::MONSTER,{0,1}}, {1,4,NodeType::ELITE,{1,2}}, {2,4,NodeType::UNKNOWN,{2,3}}, {3,4,NodeType::REST,{3,4}}, {4,4,NodeType::MONSTER,{4,5}}, {5,4,NodeType::MERCHANT,{5,6}}};
    map[4]  = std::vector<MapNode>{{0,5,NodeType::REST,{0,1}}, {1,5,NodeType::TREASURE,{1,2}}, {2,5,NodeType::MONSTER,{2,3}}, {3,5,NodeType::UNKNOWN,{3,4}}, {4,5,NodeType::ELITE,{4,5}}, {5,5,NodeType::REST,{5,6}}, {6,5,NodeType::MONSTER,{5,6}}};
    map[5]  = std::vector<MapNode>{{0,6,NodeType::ELITE,{0,1}}, {1,6,NodeType::MONSTER,{1,2}}, {2,6,NodeType::REST,{2,3}}, {3,6,NodeType::MERCHANT,{3,4}}, {4,6,NodeType::MONSTER,{4,5}}, {6,6,NodeType::ELITE,{5,6}}};
    map[6]  = std::vector<MapNode>{{0,7,NodeType::UNKNOWN,{0,1}}, {1,7,NodeType::REST,{1,2}}, {2,7,NodeType::TREASURE,{2,3}}, {3,7,NodeType::MONSTER,{3,4}}, {4,7,NodeType::REST,{4,5}}, {5,7,NodeType::UNKNOWN,{5,6}}};
    map[7]  = std::vector<MapNode>{{0,8,NodeType::MONSTER,{0,1}}, {1,8,NodeType::ELITE,{1,2}}, {2,8,NodeType::MERCHANT,{2,3}}, {3,8,NodeType::UNKNOWN,{3,4}}, {4,8,NodeType::MONSTER,{4,5}}, {5,8,NodeType::REST,{5,6}}};
    map[8]  = std::vector<MapNode>{{0,9,NodeType::REST,{0,1}}, {1,9,NodeType::MONSTER,{1,2}}, {2,9,NodeType::ELITE,{2,3}}, {3,9,NodeType::REST,{3,4}}, {4,9,NodeType::UNKNOWN,{4,5}}, {5,9,NodeType::MERCHANT,{5,6}}};
    map[9]  = std::vector<MapNode>{{0,10,NodeType::TREASURE,{0,1}}, {1,10,NodeType::UNKNOWN,{1,2}}, {2,10,NodeType::MONSTER,{2,3}}, {3,10,NodeType::ELITE,{3,4}}, {4,10,NodeType::REST,{4,5}}, {5,10,NodeType::UNKNOWN,{5,6}}};
    map[10] = std::vector<MapNode>{{0,11,NodeType::ELITE,{0,1}}, {1,11,NodeType::REST,{1,2}}, {2,11,NodeType::UNKNOWN,{2,3}}, {3,11,NodeType::MONSTER,{3,4}}, {4,11,NodeType::ELITE,{4,5}}, {5,11,NodeType::MONSTER,{5,6}}};
    map[11] = std::vector<MapNode>{{0,12,NodeType::MONSTER,{0,1}}, {1,12,NodeType::UNKNOWN,{1,2}}, {2,12,NodeType::REST,{2,3}}, {3,12,NodeType::MERCHANT,{3,4}}, {4,12,NodeType::MONSTER,{4,5}}, {5,12,NodeType::REST,{5,6}}};
    map[12] = std::vector<MapNode>{{1,13,NodeType::ELITE,{1,2}}, {2,13,NodeType::MONSTER,{2,3}}, {3,13,NodeType::UNKNOWN,{3,4}}, {4,13,NodeType::REST,{3,4,5}}};
    map[13] = std::vector<MapNode>{{1,14,NodeType::REST,{2,3}}, {2,14,NodeType::MERCHANT,{2,3}}, {3,14,NodeType::ELITE,{2,3,4}}, {4,14,NodeType::MONSTER,{3,4}}};
    map[14] = std::vector<MapNode>{{3,15,NodeType::BOSS,{}}};

    MapRenderer::render(map);
    return 0;
}
