#pragma once

#include <memory>
#include "src/core/ForwardDeclarations.h"

// ==========================================
// AbstractAction - 动作基类
//
// ECS/DOD 架构核心组件：
// - 所有游戏动作的抽象基类
// - update() 返回 true 表示完成，false 表示阻塞
//
// 设计原则：
// - update(GameEngine&) 可访问完整上下文
// - 动作是纯数据+逻辑，无状态
// - 动作执行后可能被复用（如 DamageAction）
//
// 线程安全：
// - 设计用于单线程无头演算
// - 不支持并发执行
// ==========================================

class AbstractAction {
public:
    virtual ~AbstractAction() = default;

    // ==========================================
    // 执行动作
    // @engine: 游戏引擎引用，可访问 runState/combatState/eventBus
    // @return: true 表示动作完成，false 表示阻塞（需等待）
    // ==========================================
    virtual bool update(GameEngine& engine) = 0;
};
