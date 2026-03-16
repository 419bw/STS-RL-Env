#include "Actions.h"
#include "src/power/AbstractPower.h"
#include "src/character/Character.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/utils/Logger.h"
#include <iostream>
#include <algorithm>

// ==========================================
// DummyAction 实现
// ==========================================
bool DummyAction::update(GameState& state) {
    STS_LOG(state, "    [动作队列执行] -> " << name << "\n");
    return true;
}

// ==========================================
// DamageAction 实现
// 
// 数据驱动原则：
// - 必须携带溯源信息（source: 伤害来源）
// - Action 负责将 source 传递给计算层
// - Character::calculateFinalDamage 纯计算（可用于预测）
// - Character::reduceHealthAndBlock 执行扣血
// - DamageAction 负责日志输出和事件发布
// ==========================================
DamageAction::DamageAction(std::shared_ptr<Character> src, std::shared_ptr<Character> tgt, int a) 
    : source(src), target(tgt), amount(a) {}

bool DamageAction::update(GameState& state) {
    if (!target->isDead()) {
        // 1. 调用目标的计算接口，传递 source 进行跨实体状态结算
        int final_damage = target->calculateFinalDamage(amount, source.get());
        
        // 2. 如果伤害被状态影响，输出调试信息
        if (final_damage != amount) {
            ENGINE_TRACE("伤害计算: " << amount << " -> " << final_damage 
                         << " (受状态效果影响)");
        }
        
        // 3. 执行最终的扣血
        target->reduceHealthAndBlock(final_damage);
        
        // 4. 输出日志
        STS_LOG(state, target->name << " 受到了 " << final_damage 
                  << " 点伤害，剩余血量: " << target->current_hp 
                  << ", 剩余格挡: " << target->block << "\n");
        
        // 5. 发布事件
        state.eventBus.publish(EventType::ON_DAMAGE_TAKEN, state, target.get());
        
        // 死亡判定由 SBA（全局巡视）负责，Action 只管执行
    }
    return true;
}

// ==========================================
// LoseHpAction 实现
// 
// 特性：
// - 无视护甲，直接扣除生命值
// - 通过计算管线（状态 + 遗物拦截）
// - 发布 ON_HP_LOST 事件
// - 用于中毒、献祭等效果
// ==========================================
LoseHpAction::LoseHpAction(std::shared_ptr<Character> t, int a) 
    : target(t), amount(a) {}

bool LoseHpAction::update(GameState& state) {
    if (!target->isDead()) {
        // 1. 通过计算管线获取最终掉血值
        int final_amount = target->calculateFinalHpLoss(amount);
        
        // 2. 如果掉血被拦截，输出调试信息
        if (final_amount != amount) {
            ENGINE_TRACE("掉血计算: " << amount << " -> " << final_amount 
                         << " (受状态/遗物拦截)");
        }
        
        // 3. 直接扣除生命值，无视护甲
        target->current_hp -= final_amount;
        if (target->current_hp < 0) target->current_hp = 0;
        
        // 4. 输出日志
        STS_LOG(state, target->name << " 失去了 " << final_amount 
                  << " 点生命值（无视护甲），剩余血量: " << target->current_hp << "\n");
        
        // 5. 发布事件
        state.eventBus.publish(EventType::ON_HP_LOST, state, target.get());
        
        // 死亡判定由 SBA（全局巡视）负责
    }
    return true;
}

// ==========================================
// GainBlockAction 实现
// 
// 设计原则：Action 负责日志和事件
// - Character::calculateFinalBlock 纯计算（可用于预测）
// - Character::addBlockFinal 执行获得格挡
// - GainBlockAction 负责日志输出和事件发布
// ==========================================
GainBlockAction::GainBlockAction(std::shared_ptr<Character> t, int a) 
    : target(t), amount(a) {}

bool GainBlockAction::update(GameState& state) {
    if (!target->isDead()) {
        // 1. 调用计算接口，获取最终格挡
        int final_block = target->calculateFinalBlock(amount);
        
        // 2. 如果格挡被状态影响，输出调试信息
        if (final_block != amount) {
            ENGINE_TRACE("格挡计算: " << amount << " -> " << final_block 
                         << " (受状态效果影响)");
        }
        
        // 3. 执行获得格挡
        target->addBlockFinal(final_block);
        
        // 4. 输出日志
        STS_LOG(state, target->name << " 获得了 " << final_block 
                  << " 点格挡，当前格挡: " << target->block << "\n");
        
        // 5. 发布事件（如果需要）
        // state.eventBus.publish(EventType::ON_BLOCK_GAINED, state, target.get());
    }
    return true;
}

// ==========================================
// ApplyPowerAction 实现
// 
// 保护罩逻辑：
// 1. 查阅黑板发现 !state.isPlayerTurn (玩家回合已结束)
// 2. 且释放者是怪物 (source != player)
// 3. 如果满足，设置 justApplied = true，防止本轮次掉层
// ==========================================
ApplyPowerAction::ApplyPowerAction(std::shared_ptr<Character> src, 
                                     std::shared_ptr<Character> tgt, 
                                     std::shared_ptr<AbstractPower> p) 
    : source(src), target(tgt), power(p) {}

bool ApplyPowerAction::update(GameState& state) {
    if (!target->isDead()) {
        bool isMonsterSource = (source != state.player);
        if (!state.isPlayerTurn && isMonsterSource) {
            power->justApplied = true;
            ENGINE_TRACE("保护罩激活: " << power->name << " 刚挂上，本轮次不掉层");
        } else {
            power->justApplied = false;
        }
        
        STS_LOG(state, "-> 给 " << target->name << " 施加了 " << power->amount 
                  << " 层 [" << power->name << "]\n");
        power->owner = target;
        target->powers.push_back(power);
        power->onApply(state);
    }
    return true;
}

// ==========================================
// ReducePowerAction 实现
// 
// 只负责减少层数，不负责移除
// 如果层数归零，推入 RemoveSpecificPowerAction 来清理
// ==========================================
ReducePowerAction::ReducePowerAction(std::shared_ptr<Character> t, 
                                     std::shared_ptr<AbstractPower> p, int a) 
    : target(t), power(p), reduceAmount(a) {}

bool ReducePowerAction::update(GameState& state) {
    if (power && power->amount > 0) {
        power->amount -= reduceAmount;
        STS_LOG(state, "-> " << target->name << " 的 [" << power->name 
                  << "] 减少了 " << reduceAmount << " 层，剩余 " 
                  << power->amount << " 层。\n");
        
        // 层数归零时，推入强制移除动作
        if (power->amount <= 0) {
            STS_LOG(state, "-> [" << power->name << "] 已完全消散。\n");
            state.addAction(std::make_unique<RemoveSpecificPowerAction>(target, power));
        }
    }
    return true;
}

// ==========================================
// RemoveSpecificPowerAction 实现
// 
// 无视层数，直接从目标身上移除指定的 Power
// 触发 onRemove 遗言，用于清理事件订阅等
// ==========================================
RemoveSpecificPowerAction::RemoveSpecificPowerAction(std::shared_ptr<Character> t, 
                                                      std::shared_ptr<AbstractPower> p) 
    : target(t), power(p) {}

bool RemoveSpecificPowerAction::update(GameState& state) {
    if (power) {
        STS_LOG(state, "-> 强制净化！[" << power->name << "] 被直接从 " 
                  << target->name << " 身上移除。\n");
        
        // 触发遗言（如果有）
        power->onRemove(state);
        
        // 无视层数，绝对抹杀
        auto& powers = target->powers;
        powers.erase(
            std::remove(powers.begin(), powers.end(), power),
            powers.end()
        );
        
        ENGINE_TRACE("Power 强制移除: " << power->name << " 已从 " << target->name << " 身上移除");
    }
    return true;
}
