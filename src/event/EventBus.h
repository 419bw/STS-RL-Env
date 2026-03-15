#pragma once

#include <vector>
#include <functional>
#include "src/core/Types.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 事件总线 (Event Bus)
// 用于解耦遗物、Buff与核心计算逻辑
// ==========================================

class EventBus {
private:
    // 监听器列表：事件类型 -> 回调函数列表
    std::vector<std::function<void(GameState&, void*)>> listeners[14]; 

public:
    // 订阅事件
    void subscribe(EventType type, std::function<void(GameState&, void*)> callback) {
        listeners[static_cast<int>(type)].push_back(callback);
    }

    // 发布事件
    void publish(EventType type, GameState& state, void* context = nullptr) {
        for (auto& cb : listeners[static_cast<int>(type)]) {
            cb(state, context);
        }
    }
};
