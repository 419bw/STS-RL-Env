#include "src/action/ActionManager.h"
#include "src/action/AbstractAction.h"
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"

// ==========================================
// addAction - O(1) 入队到队尾
// ==========================================
void ActionManager::addAction(std::unique_ptr<AbstractAction> action) {
    actionQueue.push_back(std::move(action));
}

// ==========================================
// addActionToFront - O(1) 入队到队头
// 用于优先执行紧急动作
// ==========================================
void ActionManager::addActionToFront(std::unique_ptr<AbstractAction> action) {
    actionQueue.push_front(std::move(action));
}

// ==========================================
// isQueueEmpty - 检查队列是否为空
// ==========================================
bool ActionManager::isQueueEmpty() const {
    return actionQueue.empty();
}

// ==========================================
// executeUntilBlocked - 执行动作直到被阻塞
//
// 执行流程：
// 1. 检查 combatState 是否存在
// 2. 重置 loopCount
// 3. 在 PLAYING_CARD 阶段循环执行：
//    - 若 currentAction 为空，从队列取下一个
//    - 调用 currentAction->update(engine)
//    - 若返回 true（完成），清除 currentAction
//    - 若返回 false（阻塞），退出循环
// 4. 防死锁看门狗：loopCount > MAX_LOOPS 时强制退出
//
// 注意：
// - 只在 PLAYING_CARD 阶段执行
// - 其他阶段由 CombatFlow 状态机驱动
// ==========================================
void ActionManager::executeUntilBlocked(GameEngine& engine, CombatFlow& flow) {
    //if (!engine.combatState) return;

    loopCount = 0;

    while (engine.combatState->currentPhase == StatePhase::PLAYING_CARD) {
        if (++loopCount > MAX_LOOPS) {
            break;
        }

        if (!currentAction) {
            if (actionQueue.empty()) {
                break;
            }
            currentAction = std::move(actionQueue.front());
            actionQueue.pop_front();
        }

        bool isDone = currentAction->update(engine);

        if (isDone) {
            currentAction.reset();
            if (engine.combatState) {
                flow.sbaGlobalCheck(engine);
                flow.checkBattleEndCondition(engine);
            }
        } else {
            break;
        }
    }
}
