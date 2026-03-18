#pragma once

#include "AbstractAction.h"
#include "src/core/ForwardDeclarations.h"
#include "src/core/Types.h"
#include <memory>
#include <string>
#include <vector>

// ==========================================
// 占位动作 (Dummy Action) - 用于状态机流程
// ==========================================
class DummyAction : public AbstractAction {
    std::string name;
public:
    DummyAction(std::string n) : name(n) {}
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：伤害动作 (Damage Action)
// 
// 数据驱动原则：
// - 必须携带溯源信息（source: 伤害来源）
// - Action 负责将 source 传递给计算层
// - 会受到护甲、状态效果等影响
// ==========================================
class DamageAction : public AbstractAction {
    std::shared_ptr<Character> source;  // 伤害来源（攻击者）
    std::shared_ptr<Character> target;  // 伤害目标
    int amount;
public:
    DamageAction(std::shared_ptr<Character> src, std::shared_ptr<Character> tgt, int a);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：直接扣血动作 (Lose HP Action)
// 
// 特性：
// - 无视护甲，直接扣除生命值
// - 不触发伤害相关事件
// - 用于中毒、献祭等效果
// ==========================================
class LoseHpAction : public AbstractAction {
    std::shared_ptr<Character> target;
    int amount;
public:
    LoseHpAction(std::shared_ptr<Character> t, int a);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：获得格挡动作 (Gain Block Action)
// ==========================================
class GainBlockAction : public AbstractAction {
    std::shared_ptr<Character> target;
    int amount;
public:
    GainBlockAction(std::shared_ptr<Character> t, int a);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：施加状态效果 (Apply Power Action)
// ==========================================
class ApplyPowerAction : public AbstractAction {
    std::shared_ptr<Character> source;
    std::shared_ptr<Character> target;
    std::shared_ptr<AbstractPower> power;
public:
    ApplyPowerAction(std::shared_ptr<Character> src, 
                      std::shared_ptr<Character> tgt, 
                      std::shared_ptr<AbstractPower> p);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：减少状态效果层数 (Reduce Power Action)
// ==========================================
class ReducePowerAction : public AbstractAction {
    std::shared_ptr<Character> target;
    std::shared_ptr<AbstractPower> power;
    int reduceAmount;
public:
    ReducePowerAction(std::shared_ptr<Character> t, std::shared_ptr<AbstractPower> p, int a);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：强制移除状态效果 (Remove Specific Power Action)
// 
// 无视层数，直接从目标身上移除指定的 Power
// 用于状态层数归零时的清理
// ==========================================
class RemoveSpecificPowerAction : public AbstractAction {
    std::shared_ptr<Character> target;
    std::shared_ptr<AbstractPower> power;
public:
    RemoveSpecificPowerAction(std::shared_ptr<Character> t, std::shared_ptr<AbstractPower> p);
    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：请求选牌 (Request Card Selection Action)
// 
// 核心职责：
// 1. 切换状态，冻结引擎
// 2. 写入选牌上下文
// 3. 返回 true，将自己踢出队列
// 
// 引擎由于 Phase 改变，将自动暂停推演
// 
// 支持区间选择：
// - "必须选1张" -> minSelection=1, maxSelection=1
// - "最多选2张" -> minSelection=0, maxSelection=2
// 
// 时序问题解决方案：
// - 使用 PileType 替代 sourcePile
// - 创建时只记录牌堆类型
// - 执行时实时获取最新牌堆状态
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

    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：消耗指定卡牌 (Specific Card Exhaust Action)
// 
// 极其原子的物理结算动作
// 由 chooseCard 根据 Purpose 路由创建
// ==========================================
class SpecificCardExhaustAction : public AbstractAction {
    std::shared_ptr<AbstractCard> targetCard;

public:
    SpecificCardExhaustAction(std::shared_ptr<AbstractCard> card)
        : targetCard(card) {}

    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：将卡牌移入手牌 (Move Card To Hand Action)
// ==========================================
class MoveCardToHandAction : public AbstractAction {
    std::shared_ptr<AbstractCard> targetCard;

public:
    MoveCardToHandAction(std::shared_ptr<AbstractCard> card)
        : targetCard(card) {}

    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：丢弃指定卡牌 (Specific Card Discard Action)
// ==========================================
class SpecificCardDiscardAction : public AbstractAction {
    std::shared_ptr<AbstractCard> targetCard;

public:
    SpecificCardDiscardAction(std::shared_ptr<AbstractCard> card)
        : targetCard(card) {}

    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：使用卡牌后的善后处理 (Use Card Action)
// 
// 核心职责：
// 1. 从 limbo 滞留区中擦除该卡牌
// 2. 判定最终去向（消耗堆/弃牌堆/能力牌消失）
// 
// 时序：必须在 card->use() 之后执行
// ==========================================
class UseCardAction : public AbstractAction {
    std::shared_ptr<AbstractCard> targetCard;

public:
    UseCardAction(std::shared_ptr<AbstractCard> card)
        : targetCard(card) {}

    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：抽牌动作 (Draw Cards Action)
// 
// 从抽牌堆抽取指定数量的牌
// 自动触发洗牌和爆牌判定
// ==========================================
class DrawCardsAction : public AbstractAction {
    int amount;

public:
    DrawCardsAction(int a) : amount(a) {}

    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：洗牌动作 (Shuffle Discard Into Draw Action)
// 
// 将弃牌堆洗入抽牌堆
// 使用隔离 RNG 保证确定性
// ==========================================
class ShuffleDiscardIntoDrawAction : public AbstractAction {
public:
    ShuffleDiscardIntoDrawAction() = default;

    bool update(GameState& state) override;
};

// ==========================================
// 具体动作：弃掉所有手牌动作 (Discard Hand Action)
// 
// 将所有手牌移入弃牌堆
// 用于回合结束时
// ==========================================
class DiscardHandAction : public AbstractAction {
public:
    DiscardHandAction() = default;

    bool update(GameState& state) override;
};
