#pragma once

#include <string>
#include <vector>
#include <memory>
#include "src/core/ForwardDeclarations.h"
#include "src/power/AbstractPower.h"

// ==========================================
// 实体基类 (Character) - 玩家和怪物的通用属性
// 
// 设计原则：Character 是纯数据 + 纯计算 + 数据总线
// - 不输出日志（由 Action 负责）
// - 不发布事件（由 Action 负责）
// - 只负责数值计算和状态变更
// - 所有遗物/被动带来的乘区修正固化为面板属性
// 
// 接口分层：
// - calculateXxx(): 纯计算，const 方法，不修改状态（可用于预测和 UI）
// - reduceXxx() / addXxx(): 执行操作，修改状态
// 
// 数据驱动原则：
// - 实体同时扮演"攻击者"和"受击者"角色
// - 遗物只修改面板属性，不干涉结算流程
// - Power 是无状态计算器，只读取面板属性
// ==========================================

class Character : public std::enable_shared_from_this<Character> {
public:
    // ==========================================
    // 基础属性
    // ==========================================
    std::string name;
    int current_hp;
    int max_hp;
    int block;
    std::vector<std::shared_ptr<AbstractPower>> powers;

    // ==========================================
    // 乘区修饰属性（数据驱动的核心）
    // 遗物通过修改这些属性来影响战斗结算
    // ==========================================
    
    // --- 易伤相关 ---
    // 作为攻击者时：对带有易伤的敌人造成的伤害倍率（默认 1.5）
    float vulnerableDamageDealtMultiplier = 1.5f;
    // 作为受击者时：自身处于易伤状态承受的伤害倍率（默认 1.5）
    float vulnerableDamageReceivedMultiplier = 1.5f;
    
    // --- 虚弱相关 ---
    // 作为攻击者时：自身处于虚弱状态造成的伤害倍率（默认 0.75）
    float weakDamageDealtMultiplier = 0.75f;
    // 作为受击者时：对带有虚弱的攻击者承受的伤害倍率（默认 0.75）
    float weakDamageReceivedMultiplier = 0.75f;
    
    // --- 力量相关 ---
    // 每点力量增加的伤害（默认 1）
    int strengthDamageBonus = 1;
    
    // --- 敏捷相关 ---
    // 每点敏捷增加的格挡（默认 1）
    int dexterityBlockBonus = 1;

    Character(std::string n, int hp) 
        : name(n), current_hp(hp), max_hp(hp), block(0) {}
    virtual ~Character() = default;

    // ==========================================
    // 纯计算接口 (const 方法，不修改状态)
    // 可用于 AI 预测、UI 显示等场景
    // ==========================================
    
    // 计算最终伤害值（考虑易伤等状态修饰）
    // source: 伤害来源（攻击者），用于跨实体状态结算
    int calculateFinalDamage(int base_damage, Character* source = nullptr) const;
    
    // 计算最终获得的格挡值（考虑敏捷等状态修饰）
    int calculateFinalBlock(int base_block) const;

    // ==========================================
    // 执行接口 (修改状态)
    // ==========================================
    
    // 扣除格挡和血量，返回实际损失的 HP
    int reduceHealthAndBlock(int damage);

    // 增加格挡值，返回实际获得的格挡
    int addBlockFinal(int amount);
    
    // 判断是否死亡
    bool isDead() const { return current_hp <= 0; }
};

// ==========================================
// 玩家类 (Player)
// ==========================================
class Player : public Character {
public:
    int energy; // 当前费用
    Player(std::string n, int hp) : Character(n, hp), energy(3) {}
};

// ==========================================
// 怪物类 (Monster)
// ==========================================
class Monster : public Character {
public:
    bool deathReported;  // 是否已播报死亡（避免重复播报）
    
    Monster(std::string n, int hp) : Character(n, hp), deathReported(false) {}
};
