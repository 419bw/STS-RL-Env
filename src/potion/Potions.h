#pragma once

#include "AbstractPotion.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 力量药水 (StrengthPotion)
// ==========================================

class StrengthPotion : public AbstractPotion {
public:
    StrengthPotion();

    void use(GameEngine& engine, std::shared_ptr<Character> target) override;
};
