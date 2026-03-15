#pragma once

#include "AbstractRelic.h"
#include "src/core/ForwardDeclarations.h"
#include "src/core/Types.h"

// ==========================================
// 具体遗物：金刚杵 (Vajra) - 每次打出攻击牌产生额外效果
// ==========================================
class CustomVajraRelic : public AbstractRelic {
public:
    CustomVajraRelic() : AbstractRelic("魔改金刚杵") {}
    void onEquip(GameState& state) override;
};

// ==========================================
// 具体遗物：化学物X (Chemical X) - 为X费牌注入额外能量
// ==========================================
class ChemicalXRelic : public AbstractRelic {
public:
    ChemicalXRelic() : AbstractRelic("化学物X") {}
    void onEquip(GameState& state) override;
};
