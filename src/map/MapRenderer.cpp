#include "src/map/MapRenderer.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

int MapRenderer::maxFloorFromMap(const MapData& map) {
    int maxFloor = 0;
    for (const auto& floor : map) {
        for (const auto& node : floor) {
            if (node.y > maxFloor) maxFloor = node.y;
        }
    }
    return maxFloor;
}

int MapRenderer::nodeRow(int floor, int maxFloor) {
    (void)maxFloor;
    return (floor - 1) * 2 + 1;
}

int MapRenderer::midRow(int y1, int y2, int maxFloor) {
    (void)maxFloor;
    (void)y2;
    return y1 * 2;
}

int MapRenderer::canvasX(int col) {
    return col * 4 + 2;
}

const char* MapRenderer::nodeColor(NodeType type) {
    switch (type) {
        case NodeType::MONSTER:  return COLOR_WHITE;
        case NodeType::ELITE:    return COLOR_RED;
        case NodeType::BOSS:     return COLOR_MAGENTA COLOR_BOLD;
        case NodeType::MERCHANT: return COLOR_YELLOW;
        case NodeType::REST:     return COLOR_GREEN;
        case NodeType::TREASURE: return COLOR_MAGENTA;
        case NodeType::UNKNOWN:  return COLOR_CYAN;
        default:                 return COLOR_WHITE;
    }
}

char MapRenderer::nodeSymbol(NodeType type) {
    switch (type) {
        case NodeType::MONSTER:  return 'M';
        case NodeType::ELITE:    return 'E';
        case NodeType::BOSS:     return 'B';
        case NodeType::MERCHANT: return '$';
        case NodeType::REST:     return 'R';
        case NodeType::TREASURE: return 'T';
        case NodeType::UNKNOWN:  return '?';
        default:                 return '?';
    }
}

void MapRenderer::render(const MapData& map) {
    if (map.empty()) return;

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, ENABLE_VIRTUAL_TERMINAL_PROCESSING | dwMode);

    int maxFloor = maxFloorFromMap(map);
    if (maxFloor == 0) return;

    int totalRows = maxFloor * 2;
    int totalCols = 7 * 4 + 4;
    std::vector<std::string> canvas(totalRows, std::string(totalCols, ' '));

    std::map<std::pair<int,int>, NodeType> nodeTypes;

    for (const auto& floor : map) {
        for (const auto& node : floor) {
            nodeTypes[{node.y, node.x}] = node.type;

            int row = nodeRow(node.y, maxFloor);
            int col = canvasX(node.x);
            canvas[row][col]     = '[';
            canvas[row][col + 1] = nodeSymbol(node.type);
            canvas[row][col + 2] = ']';

            int connRow = midRow(node.y, node.y + 1, maxFloor);
            if (connRow < 0 || connRow >= totalRows) continue;

            for (int nx : node.next_x) {
                int midCol;
                if (nx == node.x) {
                    midCol = canvasX(node.x) + 1;
                } else if (nx > node.x) {
                    midCol = canvasX(nx) - 1;
                } else {
                    midCol = canvasX(nx) + 3;
                }
                char ch = (nx == node.x) ? '|' : ((nx > node.x) ? '/' : '\\');
                if (midCol >= 0 && midCol < totalCols) {
                    canvas[connRow][midCol] = ch;
                }
            }
        }
    }

    for (int r = totalRows - 1; r >= 0; --r) {
        std::string out;
        int c = 0;
        while (c < totalCols) {
            if (canvas[r][c] == '[' && c + 2 < totalCols && canvas[r][c+2] == ']') {
                int rowInGroup = r % 2;
                int floor = r / 2 + 1;
                int colIdx = (c - 2) / 4;
                if (colIdx >= 0 && colIdx < 7 && rowInGroup == 1) {
                    auto it = nodeTypes.find({floor, colIdx});
                    if (it != nodeTypes.end()) {
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
            out += canvas[r][c];
            ++c;
        }

        size_t lastNonSpace = out.find_last_not_of(' ');
        if (lastNonSpace != std::string::npos) {
            std::cout << out.substr(0, lastNonSpace + 1) << "\n";
        } else {
            std::cout << "\n";
        }
    }
}
