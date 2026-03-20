#pragma once

#include "src/core/ForwardDeclarations.h"

enum class IntentType {
    ATTACK,
    DEFEND,
    BUFF,
    DEBUFF,
    ATTACK_DEFEND,
    ATTACK_DEBUFF,
    UNKNOWN
};

struct Intent {
    IntentType type = IntentType::ATTACK;
    int base_damage = -1;
    int hit_count = 1;
    int effect_value = 0;
    Character* target = nullptr;
    bool visible = true;
};
