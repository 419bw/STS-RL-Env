#pragma once

#include "src/core/ForwardDeclarations.h"

// ==========================================
// 查询表单系统
// 
// 设计原则：
// - 纯粹的数据结构，没有任何逻辑
// - 极其轻量，在栈内存中创建
// - 用于遗物和状态的数值修饰
// - 零开销抽象
// ==========================================

// ==========================================
// 易伤倍率查询表单
// 
// 用途：计算易伤状态下的伤害倍率
// 流程：
// 1. VulnerablePower 创建表单
// 2. 递给 source（攻击者）让遗物填表（如纸蛙）
// 3. 递给 target（受击者）让遗物填表（如蘑菇）
// 4. 最终读取 multiplier 结算
// ==========================================
struct VulnerableMultiplierQuery {
    Character* source;       // 谁打的
    Character* target;       // 打谁
    float multiplier = 1.5f; // 基础底座（默认易伤倍率）
};

// ==========================================
// 虚弱倍率查询表单
// ==========================================
struct WeakMultiplierQuery {
    Character* source;       // 谁打的
    Character* target;       // 打谁
    float multiplier = 0.75f; // 基础底座（默认虚弱倍率）
};
