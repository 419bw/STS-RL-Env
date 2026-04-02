#pragma once

#include <vector>
#include <string>

// ==========================================
// 节点类型枚举 — 对应 STS 地图上的各种房间
// ==========================================
enum class NodeType {
    MONSTER,     // 普通怪物房 (M)
    ELITE,       // 精英怪房 (E)
    REST,        // 营火/休息 (R)
    MERCHANT,    // 商店 ($)
    TREASURE,    // 宝箱房 (T)
    UNKNOWN,     // 事件房 (?) — 玩家未探索前的问号状态
    BOSS,        // Boss 房 (B)
    NONE         // 骨架占位 — 无房间，仅用于拓扑填充前的空白节点
};

// ==========================================
// 地图节点 — 地图上每一个可交互的点
//
// 坐标系说明：
// - layer: Y轴（层级），0 = 底部起始层，值越大越靠近 Boss
// - x: X轴归一化坐标 [0.0, 1.0]，0=最左，1=最右
// ==========================================
struct MapNode {
    int index;              // 在 nodes 数组中的全局索引
    NodeType type;          // 节点类型
    int layer;              // 所在层级
    float x;                // 归一化横坐标 [0, 1]
    bool visited;           // 玩家是否已访问

    MapNode() : index(-1), type(NodeType::UNKNOWN), layer(0), x(0.5f), visited(false) {}
};

// ==========================================
// 地图边 — 连接两个相邻层节点的通路
// ==========================================
struct MapEdge {
    int fromNodeIndex;      // 起点 node index（下层）
    int toNodeIndex;        // 终点 node index（上层）

    MapEdge() : fromNodeIndex(-1), toNodeIndex(-1) {}
};
