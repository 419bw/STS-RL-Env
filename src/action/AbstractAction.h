#pragma once

#include <memory>
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 动作系统 (Action System) - 极其重要！
// 解决深层嵌套逻辑，保证结算顺序严格一致
// ==========================================

class AbstractAction {
public:
    virtual ~AbstractAction() = default;
    // update 返回 true 表示该动作已完成，可以从队列中移除
    virtual bool update(GameState& state) = 0; 
};
