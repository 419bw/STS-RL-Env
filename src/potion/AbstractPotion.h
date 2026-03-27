#pragma once

#include <string>
#include <memory>
#include "src/core/Types.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 药水基类 (AbstractPotion)
// ==========================================

class AbstractPotion {
public:
    std::string id;
    PotionTarget targetType;

    AbstractPotion(std::string i, PotionTarget target = PotionTarget::SELF)
        : id(i), targetType(target) {}
    virtual ~AbstractPotion() = default;

    virtual void use(GameState& state, std::shared_ptr<Character> target = nullptr) = 0;
};