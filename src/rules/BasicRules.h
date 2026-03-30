#pragma once

#include "src/engine/GameEngine.h"

// ==========================================
// BasicRules - 基础游戏规则
//
// 注册所有基础游戏规则到 EventBus
// 这些规则在战斗开始时注册，控制战斗流程
//
// 规则列表：
// - 战斗开始：洗牌 + 刷新怪物意图
// - 玩家回合开始：抽 5 张牌
// - 玩家回合结束：弃置所有手牌
// - 怪物回合结束：刷新怪物意图
// - 怪物回合：执行怪物意图
//
// 铁律：所有操作必须通过 Action 队列执行
// ==========================================

class BasicRules {
public:
    // 注册所有基础规则到 GameEngine 的 eventBus
    static void registerRules(GameEngine& engine);
};
