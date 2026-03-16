#pragma once

#include <string>
#include <memory>
#include "src/core/Types.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 状态效果基类 (Power System)
// 
// 设计原则：Power 是纯粹的无状态计算器
// - 只读取参与双方的面板属性
// - 不关心属性是怎么来的
// - 不跨层级访问全局游戏状态
// - 不硬编码特定遗物名称
// ==========================================

class AbstractPower : public std::enable_shared_from_this<AbstractPower> {
public:
    std::string name;
    PowerType type;

    AbstractPower(std::string n, int a, PowerType t) 
        : name(n), amount(a), type(t), justApplied(false) {}
    virtual ~AbstractPower() = default;

    // ==========================================
    // 访问接口（封装内部成员）
    // ==========================================
    
    // Owner 访问
    std::shared_ptr<Character> getOwner() const { return owner; }
    
    // Amount 访问
    int getAmount() const { return amount; }
    void setAmount(int value) { amount = value; }
    
    // JustApplied 访问
    bool isJustApplied() const { return justApplied; }
    void setJustApplied(bool value) { justApplied = value; }

    // ==========================================
    // 状态叠加接口
    // 
    // 当对同一目标施加同名状态时调用
    // amount = -1 表示不可叠加（如能力牌效果）
    // 默认实现：层数相加
    // ==========================================
    virtual void stackPower(int stackAmount) {
        if (this->amount == -1) {
            // 不可叠加状态，什么都不做
            return;
        }
        this->amount += stackAmount;
    }

    // 状态挂载时触发，用于注册事件
    virtual void onApply(GameState& state) {}
    
    // 状态移除时触发，用于清理（如取消事件订阅）
    virtual void onRemove(GameState& state) {}
    
    // ==========================================
    // 数值修饰接口（数据驱动）
    // source: 伤害来源（攻击者），用于跨实体状态结算
    // ==========================================
    
    // 修改承受的伤害（如易伤）
    virtual float modifyDamageTaken(float damage, Character* source = nullptr) { 
        return damage; 
    }
    
    // 修改造成的伤害（如虚弱）
    virtual float modifyDamageDealt(float damage, Character* target = nullptr) { 
        return damage; 
    }
    
    // 修改获得的格挡（如敏捷）
    virtual float modifyBlockGained(float block) { 
        return block; 
    }
    
    // 修改掉血量（如无实体状态限制掉血为 1）
    virtual int modifyHpLoss(int amount) const { 
        return amount; 
    }

protected:
    // ==========================================
    // 内部状态（私有，通过接口访问）
    // ==========================================
    
    // 层数 (例如 3层中毒)
    int amount;
    
    // 微观首回合保护罩：防止刚挂上的状态被轮次结束判定"误杀"
    // true = 刚挂上，本轮次不掉层
    // false = 正常状态，轮次结束时正常掉层
    bool justApplied;
    
    // 拥有者引用
    std::shared_ptr<Character> owner;
    
    // 允许 Character 访问 owner 进行设置
    friend class Character;
};
