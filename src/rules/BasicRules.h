#pragma once

#include "src/core/ForwardDeclarations.h"

// ==========================================
// BasicRules - 原版游戏基础规则
// 
// 铁律三：基础游戏规则即"插件" (Plugin-based Rules)
// - 原版游戏的"回合开始抽 5 张牌"、"回合结束弃牌"
// - 必须和"遗物"、"状态效果"一样，作为独立的监听者挂载到 EventBus 上
// ==========================================

class BasicRules {
public:
    // 注册所有基础规则到 EventBus
    static void registerRules(GameState& state);
};
