#include "src/map/MapRenderer.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// ============================================================
// Boss 区域高度（单位：行）
//
// Boss 区域布局（从下到上，共 4 行）：
//   row 30 (bossTop-1)     : REST→Boss 连接竖线（phantom slash 行）
//   row 31 (bossTop)       : +------------------------------+  ← Boss 顶边框
//   row 32 (bossContentRow): |             [B]              |  ← Boss 内容行
//   row 33 (bossBotRow)    : +------------------------------+  ← Boss 底边框
//
// 计算公式（以 maxFloor=15 为例）：
//   totalRows = maxFloor * 2 + BOSS_HEIGHT = 15*2 + 4 = 34
//   bossTop = maxFloor * 2 + 1 = 31
//   bossContentRow = bossTop + 1 = 32
//   bossBotRow = bossTop + 2 = 33（正好是最后一行）
//
// REST 节点在第 15 层，nodeRow = (15-1)*2+1 = 29
// 连接竖线从 row 30 画到 row 30（bossTop-1）
// ============================================================
static const int BOSS_HEIGHT = 4;

int MapRenderer::maxFloorFromMap(const MapData& map) {
    int maxFloor = 0;
    for (const auto& floor : map) {
        for (const auto& node : floor) {
            if (node.y > maxFloor) maxFloor = node.y;
        }
    }
    return maxFloor;
}

// ============================================================
// 节点行坐标计算
//
// 每个 floor 占 2 行：第 1 行（奇数行）放节点盒子，第 2 行（偶数行）放该层所有 slash 斜线。
// floor=1 (y=1)  → nodeRow = (1-1)*2+1 = 1
// floor=2 (y=2)  → nodeRow = (2-1)*2+1 = 3
// ...
// floor=15 (y=15)→ nodeRow = (15-1)*2+1 = 29
//
// 参数 maxFloor 未使用（保留接口兼容）
// ============================================================
int MapRenderer::nodeRow(int floor, int maxFloor) {
    (void)maxFloor;
    return (floor - 1) * 2 + 1;
}

// ============================================================
// Slash 斜线行坐标计算（连接两个相邻层的斜线所在行）
//
// 公式：midRow(y1, y2) = y1 * 2
// 即：源节点所在层的"偶数行"（第 2 行）
//
// 例如：y1=14, y2=15
//   midRow = 14 * 2 = 28
//   这表示连接 floor=14 层到 floor=15 层的 slash 在 canvas row 28
//
// 而 REST (y=15) 的 nodeRow = 29，
// 所以竖线要从 REST 节点底部（row 29）往上画，
// 穿过 row 28 的 slash 行（phantom slash 行），连接到 Boss 区域。
// ============================================================
int MapRenderer::midRow(int y1, int y2, int maxFloor) {
    (void)maxFloor;
    (void)y2;
    return y1 * 2;
}

// ============================================================
// 列坐标计算
//
// 每个房间节点盒子占 COL_WIDTH=4 个 canvas 列（列索引 0~6 对应 7 个房间列）：
//   col=0 → canvasX = 0*4+2 = 2   → [X] 在 col 2,3,4
//   col=1 → canvasX = 1*4+2 = 6   → [X] 在 col 6,7,8
//   col=2 → canvasX = 2*4+2 = 10  → [X] 在 col 10,11,12
//   ...
//   col=6 → canvasX = 6*4+2 = 26  → [X] 在 col 26,27,28
// 总宽度 = 7*4+4 = 32 列
//
// 盒子内符号位置：
//   canvasX(col)       → '['
//   canvasX(col) + 1   → 符号（如 M/E/R/?）
//   canvasX(col) + 2   → ']'
//   canvasX(col) + 1   → 竖线 '|' 的位置（用于 REST 垂直连接）
// ============================================================
int MapRenderer::canvasX(int col) {
    return col * 4 + 2;
}

const char* MapRenderer::nodeColor(NodeType type) {
    switch (type) {
        case NodeType::MONSTER:  return COLOR_WHITE;
        case NodeType::ELITE:   return COLOR_RED;
        case NodeType::BOSS:    return COLOR_MAGENTA COLOR_BOLD;
        case NodeType::MERCHANT: return COLOR_YELLOW;
        case NodeType::REST:    return COLOR_GREEN;
        case NodeType::TREASURE: return COLOR_MAGENTA;
        case NodeType::UNKNOWN:  return COLOR_CYAN;
        case NodeType::NONE:    return COLOR_RESET;
        default:                return COLOR_WHITE;
    }
}

char MapRenderer::nodeSymbol(NodeType type) {
    switch (type) {
        case NodeType::MONSTER:  return 'M';
        case NodeType::ELITE:   return 'E';
        case NodeType::BOSS:    return 'B';
        case NodeType::MERCHANT: return '$';
        case NodeType::REST:    return 'R';
        case NodeType::TREASURE: return 'T';
        case NodeType::UNKNOWN:  return '?';
        case NodeType::NONE:    return '-';
        default:                return '?';
    }
}

void MapRenderer::render(const MapData& map) {
    if (map.empty()) return;

    // ========================================================
    // 启用 Windows Virtual Terminal（ANSI 颜色）
    // ========================================================
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, ENABLE_VIRTUAL_TERMINAL_PROCESSING | dwMode);

    int maxFloor = maxFloorFromMap(map);
    if (maxFloor == 0) return;

    // ========================================================
    // Canvas 尺寸计算
    //
    // 总行数 = maxFloor层的节点行(每层2行) + BOSS_HEIGHT
    //   maxFloor=15 → 15*2 + 5 = 35 行
    // 总列数 = 7列 * 每列宽度4 + 左右各2列边距 = 32 列
    // ========================================================
    int totalRows = maxFloor * 2 + BOSS_HEIGHT;  // 例如：15*2+5=35
    int totalCols = 7 * 4 + 4;                    // 固定32列
    std::vector<std::string> canvas(totalRows, std::string(totalCols, ' '));

    // 用于输出时查找节点类型的辅助 map
    // key = {floor(y), column(x)}，value = NodeType
    std::map<std::pair<int,int>, NodeType> nodeTypes;

    // ========================================================
    // 第一遍：绘制所有普通节点和层间连接斜线
    // ========================================================
    for (const auto& floor : map) {
        for (const auto& node : floor) {
            // 记录节点类型，用于输出时着色
            nodeTypes[{node.y, node.x}] = node.type;

            // 计算节点在 canvas 上的行坐标
            // 例如 floor=15 → row = 29（nodeRow）
            int row = nodeRow(node.y, maxFloor);
            int col = canvasX(node.x);

            // 绘制节点盒子：[X]
            canvas[row][col]     = '[';
            canvas[row][col + 1] = nodeSymbol(node.type);
            canvas[row][col + 2] = ']';

            // 跳过本层到 Boss 的连线（这部分在后面单独处理）
            if (node.y == maxFloor) continue;

            // 计算连接斜线所在的 canvas 行
            // midRow = y1*2，例如 y1=14 → 28
            int connRow = midRow(node.y, node.y + 1, maxFloor);
            if (connRow < 0 || connRow >= totalRows) continue;

            // 绘制所有下一层连接斜线
            for (int nx : node.next_x) {
                int midCol;
                if (nx == node.x) {
                    // 正上方：竖线位置在盒子内 '[' 后符号位
                    midCol = canvasX(node.x) + 1;
                } else if (nx > node.x) {
                    // 斜向右：slash'/' 的位置 = 目标盒子左边框 - 1
                    // canvasX(nx) 是目标盒子 '[' 位置，'[' 前一列是 slash '/'
                    midCol = canvasX(nx) - 1;
                } else {
                    // 斜向左：backslash '\' 的位置 = 目标盒子右边框 + 1
                    // canvasX(nx)+2 是目标盒子 ']' 位置，']' 后一列是 '\'
                    midCol = canvasX(nx) + 3;
                }
                char ch = (nx == node.x) ? '|' : ((nx > node.x) ? '/' : '\\');
                if (midCol >= 0 && midCol < totalCols) {
                    canvas[connRow][midCol] = ch;
                }
            }
        }
    }

    // ========================================================
    // 第二遍：绘制 Boss 区域
    //
    // Boss 区域占 3 行（bossTop, bossContentRow, bossBotRow）
    // 加上 1 行连接线空间，共 BOSS_HEIGHT=4 行。
    //
    // 绘制顺序（从下到上）：
    //   row 31 (bossTop)       : +------------------------------+  ← Boss 顶框
    //   row 32 (bossContentRow): |             [B]              |  ← Boss 内容
    //   row 33 (bossBotRow)    : +------------------------------+  ← Boss 底框
    //
    // 注意：bossTop = maxFloor * 2 + 1
    //   maxFloor=15 → bossTop = 31
    //   bossBotRow = 33（画布最后一行）
    // ========================================================
    int bossTop = maxFloor * 2 + 1;
    int bossContentRow = bossTop + 1;
    int bossBotRow = bossTop + 2;

    // 绘制 Boss 顶边框（'+----...---+'）
    canvas[bossTop][0] = '+';
    canvas[bossTop][totalCols - 1] = '+';
    for (int c = 1; c < totalCols - 1; ++c) {
        canvas[bossTop][c] = '-';
    }

    // 绘制 Boss 底边框
    canvas[bossBotRow][0] = '+';
    canvas[bossBotRow][totalCols - 1] = '+';
    for (int c = 1; c < totalCols - 1; ++c) {
        canvas[bossBotRow][c] = '-';
    }

    // 绘制 Boss 两侧边框 + [B] 盒子
    canvas[bossContentRow][0] = '|';
    canvas[bossContentRow][totalCols - 1] = '|';
    int bossContentCol = totalCols / 2 - 2;
    canvas[bossContentRow][bossContentCol]     = '[';
    canvas[bossContentRow][bossContentCol + 1] = 'B';
    canvas[bossContentRow][bossContentCol + 2] = ']';

    // ========================================================
    // 第三遍：绘制 REST→Boss 的垂直连接线和 slash 斜线
    //
    // 遍历所有 REST 节点（y == maxFloor，即 floor=15）：
    //   每个 REST 的竖线从 bossTop-1（phantom slash 行）画到 REST 节点正上方一行。
    //
    // 连接线布局（从上到下）：
    //   row 30 (bossTop)       : 已被 Boss 顶边框覆盖，跳过
    //   row 29 (bossTop-1)     : phantom slash '/' '\'，竖线会在这里画 '|'
    //   row 28 (bossTop-2)     : 竖线 '|' 继续延伸
    //   ...
    //   row 28 → row 30       : 三行组成完整的连接
    //
    // 计算竖线位置的公式：
    //   REST nodeRow = (15-1)*2+1 = 29
    //   竖线列位置 = canvasX(x) + 1 = '[' 后面那位
    //
    // 竖线绘制范围：
    //   r 从 bossTop-1（29）递减到 nodeRow+1（30）
    //   （即从 phantom slash 行一直画到 REST 节点盒子正上方）
    //   这样竖线在 row 29 覆盖 phantom slash，在 row 28 单独显示 '|'
    // ========================================================
    for (const auto& floor : map) {
        for (const auto& node : floor) {
            // 只处理 REST 节点，且位于最高层（floor=15，即 y=maxFloor）
            if (node.y != maxFloor || node.type != NodeType::REST) continue;

            int row = nodeRow(node.y, maxFloor);  // row = 29（REST 盒子所在行）
            int col = canvasX(node.x);
            int cx = col + 1;  // 竖线画在这一列（盒子内 '[' 后符号位）

            // 从 bossTop-1（phantom slash 行）往上画到 REST 盒子正上方
            for (int r = bossTop - 1; r >= row + 1; --r) {
                if (r >= 0 && r < totalRows) {
                    canvas[r][cx] = '|';
                }
            }
        }
    }

    // ========================================================
    // 第四遍：输出 canvas
    // 逆序输出（从最后一行 row=34 打印到 row=0），使最高楼层显示在最上方
    // ========================================================
    for (int r = totalRows - 1; r >= 0; --r) {
        std::string out;
        int c = 0;
        while (c < totalCols) {
            // ================================================
            // 检测节点盒子并着色输出
            // 盒子格式：[X]，占 3 字符（c, c+1, c+2）
            // 节点盒子只在奇数行（rowInGroup == 1，即 nodeRow）出现
            // ================================================
            if (canvas[r][c] == '[' && c + 2 < totalCols && canvas[r][c+2] == ']') {
                int rowInGroup = r % 2;
                int floor = r / 2 + 1;
                int colIdx = (c - 2) / 4;  // 从 canvas 列反推节点列
                if (colIdx >= 0 && colIdx < 7 && rowInGroup == 1) {
                    auto it = nodeTypes.find({floor, colIdx});
                    if (it != nodeTypes.end()) {
                        // 注入颜色：color + [X] + reset
                        out += nodeColor(it->second);
                        out += canvas[r][c];
                        out += canvas[r][c+1];
                        out += canvas[r][c+2];
                        out += COLOR_RESET;
                        c += 3;
                        continue;
                    }
                }
            }
            // Boss [B] 盒子着色（BOSS_HEIGHT 区域内的行）
            if (r == bossContentRow && c == bossContentCol) {
                out += nodeColor(NodeType::BOSS);
                out += canvas[r][c];
                out += canvas[r][c+1];
                out += canvas[r][c+2];
                out += COLOR_RESET;
                c += 3;
                continue;
            }
            out += canvas[r][c];
            ++c;
        }

        // 去除尾部空格后输出
        size_t lastNonSpace = out.find_last_not_of(' ');
        if (lastNonSpace != std::string::npos) {
            std::cout << out.substr(0, lastNonSpace + 1) << "\n";
        } else {
            std::cout << "\n";
        }
    }
}
