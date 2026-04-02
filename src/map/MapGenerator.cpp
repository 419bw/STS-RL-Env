#include "src/map/MapGenerator.h"
#include <algorithm>
#include <numeric>

MapGenerator::MapGenerator(const MapGeneratorParams& params) : params_(params) {
    if (params_.seed == 0)
        rng_ = std::mt19937(std::random_device{}());
    else
        rng_ = std::mt19937(params_.seed);
}

MapData MapGenerator::generate() const {
    MapData map(params_.height);
    buildTopology(map);
    applyFixedFloors(map);
    assignRoomTypes(map);
    return map;
}

NodeType MapGenerator::getNodeType(int y) const {
    if (y == 1) return NodeType::MONSTER;
    if (y == 9) return NodeType::TREASURE;
    if (y == 15) return NodeType::REST;
    return NodeType::NONE;
}

void MapGenerator::applyFixedFloors(MapData& map) const {
    for (int y : {1, 9, 15}) {
        int idx = y - 1;
        if (idx >= 0 && idx < (int)map.size()) {
            for (auto& node : map[idx]) {
                node.type = getNodeType(y);
            }
        }
    }
}

int MapGenerator::countValidNodes(const MapData& map) const {
    int H = (int)map.size();
    int W = params_.width;
    std::vector<std::vector<bool>> valid(H, std::vector<bool>(W, false));

    for (int y = 0; y < H; ++y) {
        for (const auto& node : map[y]) {
            if (!node.next_x.empty()) {
                valid[y][node.x] = true;
            }
        }
    }

    for (int y = 0; y < H - 1; ++y) {
        for (const auto& node : map[y]) {
            for (int nx : node.next_x) {
                if (nx >= 0 && nx < W) {
                    valid[y][node.x] = true;
                }
            }
        }
    }

    int total = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (valid[y][x]) ++total;
        }
    }
    return total;
}

void MapGenerator::fillRoomList(std::vector<NodeType>& roomList, int totalNodes) const {
    int shops  = static_cast<int>(std::round(totalNodes * 0.05));
    int rests  = static_cast<int>(std::round(totalNodes * 0.12));
    int events = static_cast<int>(std::round(totalNodes * 0.22));
    int elites = static_cast<int>(std::round(totalNodes * 0.08));

    if (params_.ascensionLevel >= 1) {
        elites = static_cast<int>(std::round(totalNodes * 0.128));
    }

    int monsters = totalNodes - shops - rests - events - elites;

    for (int i = 0; i < shops;  ++i) roomList.push_back(NodeType::MERCHANT);
    for (int i = 0; i < rests;  ++i) roomList.push_back(NodeType::REST);
    for (int i = 0; i < events; ++i) roomList.push_back(NodeType::UNKNOWN);
    for (int i = 0; i < elites;  ++i) roomList.push_back(NodeType::ELITE);
    for (int i = 0; i < monsters; ++i) roomList.push_back(NodeType::MONSTER);

    std::shuffle(roomList.begin(), roomList.end(), rng_);
}

std::vector<MapNode*> MapGenerator::findParents(const MapData& map, const MapNode& node) const {
    std::vector<MapNode*> parents;
    int prevY = node.y - 2;
    if (prevY < 0) return parents;
    for (auto& n : map[prevY]) {
        for (int nx : n.next_x) {
            if (nx == node.x) {
                parents.push_back(const_cast<MapNode*>(&n));
                break;
            }
        }
    }
    return parents;
}

std::vector<MapNode*> MapGenerator::findSiblings(const MapData& map, const MapNode& node) const {
    std::vector<MapNode*> parents = findParents(map, node);
    std::vector<MapNode*> siblings;
    for (MapNode* parent : parents) {
        for (int nx : parent->next_x) {
            if (nx != node.x) {
                for (auto& n : map[node.y - 1]) {
                    if (n.x == nx) {
                        siblings.push_back(const_cast<MapNode*>(&n));
                        break;
                    }
                }
            }
        }
    }
    return siblings;
}

bool MapGenerator::ruleAssignableToRow(const MapNode& node, NodeType room) const {
    int y = node.y - 1;
    if (y <= 4) {
        if (room == NodeType::REST || room == NodeType::ELITE) return false;
    }
    if (y >= 13) {
        if (room == NodeType::REST) return false;
    }
    return true;
}

bool MapGenerator::ruleParentMatches(const MapData& map, const MapNode& node, NodeType room) const {
    static const NodeType NO_CONSECUTIVE[] = {
        NodeType::REST, NodeType::TREASURE, NodeType::MERCHANT, NodeType::ELITE
    };
    for (NodeType forbidden : NO_CONSECUTIVE) {
        if (room == forbidden) {
            std::vector<MapNode*> parents = findParents(map, node);
            for (MapNode* p : parents) {
                if (p->type == room) return false;
            }
            break;
        }
    }
    return true;
}

bool MapGenerator::ruleSiblingMatches(const MapData& map, const MapNode& node, NodeType room) const {
    static const NodeType NO_SIBLING_SAME[] = {
        NodeType::REST, NodeType::MONSTER, NodeType::UNKNOWN, NodeType::ELITE, NodeType::MERCHANT
    };
    for (NodeType forbidden : NO_SIBLING_SAME) {
        if (room == forbidden) {
            std::vector<MapNode*> siblings = findSiblings(map, node);
            for (MapNode* s : siblings) {
                if (s->type == room) return false;
            }
            break;
        }
    }
    return true;
}

NodeType MapGenerator::getNextRoomTypeAccordingToRules(
    const MapData& map, MapNode& node, std::vector<NodeType>& roomList) const {

    if (node.y == 1) {
        if (!roomList.empty()) {
            NodeType t = roomList.back();
            roomList.pop_back();
            return t;
        }
        return NodeType::MONSTER;
    }

    for (auto it = roomList.rbegin(); it != roomList.rend(); ++it) {
        NodeType candidate = *it;
        if (!ruleAssignableToRow(node, candidate)) continue;
        if (!ruleParentMatches(map, node, candidate)) continue;
        if (!ruleSiblingMatches(map, node, candidate)) continue;

        it = std::vector<NodeType>::reverse_iterator(roomList.erase(std::next(it).base()));
        return candidate;
    }

    return NodeType::MONSTER;
}

void MapGenerator::assignRoomTypes(MapData& map) const {
    int total = countValidNodes(map);
    std::vector<NodeType> roomList;
    fillRoomList(roomList, total);

    for (int y = 0; y < (int)map.size(); ++y) {
        if (getNodeType(y + 1) != NodeType::NONE) continue;
        for (auto& node : map[y]) {
            if (node.next_x.empty()) {
                node.type = NodeType::MONSTER;
            } else {
                node.type = getNextRoomTypeAccordingToRules(map, node, roomList);
            }
        }
    }

    for (auto& layer : map) {
        for (auto& node : layer) {
            if (node.type == NodeType::NONE) {
                node.type = NodeType::MONSTER;
            }
        }
    }
}

void MapGenerator::buildTopology(MapData& map) const {
    int H = params_.height;
    int W = params_.width;
    int PD = params_.pathDensity;

    if (H <= 0 || W <= 0 || PD <= 0) return;

    struct PathState {
        int x;
        int y;
        int ancestor_x;
        int ancestor_y;
        int row;
    };

    std::vector<std::vector<bool>> hasNode(H, std::vector<bool>(W, false));
    std::vector<std::vector<std::vector<int>>> nextLinks(H, std::vector<std::vector<int>>(W));
    std::vector<std::vector<std::vector<int>>> prevLinks(H, std::vector<std::vector<int>>(W));

    std::vector<PathState> paths(PD);

    for (int i = 0; i < PD; ++i) {
        int x;
        if (i == 0) {
            x = std::uniform_int_distribution<int>(0, W - 1)(rng_);
        } else {
            do {
                x = std::uniform_int_distribution<int>(0, W - 1)(rng_);
            } while (i == 1 && x == paths[0].x);
        }
        paths[i] = {x, 0, x, 0, 0};
        hasNode[0][x] = true;
    }

    for (int row = 0; row < H - 1; ++row) {
        std::vector<PathState> nextPaths;
        std::vector<PathState> survivingPaths;

        for (auto& p : paths) {
            if (p.row != row) {
                survivingPaths.push_back(p);
                continue;
            }

            if (p.y >= H - 1) continue;

            std::vector<int> dirs;
            if (p.x == 0) {
                dirs = {0, 1};
            } else if (p.x == W - 1) {
                dirs = {-1, 0};
            } else {
                dirs = {-1, 0, 1};
            }

            std::vector<int> candidates;
            for (int d : dirs) {
                candidates.push_back(p.x + d);
            }

            int chosenX = p.x;
            std::uniform_int_distribution<int> dist(0, (int)candidates.size() - 1);
            int dirIdx = dist(rng_);
            chosenX = candidates[dirIdx];
            int chosenDir = dirs[dirIdx];

            bool blocked = false;
            if (!prevLinks[p.y + 1][chosenX].empty()) {
                int ancestorGap = (p.y + 1) - p.ancestor_y;
                if (ancestorGap < params_.minAncestorGap) {
                    blocked = true;
                }
            }

            if (blocked) {
                std::vector<int> alts;
                if (chosenDir == 0) {
                    if (p.x > 0) alts.push_back(p.x - 1);
                    if (p.x < W - 1) alts.push_back(p.x + 1);
                } else if (chosenDir == -1) {
                    if (p.x < W - 1) alts.push_back(p.x + 1);
                    alts.push_back(p.x);
                } else {
                    if (p.x > 0) alts.push_back(p.x - 1);
                    alts.push_back(p.x);
                }
                if (!alts.empty()) {
                    std::uniform_int_distribution<int> adist(0, (int)alts.size() - 1);
                    chosenX = alts[adist(rng_)];
                }
            }

            int leftMaxX = -1;
            for (int lx = 0; lx < p.x; ++lx) {
                if (nextLinks[p.y][lx].size() > 0) {
                    for (int nx : nextLinks[p.y][lx]) {
                        leftMaxX = std::max(leftMaxX, nx);
                    }
                }
            }

            int rightMinX = W;
            for (int rx = p.x + 1; rx < W; ++rx) {
                if (nextLinks[p.y][rx].size() > 0) {
                    for (int nx : nextLinks[p.y][rx]) {
                        rightMinX = std::min(rightMinX, nx);
                    }
                }
            }

            if (chosenX < leftMaxX + 1) chosenX = leftMaxX + 1;
            if (chosenX > rightMinX - 1) chosenX = rightMinX - 1;
            chosenX = std::max(0, std::min(W - 1, chosenX));

            hasNode[p.y + 1][chosenX] = true;
            nextLinks[p.y][p.x].push_back(chosenX);
            prevLinks[p.y + 1][chosenX].push_back(p.x);

            bool merged = false;
            for (auto& sp : survivingPaths) {
                if (sp.x == chosenX && sp.y == p.y + 1) {
                    merged = true;
                    sp.ancestor_x = chosenX;
                    sp.ancestor_y = p.y + 1;
                    break;
                }
            }

            if (!merged) {
                survivingPaths.push_back({chosenX, p.y + 1, chosenX, p.y + 1, row + 1});
            }

            (void)nextPaths;
        }

        paths = survivingPaths;

        for (int x = 0; x < W; ++x) {
            auto& v = nextLinks[row][x];
            std::sort(v.begin(), v.end());
            v.erase(std::unique(v.begin(), v.end()), v.end());
        }
    }

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (hasNode[y][x]) {
                map[y].push_back({x, y + 1, NodeType::NONE, nextLinks[y][x]});
            }
        }
    }
}
