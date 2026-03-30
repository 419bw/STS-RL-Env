#include "src/engine/GameEngine.h"
#include "src/action/ActionManager.h"

// ==========================================
// GameEngine 构造函数
// ==========================================
GameEngine::GameEngine() = default;

GameEngine::~GameEngine() = default;

// ==========================================
// startNewRun - 开始新游戏
//
// 初始化持久层，创建玩家初始状态
// ==========================================
void GameEngine::startNewRun(unsigned int seed) {
    runState = std::make_shared<RunState>(seed);
}

// ==========================================
// startCombat - 进入战斗
//
// 1. 从 runState 克隆玩家数据到 combatState
// 2. 初始化战斗 RNG
// 3. 洗牌并抽初始手牌
// 4. 注册遗物效果到 eventBus
// ==========================================
void GameEngine::startCombat(std::shared_ptr<Monster> monster) {
    combatState = CombatState::createFromRun(*runState, monster, runState->rng.combatRng());
}

// ==========================================
// endCombat - 结束战斗
//
// 1. 清理 combatState 临时状态
// 2. 将战斗结果写回 runState（如有奖励）
// 3. 销毁 combatState
// ==========================================
void GameEngine::endCombat() {
    if (combatState) {
        combatState->cleanup();
        combatState.reset();
    }
    actionManager.actionQueue.clear();
    actionManager.currentAction.reset();
}

// ==========================================
// tick - 主循环推进
//
// 由外部主循环调用，推动游戏时间流逝
// 委托给 CombatFlow 处理战斗流程
// ==========================================
void GameEngine::tick() {
    if (combatState) {
        // 战斗流程由 CombatFlow 驱动
    }
}
