#pragma once

#include <string>
#include <vector>
#include <memory>
#include "src/core/ForwardDeclarations.h"
#include "src/power/AbstractPower.h"

// ==========================================
// 实体基类 (Character) - 玩家和怪物的通用属性
// 
// 设计原则：Character 是纯数据 + 纯计算
// - 不输出日志（由 Action 负责）
// - 不发布事件（由 Action 负责）
// - 只负责数值计算和状态变更
// 
// 接口分层：
// - calculateXxx(): 纯计算，const 方法，不修改状态（可用于预测和 UI）
// - reduceXxx() / addXxx(): 执行操作，修改状态
// ==========================================

class Character : public std::enable_shared_from_this<Character> {
public:
    std::string name;
    int current_hp;
    int max_hp;
    int block; // 格挡值
    std::vector<std::shared_ptr<AbstractPower>> powers; // 角色身上的状态列表

    Character(std::string n, int hp) 
        : name(n), current_hp(hp), max_hp(hp), block(0) {}
    virtual ~Character() = default;

    // ==========================================
    // 纯计算接口 (const 方法，不修改状态)
    // 可用于 AI 预测、UI 显示等场景
    // ==========================================
    
    // 计算最终伤害值（考虑易伤等状态修饰）
    int calculateFinalDamage(int base_damage) const;
    
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
