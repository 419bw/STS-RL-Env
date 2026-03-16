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
// 
// 查询表单系统：
// - 遗物通过查询表单参与数值计算
// - 零开销抽象，栈内存创建表单
// - Power 创建表单，递给双方遗物填表
// ==========================================

// 前向声明查询表单
struct VulnerableMultiplierQuery;
struct WeakMultiplierQuery;

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
    // 遗物背包
    // 用于高频数值查询的实体挂载
    // 遗物在 onEquip 时会把自己塞进这里
    // ==========================================
    std::vector<std::shared_ptr<class AbstractRelic>> relics;

    // ==========================================
    // 乘区修饰属性（简单属性，不需要查询表单）
    // ==========================================
    


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
    
    // 计算最终掉血值（考虑状态和遗物拦截）
    // 用于 LoseHpAction（中毒、献祭等）
    int calculateFinalHpLoss(int base_amount) const;

    // ==========================================
    // 查询表单处理接口（重载）
    // 让身上的遗物去填表
    // ==========================================
    
    // 重载 1：处理易伤表单
    void processQuery(VulnerableMultiplierQuery& query);
    
    // 重载 2：处理虚弱表单
    void processQuery(WeakMultiplierQuery& query);

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
