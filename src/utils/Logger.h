#pragma once

#include <iostream>

// ==========================================
// STS 极简日志系统
// 
// STS_LOG: 运行时日志（用于观战和调试）
// - 用法: STS_LOG(state, "玩家受到了 " << damage << " 点伤害\n");
// - 特性：运行时检查 enableLogging，支持多线程独立控制
// 
// ENGINE_TRACE: 编译期日志（用于引擎内部追踪）
// - 用法: ENGINE_TRACE("状态效果触发: " << name);
// - 特性：只在 DEBUG_MODE 下生效，编译期剥离，零性能损耗
// ==========================================

class GameState;

// ==========================================
// 运行时日志宏 (用于观战和调试)
// ==========================================
#define STS_LOG(state, msg) \
    do { \
        if ((state).enableLogging) { \
            std::cout << msg; \
        } \
    } while(0)

// ==========================================
// 编译期调试宏 (用于引擎内部追踪)
// 
// 在 DEBUG_MODE 下输出详细的状态效果信息
// 在 Release/AI 训练模式下完全剥离，零性能损耗
// ==========================================
#ifdef DEBUG_MODE
    #define ENGINE_TRACE(msg) \
        do { \
            std::cout << "[引擎追踪] " << msg << "\n"; \
        } while(0)
#else
    #define ENGINE_TRACE(msg) do {} while(0)
#endif
