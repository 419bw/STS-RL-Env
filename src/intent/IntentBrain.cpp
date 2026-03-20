#include "IntentBrain.h"
#include "src/gamestate/GameState.h"

void IntentBrain::initializeStats(int ascensionLevel) {}

void IntentBrain::reset() {
    moveIdHistory.clear();
}

bool IntentBrain::lastMove(int moveId) const {
    if (moveIdHistory.empty()) return false;
    return moveIdHistory.back() == moveId;
}

bool IntentBrain::lastTwoMoves(int moveId) const {
    if (moveIdHistory.size() < 2) return false;
    return moveIdHistory[moveIdHistory.size() - 1] == moveId &&
           moveIdHistory[moveIdHistory.size() - 2] == moveId;
}

void IntentBrain::recordMoveId(int moveId) {
    moveIdHistory.push_back(moveId);
}