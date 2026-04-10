#pragma once

#include "src/card/AbstractCard.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 具体卡牌：打击 (Strike)
// ==========================================
class StrikeCard : public CloneableCard<StrikeCard> {
public:
    StrikeCard() : CloneableCard("Strike", 1, CardType::ATTACK, CardTarget::ENEMY) {}
    void use(GameEngine& engine, std::shared_ptr<Character> target) override;
};

// ==========================================
// 具体卡牌：致命毒药 (Deadly Poison)
// ==========================================
class DeadlyPoisonCard : public CloneableCard<DeadlyPoisonCard> {
public:
    DeadlyPoisonCard() : CloneableCard("Deadly Poison", 1, CardType::SKILL, CardTarget::ENEMY) {}
    void use(GameEngine& engine, std::shared_ptr<Character> target) override;
};

// ==========================================
// 具体卡牌：旋风斩 (Whirlwind) - X费牌示例
// ==========================================
class WhirlwindCard : public CloneableCard<WhirlwindCard> {
public:
    // cost 设为 -1，代表这是 X 费牌
    WhirlwindCard() : CloneableCard("Whirlwind", -1, CardType::ATTACK, CardTarget::ALL_ENEMY) {}
    void use(GameEngine& engine, std::shared_ptr<Character> target) override;
};

// ==========================================
// 具体卡牌：飞剑回旋镖 (Shuriken)
// ==========================================
class ShurikenCard : public CloneableCard<ShurikenCard> {
public:
    ShurikenCard() : CloneableCard("Shuriken", 1, CardType::ATTACK, CardTarget::RANDOM) {}
    void use(GameEngine& engine, std::shared_ptr<Character> target) override;
};

// ==========================================
// 具体卡牌：痛击 (Pain)
// ==========================================
class PainCard : public CloneableCard<PainCard> {
public:
    PainCard() : CloneableCard("Pain", 2, CardType::ATTACK, CardTarget::ENEMY) {}
    void use(GameEngine& engine, std::shared_ptr<Character> target) override;
};

// ==========================================
// 具体卡牌：防御 (Defend)
// ==========================================
class DefendCard : public CloneableCard<DefendCard> {
public:
    DefendCard() : CloneableCard("Defend", 1, CardType::SKILL, CardTarget::SELF) {}
    void use(GameEngine& engine, std::shared_ptr<Character> target) override;
};
