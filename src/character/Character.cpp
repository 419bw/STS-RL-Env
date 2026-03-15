#include "Character.h"
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
