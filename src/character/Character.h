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

    // ==========================================
    // Power 管理接口（封装 powers 数组）
    // 
    // 设计原则：高内聚低耦合
    // - 外部只能通过接口操作 Power
    // - 叠加逻辑封装在 Character 内部
    // - 单一职责：每个接口只负责一项功能
    // ==========================================
    
    // 添加状态（含叠加逻辑）
    // 返回值：true = 新添加，false = 叠加到已有状态
    bool addPower(std::shared_ptr<AbstractPower> power);
    
    // 移除指定状态
    void removePower(std::shared_ptr<AbstractPower> power);
    
    // 检查是否存在指定名称的状态
    bool hasPower(const std::string& powerName) const;
    
    // 获取指定名称的状态（返回 nullptr 表示不存在）
    std::shared_ptr<AbstractPower> getPower(const std::string& powerName) const;
    
    // 获取状态数量
    size_t getPowerCount() const { return powers.size(); }
    
    // 清空所有状态
    void clearPowers();
    
    // 遍历状态的只读访问（用于计算层遍历）
    // 使用回调函数模式，避免暴露内部容器
    template<typename Func>
    void forEachPower(Func&& func) const {
        for (const auto& power : powers) {
            func(power);
        }
    }

    // ==========================================
    // Relic 管理接口（封装 relics 数组）
    // 
    // 设计原则：与 Power 管理接口一致
    // - 外部只能通过接口操作 Relic
    // - 添加时自动触发 onEquip
    // - 移除时自动触发 onRemove
    // ==========================================
    
    // 添加遗物（自动触发 onEquip）
    void addRelic(std::shared_ptr<class AbstractRelic> relic, GameState& state);
    
    // 移除遗物（自动触发 onRemove）
    void removeRelic(std::shared_ptr<class AbstractRelic> relic, GameState& state);
    
    // 检查是否存在指定名称的遗物
    bool hasRelic(const std::string& relicName) const;
    
    // 获取指定名称的遗物（返回 nullptr 表示不存在）
    std::shared_ptr<class AbstractRelic> getRelic(const std::string& relicName) const;
    
    // 获取遗物数量
    size_t getRelicCount() const { return relics.size(); }
    
    // 清空所有遗物
    void clearRelics(GameState& state);
    
    // 遍历遗物的只读访问（用于计算层遍历）
    template<typename Func>
    void forEachRelic(Func&& func) const {
        for (const auto& relic : relics) {
            func(relic);
        }
    }

private:
    // ==========================================
    // 状态效果列表（私有，外部只能通过接口访问）
    // ==========================================
    std::vector<std::shared_ptr<AbstractPower>> powers;
    
    // ==========================================
    // 遗物背包（私有，外部只能通过接口访问）
    // ==========================================
    std::vector<std::shared_ptr<class AbstractRelic>> relics;
};

// ==========================================
// 玩家类 (Player)
// ==========================================
class Player : public Character {
public:
    Player(std::string n, int hp) : Character(n, hp), energy(3) {}
    
    // ==========================================
    // Energy 管理接口（封装 energy 成员）
    // ==========================================
    
    // 获取当前费用
    int getEnergy() const { return energy; }
    
    // 检查是否有足够费用
    bool hasEnoughEnergy(int cost) const { 
        return cost == -1 || energy >= cost;  // -1 表示 X 费牌
    }
    
    // 消耗费用（返回实际消耗量）
    int spendEnergy(int cost) {
        if (cost == -1) {
            // X 费牌：消耗所有费用
            int spent = energy;
            energy = 0;
            return spent;
        }
        int spent = std::min(energy, cost);
        energy -= spent;
        return spent;
    }
    
    // 重置费用（回合开始时调用）
    void resetEnergy(int maxEnergy = 3) {
        energy = maxEnergy;
    }
    
    // 增加费用（如某些遗物效果）
    void gainEnergy(int amount) {
        energy += amount;
    }

private:
    int energy;  // 当前费用
};

// ==========================================
// 怪物类 (Monster)
// ==========================================
class Monster : public Character {
public:
    bool deathReported;  // 是否已播报死亡（避免重复播报）
    
    Monster(std::string n, int hp) : Character(n, hp), deathReported(false) {}
};
