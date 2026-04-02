#pragma once

#include <string>
#include <vector>
#include <map>

// ==========================================
// 数据结构（来自规范，不可修改）
// ==========================================
enum class NodeType {
    MONSTER, ELITE, REST, MERCHANT, TREASURE, UNKNOWN, BOSS, NONE
};

struct MapNode {
    int x;
    int y;
    NodeType type;
    std::vector<int> next_x;
};

using MapData = std::vector<std::vector<MapNode>>;

// ==========================================
// ANSI 颜色宏
// ==========================================
#define COLOR_RESET   "\033[0m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BOLD    "\033[1m"

// ==========================================
// MapRenderer — 虚拟画布渲染器
//
// 每层 ROWS_PER_FLOOR=4 行，节点画在 offset=3（层底），connector 画在 offset=0（层顶）：
//   row 0: connector row (midpoint between y and y+1)
//   row 1: blank
//   row 2: node box
// 行公式: nodeRow(y) = (maxFloor-y)*4 + 3
//         midRow(y,y+1) = (nodeRow(y) + nodeRow(y+1))/2 = (maxFloor-y)*4 - 1
//
// 列: COL_WIDTH=6, PADDING_LEFT=3
// canvasX(col) = col * 6 + 4   (节点盒占 col*6+4 .. col*6+6)
// midCol(x, nx) = (canvasX(x) + canvasX(nx)) / 2 = (x+nx)*3 + 4
// ==========================================
class MapRenderer {
public:
    static void render(const MapData& map);

private:
    static constexpr int COLS            = 7;
    static constexpr int COL_WIDTH       = 5;
    static constexpr int ROWS_PER_FLOOR = 3;
    static constexpr int PADDING_LEFT   = 2;

    static int maxFloorFromMap(const MapData& map);
    static int nodeRow(int floor, int maxFloor);
    static int midRow(int y1, int y2, int maxFloor);
    static int canvasX(int col);
    static const char* nodeColor(NodeType type);
    static char        nodeSymbol(NodeType type);
};
