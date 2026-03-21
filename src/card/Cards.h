#pragma once

#include "AbstractCard.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 具体卡牌：打击 (Strike)
// ==========================================
class StrikeCard : public AbstractCard {
public:
    StrikeCard() : AbstractCard("Strike", 1, CardType::ATTACK, CardTarget::ENEMY) {}
    void use(GameState& state, std::shared_ptr<Character> target) override;
};

// ==========================================
// 具体卡牌：致命毒药 (Deadly Poison)
// ==========================================
class DeadlyPoisonCard : public AbstractCard {
public:
    DeadlyPoisonCard() : AbstractCard("Deadly Poison", 1, CardType::SKILL, CardTarget::ENEMY) {}
    void use(GameState& state, std::shared_ptr<Character> target) override;
};

// ==========================================
// 具体卡牌：旋风斩 (Whirlwind) - X费牌示例
// ==========================================
class WhirlwindCard : public AbstractCard {
public:
    // cost 设为 -1，代表这是 X 费牌
    WhirlwindCard() : AbstractCard("Whirlwind", -1, CardType::ATTACK, CardTarget::ALL_ENEMY) {}
    void use(GameState& state, std::shared_ptr<Character> target) override;
};
