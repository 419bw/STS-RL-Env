#include "src/state/RunState.h"
#include "src/character/Character.h"
#include "src/core/RandomManager.h"

// ==========================================
// RunState 构造函数
//
// 创建初始玩家状态
// 初始化 RNG
// ==========================================
RunState::RunState(unsigned int seed)
    : rng(seed) {
    player = std::make_shared<Player>("战士", 80);
}
