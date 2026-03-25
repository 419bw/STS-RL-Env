#pragma once

#include <string>
#include <memory>
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 药水基类 (AbstractPotion)
// ==========================================

class AbstractPotion {
public:
    std::string id;

    AbstractPotion(std::string i) : id(i) {}
    virtual ~AbstractPotion() = default;

    // 使用药水
    virtual void use(GameState& state) = 0;
};