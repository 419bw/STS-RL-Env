#pragma once

#include "src/core/ForwardDeclarations.h"
#include <string>

enum class IntentType {
    ATTACK,
    DEFEND,
    BUFF,
    DEBUFF,
    ATTACK_DEFEND,
    ATTACK_DEBUFF,
    UNKNOWN
};

inline std::string intentTypeToString(IntentType type) {
    switch (type) {
        case IntentType::ATTACK: return "ATTACK";
        case IntentType::DEFEND: return "DEFEND";
        case IntentType::BUFF: return "BUFF";
        case IntentType::DEBUFF: return "DEBUFF";
        case IntentType::ATTACK_DEFEND: return "ATTACK_DEFEND";
        case IntentType::ATTACK_DEBUFF: return "ATTACK_DEBUFF";
        case IntentType::UNKNOWN: return "UNKNOWN";
        default: return "UNDEFINED";
    }
}

struct Intent {
    IntentType type = IntentType::ATTACK;
    int base_damage = -1;
    int hit_count = 1;
    int effect_value = 0;
    Character* target = nullptr;
    bool visible = true;
};

std::string Intent_DebugString(const Intent& intent);
