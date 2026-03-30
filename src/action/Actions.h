#pragma once

#include "AbstractAction.h"
#include "src/core/ForwardDeclarations.h"
#include "src/core/Types.h"
#include <memory>
#include <string>
#include <vector>

// ==========================================
// 占位动作 (DummyAction) - 用于状态机流程
//
// 纯状态机推进，不携带任何业务逻辑
// 用途：作为 Placeholder，空占一个队列位置
// ==========================================
class DummyAction : public AbstractAction {
    std::string name;
public:
    DummyAction(std::string n) : name(n) {}
    bool update(GameEngine& engine) override;
};

// ==========================================
// 刷新所有怪物意图 (RollAllMonsterIntentsAction)
//
// 物理行为：遍历所有怪物，调用其 Brain 生成意图
// 异常安全：如果怪物已死亡，跳过
// 触发时机：回合开始（MONSTER_TURN_START）
// ==========================================
class RollAllMonsterIntentsAction : public AbstractAction {
public:
    RollAllMonsterIntentsAction() = default;
    bool update(GameEngine& engine) override;
};

// ==========================================
// 怪物执行回合 (MonsterTakeTurnAction)
//
// 核心职责：驱动怪物执行回合
// 1. 获取真实 Intent（通过 Brain）
// 2. 根据 Intent 类型生成对应 Action 队列
// 3. 若怪物死亡，跳过
// 4. 触发 ON_TURN_END（供中毒结算用）
// ==========================================
class MonsterTakeTurnAction : public AbstractAction {
    std::shared_ptr<Monster> monster;
public:
    MonsterTakeTurnAction(std::shared_ptr<Monster> m) : monster(m) {}
    bool update(GameEngine& engine) override;
};

// ==========================================
// 伤害动作 (DamageAction)
//
// 数据驱动原则：
// - 必须携带溯源信息（source: 伤害来源）
// - Action 负责将 source 传递给计算层
// - Character::calculateFinalDamage 纯计算（可用于预测）
// - Character::takeDamage 执行扣血
// - DamageAction 负责日志输出和事件发布
//
// 事件发布：
// - ON_ATTACKED：被攻击时（不论有没有格挡，唤醒荆棘）
// - ON_UNBLOCKED_DAMAGE_TAKEN：格挡击穿时（唤醒静电释放）
// - ON_HP_LOST：真实掉血时（唤醒红骷髅）
// ==========================================
class DamageAction : public AbstractAction {
    std::shared_ptr<Character> source;
    std::shared_ptr<Character> target;
    int amount;
    DamageType damageType;
public:
    DamageAction(std::shared_ptr<Character> src, std::shared_ptr<Character> tgt, int a,
                 DamageType type = DamageType::ATTACK);
    bool update(GameEngine& engine) override;
};

// ==========================================
// 随机伤害动作 (RandomDamageAction)
//
// 特性：
// - 随机选择存活的怪物作为目标
// - 将实际伤害委托给 DamageAction
// - 若没有存活目标则直接结束
//
// 使用场景：
// - 【飞镖】：3 次随机 3 点伤害
// - 【多重打击】：2 次随机 4 点伤害
// ==========================================
class RandomDamageAction : public AbstractAction {
    std::shared_ptr<Character> source;
    int damage;
    DamageType damageType;
public:
    RandomDamageAction(std::shared_ptr<Character> src, int dmg,
                       DamageType type = DamageType::ATTACK);
    bool update(GameEngine& engine) override;
};

// ==========================================
// 失去生命值动作 (LoseHpAction)
//
// 特性：
// - 无视护甲，直接扣除生命值
// - 通过计算管线（状态 + 遗物拦截）
// - 发布 ON_HP_LOST 事件
// - 用于中毒、献祭等效果
//
// 与 DamageAction 的区别：
// - LoseHpAction 不触发格挡计算
// - LoseHpAction 不触发 ON_ATTACKED
// - LoseHpAction 不触发 ON_UNBLOCKED_DAMAGE_TAKEN
// ==========================================
class LoseHpAction : public AbstractAction {
    std::shared_ptr<Character> target;
    int amount;
public:
    LoseHpAction(std::shared_ptr<Character> t, int a);
    bool update(GameEngine& engine) override;
};

// ==========================================
// 获得格挡动作 (GainBlockAction)
//
// 设计原则：Action 负责日志和事件
// - Character::calculateFinalBlock 纯计算（可用于预测）
// - Character::addBlockFinal 执行获得格挡
// - GainBlockAction 负责日志输出和事件发布
// ==========================================
class GainBlockAction : public AbstractAction {
    std::shared_ptr<Character> target;
    int amount;
public:
    GainBlockAction(std::shared_ptr<Character> t, int a);
    bool update(GameEngine& engine) override;
};

// ==========================================
// 施加状态动作 (ApplyPowerAction)
//
// 保护罩逻辑：
// 1. 查阅黑板发现 !state.isPlayerTurn (玩家回合已结束)
// 2. 且释放者是怪物 (source != player)
// 3. 如果满足，设置 justApplied = true，防止本轮次掉层
//
// 事件发布：
// - Power::onApply() 由具体 Power 类实现
// - onApply 可发布更细粒度的事件
// ==========================================
class ApplyPowerAction : public AbstractAction {
    std::shared_ptr<Character> source;
    std::shared_ptr<Character> target;
    std::shared_ptr<AbstractPower> power;
public:
    ApplyPowerAction(std::shared_ptr<Character> src,
                      std::shared_ptr<Character> tgt,
                      std::shared_ptr<AbstractPower> p);
    bool update(GameEngine& engine) override;
};

// ==========================================
// 减少状态层数动作 (ReducePowerAction)
//
// 只负责减少层数，不负责移除
// 如果层数归零，推入 RemoveSpecificPowerAction 来清理
//
// 设计注意：
// - 每减少 1 层推一次 Action，而非一次性减完
// - 这样可以正确触发"每减少 1 层触发一次"的效果
// ==========================================
class ReducePowerAction : public AbstractAction {
    std::shared_ptr<Character> target;
    std::shared_ptr<AbstractPower> power;
    int reduceAmount;
public:
    ReducePowerAction(std::shared_ptr<Character> t, std::shared_ptr<AbstractPower> p, int a);
    bool update(GameEngine& engine) override;
};

// ==========================================
// 移除特定状态动作 (RemoveSpecificPowerAction)
//
// 无视层数，直接从目标身上移除指定的 Power
// 触发 onRemove 遗言，用于清理事件订阅等
//
// 与 ReducePowerAction 的区别：
// - ReducePowerAction 是"减少"
// - RemoveSpecificPowerAction 是"强制移除"（无视层数）
// ==========================================
class RemoveSpecificPowerAction : public AbstractAction {
    std::shared_ptr<Character> target;
    std::shared_ptr<AbstractPower> power;
public:
    RemoveSpecificPowerAction(std::shared_ptr<Character> t, std::shared_ptr<AbstractPower> p);
    bool update(GameEngine& engine) override;
};

// ==========================================
// 请求选牌动作 (RequestCardSelectionAction)
//
// 核心职责：
// 1. 根据 PileType 实时获取牌堆（解决时序问题）
// 2. 切换状态，冻结引擎
// 3. 写入选牌上下文
// 4. 返回 true，将自己踢出队列
//
// 设计注意：
// - 创建时只记录 PileType，执行时实时获取最新牌堆
// - 这样可以避免"创建时牌堆快照"导致的时序问题
// ==========================================
class RequestCardSelectionAction : public AbstractAction {
    PileType sourcePileType;
    SelectionPurpose purpose;
    int minSelection;
    int maxSelection;

public:
    RequestCardSelectionAction(
        PileType pileType,
        SelectionPurpose p,
        int minAmt = 1,
        int maxAmt = 1
    ) : sourcePileType(pileType), purpose(p), minSelection(minAmt), maxSelection(maxAmt) {}

    bool update(GameEngine& engine) override;
};

// ==========================================
// 指定卡牌消耗动作 (SpecificCardExhaustAction)
//
// 委托给 DeckSystem
// 注意：不发布 ON_CARD_DISCARDED，只发布 ON_CARD_EXHAUSTED
// ==========================================
class SpecificCardExhaustAction : public AbstractAction {
    std::shared_ptr<AbstractCard> targetCard;

public:
    SpecificCardExhaustAction(std::shared_ptr<AbstractCard> card)
        : targetCard(card) {}

    bool update(GameEngine& engine) override;
};

// ==========================================
// 移动卡牌到手牌动作 (MoveCardToHandAction)
//
// 委托给 DeckSystem
// 爆牌判定：手牌 >= 10 时进入弃牌堆
// 注意：爆牌时发布 ON_CARD_DISCARDED，而非 ON_CARD_DRAWN
// ==========================================
class MoveCardToHandAction : public AbstractAction {
    std::shared_ptr<AbstractCard> targetCard;

public:
    MoveCardToHandAction(std::shared_ptr<AbstractCard> card)
        : targetCard(card) {}

    bool update(GameEngine& engine) override;
};

// ==========================================
// 重置所有大脑动作 (ResetAllBrainsAction)
//
// 驱动层专用 Action
// 用于在特定时机（如回合开始）重置所有怪物的 Brain 状态
// 实现细节：调用每个 Brain 的 reset() 方法
// ==========================================
class ResetAllBrainsAction : public AbstractAction {
public:
    ResetAllBrainsAction() = default;
    bool update(GameEngine& engine) override;
};

// ==========================================
// 指定卡牌弃置动作 (SpecificCardDiscardAction)
//
// 委托给 DeckSystem
// 注意：先发布 ON_CARD_DISCARDED，再移入弃牌堆
// ==========================================
class SpecificCardDiscardAction : public AbstractAction {
    std::shared_ptr<AbstractCard> targetCard;

public:
    SpecificCardDiscardAction(std::shared_ptr<AbstractCard> card)
        : targetCard(card) {}

    bool update(GameEngine& engine) override;
};

// ==========================================
// 使用卡牌动作 (UseCardAction)
//
// 滞留区善后：卡牌结算完毕后的最终去向判定
// 三种归宿：
// 1. 消耗堆（isExhaust == true）
// 2. 弃牌堆（普通卡牌）
// 3. 能力牌（PowerCard）：直接生效，不进任何牌堆
// ==========================================
class UseCardAction : public AbstractAction {
    std::shared_ptr<AbstractCard> targetCard;

public:
    UseCardAction(std::shared_ptr<AbstractCard> card)
        : targetCard(card) {}

    bool update(GameEngine& engine) override;
};

// ==========================================
// 抽牌动作 (DrawCardsAction)
//
// 委托给 DeckSystem
// 内部逻辑：
// 1. 检查手牌是否已满（>= 10）
// 2. 若抽牌堆空，自动洗弃牌堆
// 3. 抽指定数量
// 4. 每抽一张发布 ON_CARD_DRAWN
// ==========================================
class DrawCardsAction : public AbstractAction {
    int amount;

public:
    DrawCardsAction(int a) : amount(a) {}

    bool update(GameEngine& engine) override;
};

// ==========================================
// 洗牌并入抽牌堆动作 (ShuffleDiscardIntoDrawAction)
//
// 委托给 DeckSystem
// 内部逻辑：
// 1. 将弃牌堆全部移入抽牌堆
// 2. 调用 std::shuffle 洗牌
// 3. 发布 ON_SHUFFLE
// ==========================================
class ShuffleDiscardIntoDrawAction : public AbstractAction {
public:
    ShuffleDiscardIntoDrawAction() = default;

    bool update(GameEngine& engine) override;
};

// ==========================================
// 弃置所有手牌动作 (DiscardHandAction)
//
// 委托给 DeckSystem
// 将手牌全部移入弃牌堆
// 发布多个 ON_CARD_DISCARDED
// ==========================================
class DiscardHandAction : public AbstractAction {
public:
    DiscardHandAction() = default;

    bool update(GameEngine& engine) override;
};
