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
    static constexpr int MAX_AMOUNT = 999;
    static constexpr int MIN_AMOUNT = -999;

    virtual void stackPower(int stackAmount) {
        if (this->amount == -1) {
            return;
        }
        int newAmount = this->amount + stackAmount;
        if (newAmount > MAX_AMOUNT) {
            newAmount = MAX_AMOUNT;
        } else if (newAmount < MIN_AMOUNT) {
            newAmount = MIN_AMOUNT;
        }
        this->amount = newAmount;
    }

    // 状态挂载时触发，用于注册事件
    virtual void onApply(GameState& state) {}
    
    // 状态移除时触发，用于清理（如取消事件订阅）
    virtual void onRemove(GameState& state) {}
    
    // ==========================================
    // 四阶段伤害计算钩子 (Pipeline Hooks)
    // 
    // 阶段1: atDamageGive - 攻击者基础修饰 (加法为主，如力量)
    // 阶段2: atDamageReceive - 防御者基础修饰 (乘法为主，如易伤)
    // 阶段3: atDamageFinalGive - 攻击者最终修饰 (极限乘法，如笔尖)
    // 阶段4: atDamageFinalReceive - 防御者最终修饰 (截断，如无实体)
    // ==========================================
    
    // 阶段1: 攻击者造成伤害时 (力量、虚弱)
    virtual float atDamageGive(float damage, DamageType type) {
        return damage;
    }
    
    // 阶段2: 防御者受到伤害时 (易伤)
    // source: 攻击者，用于易伤倍率计算时让攻击者遗物参与
    virtual float atDamageReceive(float damage, DamageType type, Character* source = nullptr) {
        return damage;
    }
    
    // 阶段3: 攻击者最终伤害修饰 (笔尖、愤怒姿态)
    virtual float atDamageFinalGive(float damage, DamageType type) {
        return damage;
    }
    
    // 阶段4: 防御者最终伤害修饰 (无实体、鸟居)
    virtual float atDamageFinalReceive(float damage, DamageType type) {
        return damage;
    }
    
    // ==========================================
    // 格挡计算钩子 (只计算自己)
    // ==========================================
    
    // 阶段1: 获得格挡时 (敏捷)
    virtual float atBlockGive(float block) {
        return block;
    }
    
    // 阶段2: 最终格挡修饰
    virtual float atBlockFinalGive(float block) {
        return block;
    }
    
    // ==========================================
    // 掉血计算钩子 (用于 HP_LOSS 类型)
    // ==========================================
    
    // 修改掉血量（如无实体状态限制掉血为 1）
    virtual int modifyHpLoss(int amount) const {
        return amount;
    }

    // ==========================================
    // 视野拦截查询
    // ==========================================

    // 能否看到敌人意图（某些致盲状态可以遮挡）
    virtual bool canSeeEnemyIntents() const { return true; }

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
