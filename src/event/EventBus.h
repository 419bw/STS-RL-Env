#pragma once

#include <vector>
#include <functional>
#include <algorithm>
#include "src/core/Types.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 事件总线 (Event Bus) - 自清理版本
// 
// 特性：
// - 回调返回 bool：true 表示继续监听，false 表示移除
// - Erase-Remove Idiom：自动清理返回 false 的僵尸监听者
// - 配合 weak_ptr 使用，实现安全的生命周期管理
// ==========================================

class EventBus {
private:
    // 监听器列表：事件类型 -> 回调函数列表
    // 回调返回 true 表示继续监听，false 表示移除
    std::vector<std::function<bool(GameState&, void*)>> listeners[14]; 

public:
    // 订阅事件
    void subscribe(EventType type, std::function<bool(GameState&, void*)> callback) {
        listeners[static_cast<int>(type)].push_back(callback);
    }

    // 发布事件（自动清理僵尸监听者）
    void publish(EventType type, GameState& state, void* context = nullptr) {
        auto& callbackList = listeners[static_cast<int>(type)];
        
        // Erase-Remove Idiom：移除所有返回 false 的监听者
        callbackList.erase(
            std::remove_if(callbackList.begin(), callbackList.end(),
                [&state, context](std::function<bool(GameState&, void*)>& callback) {
                    return !callback(state, context);  // 返回 false 则移除
                }),
            callbackList.end()
        );
    }
};
