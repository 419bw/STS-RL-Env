#pragma once

#include <string>
#include <memory>
#include "src/core/ForwardDeclarations.h"
#include "src/core/Types.h"

// ==========================================
// 遗物基类 (AbstractRelic)
// 
// 设计原则：
// - 遗物只负责修改面板属性和数值拦截
// - 不干涉战斗结算的具体流程
// - 通过两条路线影响游戏：
//   1. EventBus 路线：订阅事件，触发一次性效果
//   2. Query Pipeline 路线：高频数值拦截（零开销抽象）
// ==========================================

// 前向声明查询表单
struct VulnerableMultiplierQuery;
struct WeakMultiplierQuery;

class AbstractRelic : public std::enable_shared_from_this<AbstractRelic> {
public:
    std::string name;

    AbstractRelic(std::string n) : name(n) {}
    virtual ~AbstractRelic() = default;

    // ==========================================
    // Owner 访问接口（封装 owner 指针）
    // ==========================================
    Character* getOwner() const { return owner; }

    // ==========================================
    // 1. 生命周期与事件订阅 (EventBus 路线)
    // ==========================================
    
    // 在装备遗物时调用
    // 子类重写时：
    // 1. 调用基类 onEquip（把自己塞进 owner 的背包）
    // 2. 订阅 EventBus 事件
    virtual void onEquip(GameState& state, Character* target);
    
    // 在移除遗物时调用
    // 子类重写时：取消 EventBus 订阅
    virtual void onRemove(GameState& state) {}

    // ==========================================
    // 2. 高频数值拦截器 (Query Pipeline 路线)
    // 默认实现为"什么都不做"，直接返回原值
    // ==========================================
    
    // ==========================================
    // 四阶段伤害计算钩子 (Pipeline Hooks)
    // ==========================================
    
    // 阶段1: 攻击者造成伤害时 (纸蛙)
    virtual float atDamageGive(float damage, DamageType type) {
        return damage;
    }
    
    // 阶段2: 防御者受到伤害时 (奇数蘑菇)
    virtual float atDamageReceive(float damage, DamageType type) {
        return damage;
    }
    
    // 阶段3: 攻击者最终伤害修饰 (笔尖)
    virtual float atDamageFinalGive(float damage, DamageType type) {
        return damage;
    }
    
    // 阶段4: 防御者最终伤害修饰 (无实体)
    virtual float atDamageFinalReceive(float damage, DamageType type) {
        return damage;
    }
    
    // ==========================================
    // 实际扣血钩子 (在护甲之后)
    // 
    // 与 atDamageFinalReceive 的区别：
    // - atDamageFinalReceive: 修饰伤害值（护甲之前）
    // - onActualHpLoss: 修饰实际扣血值（护甲之后）
    // 
    // 例如：
    // - 无实体: atDamageFinalReceive 把伤害变成 1
    // - 鸟居: onActualHpLoss 把扣血变成 1（只在有实际扣血时）
    // - 钨合金棍: onActualHpLoss 把扣血减 1
    // ==========================================
    
    // 实际扣血时触发（护甲之后）
    // 返回实际扣血值
    virtual int onActualHpLoss(int hpLoss, DamageType type) {
        return hpLoss;
    }
    
    // ==========================================
    // 格挡计算钩子
    // ==========================================
    
    // 阶段1: 获得格挡时
    virtual float atBlockGive(float block) {
        return block;
    }
    
    // 阶段2: 最终格挡修饰
    virtual float atBlockFinalGive(float block) {
        return block;
    }
    
    // ==========================================
    // 掉血计算钩子 (HP_LOSS 类型)
    // ==========================================
    
    // 拦截掉血（如钨钢棍减少 1 点掉血）
    virtual int modifyHpLoss(int amount) const { return amount; }

    // ==========================================
    // 3. 查询表单处理 (Zero-Overhead Query)
    // 遗物看表、填表（重载）
    // ==========================================
    
    // 重载 1：处理易伤倍率查询（如纸蛙、奇数蘑菇）
    virtual void onQuery(VulnerableMultiplierQuery& query) {}
    
    // 重载 2：处理虚弱倍率查询
    virtual void onQuery(WeakMultiplierQuery& query) {}

    // ==========================================
    // 视野拦截钩子
    // ==========================================

    // 能否看到怪物意图（符文圆顶遮挡）
    virtual bool canSeeEnemyIntents() const { return true; }

protected:
    // ==========================================
    // 内部引用（私有，由 Character::addRelic 设置）
    // ==========================================
    Character* owner = nullptr;
    
    // 允许 Character 访问 owner 进行设置
    friend class Character;
};
