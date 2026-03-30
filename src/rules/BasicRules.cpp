#include "BasicRules.h"
#include "src/engine/GameEngine.h"
#include "src/action/Actions.h"
#include "src/utils/Logger.h"
#include <iostream>

// ==========================================
// 基础规则注册
//
// 回调返回值：
// - true：继续监听（规则是永久的）
// - false：移除监听者
//
// 原版规则：
// - 战斗开始：洗牌，不抽牌
// - 玩家回合开始：抽 5 张牌
// - 玩家回合结束：弃置所有手牌
//
// 铁律：所有操作必须通过 Action 队列执行
// ==========================================

void BasicRules::registerRules(GameEngine& engine) {

    // 规则 1：战斗开始时 - 洗牌 + 刷新怪物意图
    engine.eventBus.subscribe(EventType::PHASE_BATTLE_START,
        [](GameEngine& eng, void*) -> bool {
            if (!eng.combatState) return true;
            //重置怪物意图（战斗开始时，所有怪物意图重置为默认值）
            eng.actionManager.addAction(std::make_unique<ResetAllBrainsAction>());
            STS_LOG(*eng.combatState, "    [BasicRules] 战斗开始 -> 洗牌 + 刷新怪物意图\n");
            eng.actionManager.addAction(std::make_unique<ShuffleDiscardIntoDrawAction>());
            // 刷新所有怪物意图（为下回合准备）
            eng.actionManager.addAction(std::make_unique<RollAllMonsterIntentsAction>());
            return true;
        });

    // 规则 2：玩家回合开始时 - 抽 5 张牌（原版行为）
    engine.eventBus.subscribe(EventType::PHASE_PLAYER_TURN_START,
        [](GameEngine& eng, void*) -> bool {
            if (!eng.combatState) return true;
            STS_LOG(*eng.combatState, "    [BasicRules] 玩家回合开始 -> 抽 5 张牌\n");
            eng.actionManager.addAction(std::make_unique<DrawCardsAction>(5));
            return true;
        });

    // 规则 3：玩家回合结束时 - 弃置所有手牌
    engine.eventBus.subscribe(EventType::PHASE_PLAYER_TURN_END,
        [](GameEngine& eng, void*) -> bool {
            if (!eng.combatState) return true;
            STS_LOG(*eng.combatState, "    [BasicRules] 玩家回合结束 -> 弃置所有手牌\n");
            eng.actionManager.addAction(std::make_unique<DiscardHandAction>());
            return true;
        });

    // 规则 4：怪物回合结束时 - 刷新怪物意图（为下回合准备）
    engine.eventBus.subscribe(EventType::PHASE_MONSTER_TURN_END,
        [](GameEngine& eng, void*) -> bool {
            if (!eng.combatState) return true;
            STS_LOG(*eng.combatState, "    [BasicRules] 怪物回合结束 -> 刷新怪物意图\n");
            eng.actionManager.addAction(std::make_unique<RollAllMonsterIntentsAction>());
            return true;
        });

    // 规则 5：怪物回合 - 执行怪物意图（推入 MonsterTakeTurnAction）
    engine.eventBus.subscribe(EventType::PHASE_MONSTER_TURN,
        [](GameEngine& eng, void*) -> bool {
            if (!eng.combatState) return true;
            for (auto& monster : eng.combatState->monsters) {
                if (!monster->isDead()) {
                    STS_LOG(*eng.combatState, "    [BasicRules] " << monster->name << " 执行回合\n");
                    eng.actionManager.addAction(std::make_unique<MonsterTakeTurnAction>(monster));
                }
            }
            return true;
        });
}
