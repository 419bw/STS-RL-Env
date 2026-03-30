#include "Actions.h"
#include "src/power/AbstractPower.h"
#include "src/character/Character.h"
#include "src/engine/GameEngine.h"
#include "src/event/EventBus.h"
#include "src/card/AbstractCard.h"
#include "src/system/DeckSystem.h"
#include "src/utils/Logger.h"
#include "src/intent/IntentBrain.h"
#include <iostream>
#include <algorithm>

// ==========================================
// DummyAction 实现
// ==========================================
bool DummyAction::update(GameEngine& engine) {
    STS_LOG(*engine.combatState, "    [动作队列执行] -> " << name << "\n");
    return true;
}

// ==========================================
// RollAllMonsterIntentsAction 实现
// ==========================================
bool RollAllMonsterIntentsAction::update(GameEngine& engine) {
    for (auto& monster : engine.combatState->monsters) {
        if (!monster->isDead()) {
            monster->rollIntent(engine);
        }
    }
    return true;
}

// ==========================================
// MonsterTakeTurnAction 实现
// ==========================================
bool MonsterTakeTurnAction::update(GameEngine& engine) {
    if (!monster->isDead()) {
        monster->takeTurn(engine);
    }
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
DamageAction::DamageAction(std::shared_ptr<Character> src, std::shared_ptr<Character> tgt, int a,
                           DamageType type)
    : source(src), target(tgt), amount(a), damageType(type) {}

bool DamageAction::update(GameEngine& engine) {
    if (!target->isDead()) {
        // 1. 算伤管线 (算出破甲前的最终伤害数值)
        int final_damage = target->calculateFinalDamage(amount, source.get(), damageType);
        
        // ==========================================
        // ★ 第一层广播：ON_ATTACKED (肢体接触)
        // 只要是物理攻击，不管有没有被格挡，无条件触发！(唤醒荆棘、火焰屏障)
        // ==========================================
        if (this->damageType == DamageType::ATTACK) {
            DamageContext ctx(target.get(), source.get(), this->damageType, final_damage);
            engine.eventBus.publish(EventType::ON_ATTACKED, engine, &ctx);
        }
        
        // 2. 破甲管线 (takeDamage 内部会调用 loseHp，并处理鸟居)
        Character::DamageResult receipt = target->takeDamage(final_damage, damageType);
        
        // 3. 输出日志
        if (receipt.damage_taken > 0 || receipt.hp_lost > 0) {
            STS_LOG(*engine.combatState, target->name << " 受到了 " << final_damage
                      << " 点伤害，破甲 " << receipt.damage_taken
                      << "，真实掉血 " << receipt.hp_lost
                      << "，剩余血量: " << target->current_hp
                      << ", 剩余格挡: " << target->block << "\n");
        } else {
            STS_LOG(*engine.combatState, target->name << " 受到了 " << final_damage
                      << " 点伤害，完全被格挡！剩余格挡: " << target->block << "\n");
        }
        
        // ==========================================
        // ★ 第二层广播：ON_UNBLOCKED_DAMAGE_TAKEN (护甲击穿)
        // 只要破甲了就发！(唤醒静电释放、多层护甲)
        // ==========================================
        if (receipt.damage_taken > 0) {
            DamageContext ctx(target.get(), source.get(), this->damageType, receipt.damage_taken);
            engine.eventBus.publish(EventType::ON_UNBLOCKED_DAMAGE_TAKEN, engine, &ctx);
        }
        
        // ==========================================
        // ★ 第三层广播：ON_HP_LOST (肉体流血)
        // 只有肉体真实流血了才发！(唤醒撕裂、红骷髅)
        // ==========================================
        if (receipt.hp_lost > 0) {
            DamageContext ctx(target.get(), source.get(), this->damageType, receipt.hp_lost);
            engine.eventBus.publish(EventType::ON_HP_LOST, engine, &ctx);
        }
        
        // 死亡判定由 SBA（全局巡视）负责，Action 只管执行
    }
    return true;
}

// ==========================================
// RandomDamageAction 实现
//
// 特性：
// - 随机选择存活的怪物作为目标
// - 将实际伤害委托给 DamageAction
// - 若没有存活目标则直接结束
// ==========================================
RandomDamageAction::RandomDamageAction(std::shared_ptr<Character> src, int dmg,
                                       DamageType type)
    : source(src), damage(dmg), damageType(type) {}

bool RandomDamageAction::update(GameEngine& engine) {
    if (!source || source->isDead()) {
        return true;
    }

    std::vector<std::shared_ptr<Monster>> alive;
    for (auto& m : engine.combatState->monsters) {
        if (!m->isDead()) {
            alive.push_back(m);
        }
    }

    if (alive.empty()) {
        return true;
    }

    std::uniform_int_distribution<size_t> dist(0, alive.size() - 1);
    auto target = alive[dist(engine.combatState->combatRng.combatRng)];

    engine.actionManager.addActionToFront(std::make_unique<DamageAction>(
        source, target, damage, damageType));

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

bool LoseHpAction::update(GameEngine& engine) {
    if (!target->isDead()) {
        // 1. 直接命令目标掉血，把底层拦截任务交给 loseHp 自己处理！
        // 拿到真实掉血的收据
        int actual_hp_lost = target->loseHp(amount);
        
        // 2. 如果真的掉血了，才发布事件！
        if (actual_hp_lost > 0) {
            STS_LOG(*engine.combatState, target->name << " 失去了 " << actual_hp_lost
                      << " 点生命值（无视护甲），剩余血量: " << target->current_hp << "\n");
            DamageContext ctx(target.get(), nullptr, DamageType::HP_LOSS, actual_hp_lost);
            engine.eventBus.publish(EventType::ON_HP_LOST, engine, &ctx);
        }
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

bool GainBlockAction::update(GameEngine& engine) {
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

        STS_LOG(*engine.combatState, target->name << " 获得了 " << final_block
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

bool ApplyPowerAction::update(GameEngine& engine) {
    if (!target->isDead()) {
        // ==========================================
        // 状态叠加逻辑（委托给 Character 接口）
        // 
        // Character::addPower 内部处理：
        // - 查找同名状态
        // - 存在则调用 stackPower 叠加
        // - 不存在则添加新状态
        // ==========================================
        
        bool isNewPower = target->addPower(power);
        
        if (isNewPower) {
            bool isMonsterSource = (source != engine.combatState->player);
            if (!engine.combatState->isPlayerTurn && isMonsterSource) {
                power->setJustApplied(true);
                ENGINE_TRACE("保护罩激活: " << power->name << " 刚挂上，本轮次不掉层");
            } else {
                power->setJustApplied(false);
            }

            STS_LOG(*engine.combatState, "-> 给 " << target->name << " 施加了 " << power->getAmount()
                      << " 层 [" << power->name << "]\n");
            power->onApply(engine);
        } else {
            // 叠加到已有状态
            auto existingPower = target->getPower(power->name);
            STS_LOG(*engine.combatState, "-> " << target->name << " 的 [" << power->name
                      << "] 叠加到 " << existingPower->getAmount() << " 层\n");
        }
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

bool ReducePowerAction::update(GameEngine& engine) {
    if (power && power->getAmount() > 0) {
        power->setAmount(power->getAmount() - reduceAmount);
        STS_LOG(*engine.combatState, "-> " << target->name << " 的 [" << power->name
                  << "] 减少了 " << reduceAmount << " 层，剩余 "
                  << power->getAmount() << " 层。\n");
        
        // 层数归零时，推入强制移除动作
        if (power->getAmount() <= 0) {
            STS_LOG(*engine.combatState, "-> [" << power->name << "] 已完全消散。\n");
            engine.actionManager.addAction(std::make_unique<RemoveSpecificPowerAction>(target, power));
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

bool RemoveSpecificPowerAction::update(GameEngine& engine) {
    if (power) {
        STS_LOG(*engine.combatState, "-> 强制净化！[" << power->name << "] 被直接从 "
                  << target->name << " 身上移除。\n");

        power->onRemove(engine);
        target->removePower(power);
        
        ENGINE_TRACE("Power 强制移除: " << power->name << " 已从 " << target->name << " 身上移除");
    }
    return true;
}

// ==========================================
// RequestCardSelectionAction 实现
// 
// 核心职责：
// 1. 根据 PileType 实时获取牌堆（解决时序问题）
// 2. 切换状态，冻结引擎
// 3. 写入选牌上下文（使用 CardSelectionContext）
// 4. 返回 true，将自己踢出队列
// 
// 引擎由于 Phase 改变，将自动暂停推演
// ==========================================
bool RequestCardSelectionAction::update(GameState& state) {
    // 1. 根据 PileType 实时获取牌堆
    std::vector<std::shared_ptr<AbstractCard>>* pile = nullptr;
    
    switch (sourcePileType) {
        case PileType::HAND:
            pile = &engine.combatState->hand;
            break;
        case PileType::DRAW_PILE:
            pile = &engine.combatState->drawPile;
            break;
        case PileType::DISCARD_PILE:
            pile = &engine.combatState->discardPile;
            break;
        case PileType::EXHAUST_PILE:
            pile = &engine.combatState->exhaustPile;
            break;
        case PileType::LIMBO:
            pile = &engine.combatState->limbo;
            break;
        default:
            pile = nullptr;
            STS_LOG(*engine.combatState, "[警告] 未知的牌堆类型: " << static_cast<int>(sourcePileType));
            break;
    }

    if (!pile || pile->empty()) {
        return true;  // 无牌可选，直接跳过
    }

    // 2. 切换状态，冻结引擎
    engine.combatState->currentPhase = StatePhase::WAITING_FOR_CARD_SELECTION;

    / 3. 写入上下文（使用 CardSelectionContext）
    CardSelectionContext ctx;
    ctx.choices = *pile;
    ctx.purpose = purpose;
    ctx.minSelection = std::min(this->minSelection, static_cast<int>(pile->size()));
    ctx.maxSelection = std::min(this->maxSelection, static_cast<int>(pile->size()));
    engine.combatState->selectionCtx = ctx;

    // 4. 输出日志
    if (ctx.minSelection == ctx.maxSelection) {
        STS_LOG(*engine.combatState, "[选牌请求] 请选择 " << ctx.minSelection << " 张牌\n");
    } else {
        STS_LOG(*engine.combatState, "[选牌请求] 请选择 " << ctx.minSelection << "-" << ctx.maxSelection << " 张牌\n");
    }
    for (size_t i = 0; i < pile->size(); ++i) {
        STS_LOG(*engine.combatState, "  [" << i << "] " << (*pile)[i]->id << "\n");
    }

    // 5. 返回 true，将自己踢出队列
    // 此时引擎由于 Phase 改变，将自动暂停推演
    return true;
}

// ==========================================
// SpecificCardExhaustAction 实现
// 
// 委托给 DeckSystem
// ==========================================
bool SpecificCardExhaustAction::update(GameEngine& engine) {
    if (targetCard) {
        DeckSystem::moveToExhaust(engine, targetCard);
    }
    return true;
}

// ==========================================
// MoveCardToHandAction 实现
// 
// 委托给 DeckSystem
// 爆牌判定：手牌 >= 10 时拿不到手牌中
// ==========================================
bool MoveCardToHandAction::update(GameEngine& engine) {
    if (targetCard) {
        DeckSystem::moveToHand(engine, targetCard);
    }
    return true;
}

// ==========================================
// SpecificCardDiscardAction 实现
// 
// 委托给 DeckSystem
// ==========================================
bool SpecificCardDiscardAction::update(GameEngine& engine) {
    if (targetCard) {
        DeckSystem::moveToDiscard(engine, targetCard);
    }
    return true;
}

// ==========================================
// UseCardAction 实现
// 
// 滞留区善后：卡牌结算完毕后的最终去向判定
// ==========================================
bool UseCardAction::update(GameEngine& engine) {
    if (!targetCard) {
        return true;
    }

    // 1. 从 limbo 滞留区中擦除该卡牌
    DeckSystem::eraseFromLimbo(engine, targetCard);

    // 2. 判定最终去向
    if (targetCard->isExhaust) {
        // 消耗牌：进入消耗堆
        engine.combatState->exhaustPile.push_back(targetCard);
        engine.eventBus.publish(EventType::ON_CARD_EXHAUSTED, engine, targetCard.get());
        STS_LOG(*engine.combatState, "-> " << targetCard->id << " 已消耗。\n");
    } else if (targetCard->type == CardType::POWER) {
        // 能力牌：不进入任何物理牌堆（直接消失）
        STS_LOG(*engine.combatState, "-> " << targetCard->id << " (能力牌) 已生效。\n");
    } else {
        // 普通牌：进入弃牌堆
        engine.combatState->discardPile.push_back(targetCard);
    }

    return true;
}

// ==========================================
// DrawCardsAction 实现
// 
// 委托给 DeckSystem
// ==========================================
bool DrawCardsAction::update(GameEngine& engine) {
    DeckSystem::drawCards(engine, amount);
    return true;
}

// ==========================================
// ShuffleDiscardIntoDrawAction 实现
// 
// 委托给 DeckSystem
// ==========================================
bool ShuffleDiscardIntoDrawAction::update(GameEngine& engine) {
    DeckSystem::shuffleDiscardIntoDraw(engine);
    return true;
}

// ==========================================
// DiscardHandAction 实现
//
// 委托给 DeckSystem
// ==========================================
bool DiscardHandAction::update(GameEngine& engine) {
    DeckSystem::discardHand(engine);
    return true;
}

bool ResetAllBrainsAction::update(GameEngine& engine) {
    for (auto& monster : engine.combatState->monsters) {
        if (!monster || monster->isDead()) {
            continue;
        }
        auto brain = monster->getBrain();
        if (brain) {
            brain->reset();
        }
    }
    return true;
}
