#pragma once

#include "AbstractRelic.h"
#include "src/core/ForwardDeclarations.h"
#include "src/core/Types.h"

// ==========================================
// 具体遗物：金刚杵 - 每次打出攻击牌产生额外效果
// ==========================================
class CustomVajraRelic : public AbstractRelic {
public:
    CustomVajraRelic() : AbstractRelic("魔改金刚杵") {}
    void onEquip(GameState& state, Character* target) override;
};

// ==========================================
// 具体遗物：化学物X (Chemical X) - 为X费牌注入额外能量
// ==========================================
class ChemicalXRelic : public AbstractRelic {
public:
    ChemicalXRelic() : AbstractRelic("化学物X") {}
    void onEquip(GameState& state, Character* target) override;
};

// ==========================================
// 具体遗物：鸟居
// 
// 效果：受到不大于5的实际扣血时，将扣血改为1
// 注意：这是在护甲之后触发，只影响伤害扣血
// 阶段：在 onActualHpLoss 生效（只对伤害生效）
// ==========================================
class ToriiRelic : public AbstractRelic {
public:
    ToriiRelic() : AbstractRelic("鸟居") {}
    
    // 实际扣血时触发（护甲之后，只对伤害生效）
    int onActualHpLoss(int hpLoss, DamageType type) override {
        if (hpLoss > 0 && hpLoss <= 5) {
            return 1;
        }
        return hpLoss;
    }
};

// ==========================================
// 具体遗物：钨合金棍
// 
// 效果：受到的所有血量减少都减1
// 注意：对所有掉血生效（伤害、中毒、献祭等）
// 阶段：在 modifyHpLoss 生效（在 loseHp 中调用）
// ==========================================
class TungstenRodRelic : public AbstractRelic {
public:
    TungstenRodRelic() : AbstractRelic("钨合金棍") {}
    
    // 所有掉血都会经过这里
    int modifyHpLoss(int amount) const override {
        return std::max(0, amount - 1);
    }
};
