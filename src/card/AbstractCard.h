#pragma once

#include <string>
#include <memory>
#include "src/core/Types.h"
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 卡牌基类 (AbstractCard)
// ==========================================

class AbstractCard {
public:
    std::string id;
    int cost; // -1 表示 X 费牌
    CardType type;
    int energyOnUse; // 专门用于记录 X 费牌打出时的费用
    bool isExhaust;  // 是否消耗（打出后进入消耗堆而非弃牌堆）

    AbstractCard(std::string i, int c, CardType t) 
        : id(i), cost(c), type(t), energyOnUse(0), isExhaust(false) {}
    virtual ~AbstractCard() = default;

    // 打出卡牌的核心接口，Target可以为空（例如群体AOE或技能牌）
    virtual void use(GameState& state, std::shared_ptr<Character> target) = 0;
};
