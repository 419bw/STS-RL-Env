#include "src/map/MapGenerator.h"
#include <algorithm>
#include <functional>
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

    int y = node.y - 1;

    for (auto it = roomList.begin(); it != roomList.end(); ++it) {
        NodeType candidate = *it;
        if (!ruleAssignableToRow(node, candidate)) continue;

        if (y != 0) {
            if (!ruleParentMatches(map, node, candidate)) continue;
            if (!ruleSiblingMatches(map, node, candidate)) continue;
        }

        roomList.erase(it);
        return candidate;
    }

    return NodeType::MONSTER;
}

void MapGenerator::assignRoomTypes(MapData& map) const {
    int total = countValidNodes(map);
    std::vector<NodeType> roomList;
    fillRoomList(roomList, total);

    while (roomList.size() < static_cast<size_t>(total)) {
        roomList.push_back(NodeType::MONSTER);
    }

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

namespace {

int randRange(std::mt19937& rng, int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

struct InternalNode {
    int x;
    int y;
    std::vector<int> edges;
    std::vector<int> parents;
};

int getCommonAncestor(int x1, int y1, int x2, int y2,
                      const std::vector<std::vector<InternalNode>>& nodes,
                      int maxDepth) {
    if (y1 != y2 || x1 == x2) return -1;

    int l_x = x1, l_y = y1;
    int r_x = x2, r_y = y2;

    // 注意：这里使用 x1 < y2 是原版源码的 Bug，应该用 x1 < x2
    // 但为复刻原版行为，保留此 Bug
    if (x1 < y2) {
        l_x = x1; l_y = y1;
        r_x = x2; r_y = y2;
    } else {
        l_x = x2; l_y = y2;
        r_x = x1; r_y = y1;
    }

    int currentY = y1;
    int minY = std::max(0, y1 - maxDepth);

    while (currentY >= minY) {
        if (l_x < 0 || r_x < 0) break;
        if (currentY < 0 || currentY >= (int)nodes.size()) break;

        const auto& lNode = nodes[currentY][l_x];
        const auto& rNode = nodes[currentY][r_x];

        if (lNode.parents.empty() || rNode.parents.empty()) break;

        int lParentX = -1, rParentX = -1;
        for (int px : lNode.parents) {
            lParentX = std::max(lParentX, px);
        }
        for (int px : rNode.parents) {
            rParentX = std::min(rParentX, px);
        }

        if (lParentX == rParentX && currentY > 0) {
            return currentY;
        }

        l_x = lParentX;
        r_x = rParentX;
        currentY--;
    }

    return -1;
}

int getMaxEdge(const std::vector<int>& edges) {
    if (edges.empty()) return -1;
    return *std::max_element(edges.begin(), edges.end());
}

int getMinEdge(const std::vector<int>& edges) {
    if (edges.empty()) return -1;
    return *std::min_element(edges.begin(), edges.end());
}

}

void MapGenerator::buildTopology(MapData& map) const {
    int H = params_.height;
    int W = params_.width;
    int PD = params_.pathDensity;

    if (H <= 0 || W <= 0 || PD <= 0) return;

    std::vector<std::vector<InternalNode>> nodes(H, std::vector<InternalNode>(W));
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            nodes[y][x] = InternalNode{x, y, {}, {}};
        }
    }

    // ============================================================
    // 路径生成核心函数（模拟源码的 _createPaths 递归逻辑）
    //
    // 从 (srcX, srcY) 开始，逐行向下生成路径，直到到达地图底部。
    // 每一步随机选择左/直/右方向，并经过祖先间距检查和邻居交叉检查。
    // ============================================================
    std::function<void(int, int)> createPath = [&](int srcX, int srcY) {
        int currentX = srcX;
        int currentY = srcY;

        while (true) {
            if (currentY >= H) break;

            // ========================================================
            // 到达最后一行：创建 Boss 边
            //
            // Boss 边是一个特殊的边，值为 W（超出正常 x 坐标范围 [0, W-1]）。
            // 这代表通往 Boss 的出口，不是连接到下一层的普通边。
            //
            // 注意：Boss 边必须被正确处理，否则最后一行的节点不会被添加到 map 中。
            // 参见下方的节点添加逻辑（hasBossEdge 检查）。
            // ========================================================
            if (currentY + 1 >= H) {
                nodes[currentY][currentX].edges.push_back(W);
                std::sort(nodes[currentY][currentX].edges.begin(),
                          nodes[currentY][currentX].edges.end());
                break;
            }

            int rowEndNode = W - 1;
            int min, max;
            if (currentX == 0) {
                min = 0; max = 1;
            } else if (currentX == rowEndNode) {
                min = -1; max = 0;
            } else {
                min = -1; max = 1;
            }

            int newEdgeX = currentX + randRange(rng_, min, max);
            int newEdgeY = currentY + 1;

            InternalNode& targetCandidate = nodes[newEdgeY][newEdgeX];

            if (!targetCandidate.parents.empty()) {
                for (int parentX : targetCandidate.parents) {
                    if (parentX != currentX) {
                        int ancestorY = getCommonAncestor(
                            parentX, newEdgeY, currentX, currentY,
                            nodes, params_.maxAncestorGap);

                        if (ancestorY >= 0) {
                            int ancestorGap = newEdgeY - ancestorY;
                            if (ancestorGap < params_.minAncestorGap) {
                                if (newEdgeX > currentX) {
                                    newEdgeX = currentX + randRange(rng_, -1, 0);
                                    if (newEdgeX < 0) newEdgeX = currentX;
                                } else if (newEdgeX == currentX) {
                                    newEdgeX = currentX + randRange(rng_, -1, 1);
                                    if (newEdgeX > rowEndNode) newEdgeX = currentX - 1;
                                    else if (newEdgeX < 0) newEdgeX = currentX + 1;
                                } else {
                                    newEdgeX = currentX + randRange(rng_, 0, 1);
                                    if (newEdgeX > rowEndNode) newEdgeX = currentX;
                                }
                            }
                        }
                    }
                }
            }

            if (currentX > 0) {
                const InternalNode& leftNode = nodes[currentY][currentX - 1];
                if (!leftNode.edges.empty()) {
                    int leftMaxEdge = getMaxEdge(leftNode.edges);
                    if (leftMaxEdge >= 0 && leftMaxEdge > newEdgeX) {
                        newEdgeX = leftMaxEdge;
                    }
                }
            }

            if (currentX < rowEndNode) {
                const InternalNode& rightNode = nodes[currentY][currentX + 1];
                if (!rightNode.edges.empty()) {
                    int rightMinEdge = getMinEdge(rightNode.edges);
                    if (rightMinEdge >= 0 && rightMinEdge < newEdgeX) {
                        newEdgeX = rightMinEdge;
                    }
                }
            }

            newEdgeX = std::max(0, std::min(rowEndNode, newEdgeX));

            nodes[currentY][currentX].edges.push_back(newEdgeX);
            std::sort(nodes[currentY][currentX].edges.begin(),
                      nodes[currentY][currentX].edges.end());

            nodes[newEdgeY][newEdgeX].parents.push_back(currentX);

            currentX = newEdgeX;
            currentY = newEdgeY;
        }
    };

    int firstStartingNode = -1;
    for (int i = 0; i < PD; ++i) {
        int startingNode = randRange(rng_, 0, W - 1);

        if (i == 0) {
            firstStartingNode = startingNode;
        }

        while (startingNode == firstStartingNode && i == 1) {
            startingNode = randRange(rng_, 0, W - 1);
        }

        createPath(startingNode, 0);
    }

    {
        std::vector<std::pair<int, int>> existingEdges;
        std::vector<int> deleteList;

        for (int x = 0; x < W; ++x) {
            InternalNode& node = nodes[0][x];
            if (node.edges.empty()) continue;

            existingEdges.clear();
            deleteList.clear();

            for (size_t i = 0; i < node.edges.size(); ++i) {
                int edge = node.edges[i];
                bool isDuplicate = false;
                for (const auto& existing : existingEdges) {
                    if (existing.first == edge && existing.second == 1) {
                        isDuplicate = true;
                        deleteList.push_back(static_cast<int>(i));
                        break;
                    }
                }
                if (!isDuplicate) {
                    existingEdges.emplace_back(edge, 1);
                }
            }

            for (auto it = deleteList.rbegin(); it != deleteList.rend(); ++it) {
                node.edges.erase(node.edges.begin() + *it);
            }
        }
    }

    // ============================================================
    // 将内部节点结构转换为最终 MapData
    //
    // 关键点：Boss 边的处理
    //
    // 普通边的值范围是 [0, W-1]，代表下一层的 x 坐标。
    // Boss 边的值是 W，代表通往 Boss 的出口（不是真实的节点坐标）。
    //
    // 问题：最后一行的节点只有 Boss 边，没有普通边。
    // 如果只检查 nextXList 是否为空，这些节点会被遗漏。
    // 遗漏后，applyFixedFloors 无法设置第15层为 REST。
    //
    // 解决：额外检查 hasBossEdge，确保有 Boss 边的节点也被添加。
    // 注意：Boss 边不存储在 next_x 中（它不是真实的节点连接）。
    // ============================================================
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const InternalNode& inode = nodes[y][x];
            if (!inode.edges.empty()) {
                std::vector<int> nextXList;
                bool hasBossEdge = false;
                for (int edge : inode.edges) {
                    if (edge < W) {
                        nextXList.push_back(edge);
                    } else {
                        hasBossEdge = true;
                    }
                }
                if (!nextXList.empty() || hasBossEdge) {
                    map[y].push_back({x, y + 1, NodeType::NONE, nextXList});
                }
            }
        }
    }
}
