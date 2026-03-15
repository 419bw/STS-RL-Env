#include "BasicRules.h"
#include "src/gamestate/GameState.h"
#include "src/action/Actions.h"
#include <iostream>

// ==========================================
// 基础规则注册
// 
// 这些规则通过监听宏观阶段事件，向动作队列推入具体动作
// 实现了控制反转：CombatFlow 不知道 Action，但 BasicRules 知道
// ==========================================

void BasicRules::registerRules(GameState& state) {
    
    // 规则 1：战斗开始时 - 洗牌并抽 5 张初始手牌
    state.eventBus.subscribe(EventType::PHASE_BATTLE_START, [](GameState& gs, void*) {
        std::cout << "    [BasicRules] 战斗开始 -> 洗牌\n";
        gs.actionQueue.push(std::make_unique<DummyAction>("洗牌入抽牌堆"));
    });

    // 规则 2：回合开始时 - 抽 1 张牌
    state.eventBus.subscribe(EventType::PHASE_TURN_START, [](GameState& gs, void*) {
        std::cout << "    [BasicRules] 回合开始 -> 抽 5 张牌\n";
        gs.actionQueue.push(std::make_unique<DummyAction>("抽取 5 张牌"));
    });

    // 规则 3：回合结束时 - 弃置手牌
    state.eventBus.subscribe(EventType::PHASE_TURN_END, [](GameState& gs, void*) {
        std::cout << "    [BasicRules] 回合结束 -> 弃置所有手牌\n";
        gs.actionQueue.push(std::make_unique<DummyAction>("弃置所有手牌"));
    });

    // 规则 4：怪物回合 - 执行怪物意图
    state.eventBus.subscribe(EventType::PHASE_MONSTER_TURN, [](GameState& gs, void*) {
        for (auto& monster : gs.monsters) {
            if (!monster->isDead()) {
                std::cout << "    [BasicRules] " << monster->name << " 执行意图\n";
                gs.actionQueue.push(std::make_unique<DummyAction>(
                    monster->name + " 执行意图"));
            }
        }
    });
}
