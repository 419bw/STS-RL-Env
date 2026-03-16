#pragma once

#include <string>
#include <memory>
#include "src/core/ForwardDeclarations.h"

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
    Character* owner = nullptr;  // 必须知道自己戴在谁身上

    AbstractRelic(std::string n) : name(n) {}
    virtual ~AbstractRelic() = default;

    // ==========================================
    // 1. 生命周期与事件订阅 (EventBus 路线)
    // ==========================================
    
    // 在装备遗物时调用
    // 子类重写时：
    // 1. 调用基类 onEquip（把自己塞进 owner 的背包）
    // 2. 订阅 EventBus 事件
    virtual void onEquip(GameState& state, Character* target);

    // ==========================================
    // 2. 高频数值拦截器 (Query Pipeline 路线)
    // 默认实现为"什么都不做"，直接返回原值
    // ==========================================
    
    // 拦截掉血（如钨钢棍减少 1 点掉血）
    virtual int modifyHpLoss(int amount) const { return amount; }
    
    // 拦截伤害（如某些遗物增加伤害）
    virtual float modifyDamage(float damage) const { return damage; }

    // ==========================================
    // 3. 查询表单处理 (Zero-Overhead Query)
    // 遗物看表、填表（重载）
    // ==========================================
    
    // 重载 1：处理易伤倍率查询（如纸蛙、奇数蘑菇）
    virtual void onQuery(VulnerableMultiplierQuery& query) {}
    
    // 重载 2：处理虚弱倍率查询
    virtual void onQuery(WeakMultiplierQuery& query) {}
};
