#include "Character.h"
#include "src/relic/AbstractRelic.h"
#include "src/core/Queries.h"
#include "src/utils/Logger.h"

// ==========================================
// Character 实现
// 
// 设计原则：纯数据 + 纯计算 + 数据总线
// - 不输出日志（由 Action 负责）
// - 不发布事件（由 Action 负责）
// - 所有遗物/被动带来的乘区修正固化为面板属性
// ==========================================

// ==========================================
// 纯计算接口 (const 方法，不修改状态)
// 可用于 AI 预测、UI 显示等场景
// ==========================================

int Character::calculateFinalDamage(int base_damage, Character* source) const {
    float final_dmg = static_cast<float>(base_damage);
    
    // 遍历所有 Power，传递 source 进行跨实体状态结算
    for (const auto& power : powers) {
        final_dmg = power->modifyDamageTaken(final_dmg, source);
    }
    
    return static_cast<int>(final_dmg);
}

int Character::calculateFinalBlock(int base_block) const {
    float final_block = static_cast<float>(base_block);
    for (const auto& power : powers) {
        final_block = power->modifyBlockGained(final_block);
    }
    return static_cast<int>(final_block);
}

int Character::calculateFinalHpLoss(int base_amount) const {
    int final_amount = base_amount;
    
    // 1. 状态拦截（比如【无实体】会在这里把 final_amount 强行改成 1）
    for (const auto& power : powers) {
        final_amount = power->modifyHpLoss(final_amount);
    }
    
    // 2. 遗物拦截（比如玩家的【钨钢棍】会在这里把 final_amount 减 1）
    for (const auto& relic : relics) {
        final_amount = relic->modifyHpLoss(final_amount);
    }
    
    // 兜底保护：掉血不可能变成回血
    return std::max(0, final_amount);
}

// ==========================================
// 执行接口 (修改状态)
// ==========================================

int Character::reduceHealthAndBlock(int damage) {
    if (block >= damage) {
        block -= damage;
        return 0;  // 格挡完全吸收，没有损失 HP
    } else {
        int hp_lost = damage - block;
        block = 0;
        current_hp -= hp_lost;
        if (current_hp < 0) current_hp = 0;
        return hp_lost;  // 返回实际损失的 HP
    }
}

int Character::addBlockFinal(int amount) {
    block += amount;
    return amount;
}

// ==========================================
// 查询表单处理接口（重载）
// 让身上的遗物去填表
// ==========================================

void Character::processQuery(VulnerableMultiplierQuery& query) {
    for (auto& relic : relics) {
        relic->onQuery(query);
    }
}

void Character::processQuery(WeakMultiplierQuery& query) {
    for (auto& relic : relics) {
        relic->onQuery(query);
    }
}
