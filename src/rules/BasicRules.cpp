#include "BasicRules.h"
#include "src/gamestate/GameState.h"
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

void BasicRules::registerRules(GameState& state) {

    // 规则 1：战斗开始时 - 洗牌 + 刷新怪物意图
    state.eventBus.subscribe(EventType::PHASE_BATTLE_START,
        [](GameState& gs, void*) -> bool {
            // 重置所有怪物的 brain 状态（通过 Action 队列）
            for (auto& monster : gs.monsters) {
                if (!monster->isDead()) {
                    gs.addAction(std::make_unique<ResetBrainAction>(monster.get()));
                }
            }

            STS_LOG(gs, "    [BasicRules] 战斗开始 -> 洗牌 + 刷新怪物意图\n");
            gs.addAction(std::make_unique<ShuffleDiscardIntoDrawAction>());
            gs.addAction(std::make_unique<RollAllMonsterIntentsAction>());
            return true;
        });

    // 规则 2：玩家回合开始时 - 抽 5 张牌（原版行为）
    state.eventBus.subscribe(EventType::PHASE_PLAYER_TURN_START,
        [](GameState& gs, void*) -> bool {
            STS_LOG(gs, "    [BasicRules] 玩家回合开始 -> 抽 5 张牌\n");
            gs.addAction(std::make_unique<DrawCardsAction>(5));
            return true;
        });

    // 规则 3：玩家回合结束时 - 弃置所有手牌
    state.eventBus.subscribe(EventType::PHASE_PLAYER_TURN_END,
        [](GameState& gs, void*) -> bool {
            STS_LOG(gs, "    [BasicRules] 玩家回合结束 -> 弃置所有手牌\n");
            gs.addAction(std::make_unique<DiscardHandAction>());
            return true;
        });

    // 规则 4：怪物回合结束时 - 刷新怪物意图（为下回合准备）
    state.eventBus.subscribe(EventType::PHASE_MONSTER_TURN_END,
        [](GameState& gs, void*) -> bool {
            STS_LOG(gs, "    [BasicRules] 怪物回合结束 -> 刷新怪物意图\n");
            gs.addAction(std::make_unique<RollAllMonsterIntentsAction>());
            return true;
        });

    // 规则 5：怪物回合 - 执行怪物意图（推入 MonsterTakeTurnAction）
    state.eventBus.subscribe(EventType::PHASE_MONSTER_TURN,
        [](GameState& gs, void*) -> bool {
            for (auto& monster : gs.monsters) {
                if (!monster->isDead()) {
                    STS_LOG(gs, "    [BasicRules] " << monster->name << " 执行回合\n");
                    gs.addAction(std::make_unique<MonsterTakeTurnAction>(monster));
                }
            }
            return true;
        });
}
