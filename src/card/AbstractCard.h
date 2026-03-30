#pragma once

#include <string>
#include <memory>
#include "src/core/Types.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// AbstractCard - 卡牌基类
//
// ECS/DOD 架构核心组件：
// - 所有卡牌的抽象基类
// - use() 是打出卡牌的核心接口
// - clone() 支持深拷贝（用于战斗初始化）
//
// 设计原则：
// - 纯虚基类，具体卡牌必须实现 use()
// - clone() 由 CloneableCard CRTP 模板自动生成
// ==========================================

class AbstractCard {
public:
    // 卡牌标识
    std::string id;

    // 费用（-1 表示 X 费牌）
    int cost;

    // 卡牌类型
    CardType type;

    // 目标类型
    CardTarget targetType;

    // X 费牌打出时消耗的费用
    int energyOnUse;

    // 是否消耗（打出后进入消耗堆）
    bool isExhaust;

    // ==========================================
    // 构造函数
    // ==========================================
    AbstractCard(std::string i, int c, CardType t, CardTarget target)
        : id(i), cost(c), type(t), targetType(target), energyOnUse(0), isExhaust(false) {}
    virtual ~AbstractCard() = default;

    // ==========================================
    // 打出卡牌
    // @state: 游戏状态
    // @target: 目标角色（可为 nullptr，如群体技能）
    // ==========================================
    virtual void use(GameEngine& engine, std::shared_ptr<Character> target) = 0;

    // ==========================================
    // 深拷贝
    // 用于战斗初始化时克隆 masterDeck
    // 具体实现由 CloneableCard 模板自动生成
    // ==========================================
    virtual std::shared_ptr<AbstractCard> clone() const = 0;
};

// ==========================================
// CloneableCard - CRTP 模板基类
//
// 使用 CRTP（奇特的递归模板模式）自动生成 clone()
//
// 优点：
// - 子类无需手写 clone()
// - 避免忘记实现导致的切片问题
// - 编译器自动生成，类型安全
//
// 使用方法：
// class StrikeCard : public CloneableCard<StrikeCard> {
//     StrikeCard() : CloneableCard("Strike", 1, CardType::ATTACK, CardTarget::ENEMY) {}
//     void use(GameState& state, std::shared_ptr<Character> target) override;
// };
//
// 注意：
// - 必须加 using AbstractCard::AbstractCard; 继承父类构造函数
// - 禁止使用 [&] 捕获 LambdaAction，必须使用 [=]
// ==========================================
template <typename Derived>
class CloneableCard : public AbstractCard {
public:
    // 继承父类所有构造函数
    using AbstractCard::AbstractCard;

    // ==========================================
    // clone - 自动生成深拷贝
    //
    // 让编译器自动将 *this 强转为 Derived&，
    // 然后调用 Derived 的拷贝构造函数
    // ==========================================
    std::shared_ptr<AbstractCard> clone() const override {
        return std::make_shared<Derived>(static_cast<const Derived&>(*this));
    }
};
