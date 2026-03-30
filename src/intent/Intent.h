#pragma once

#include "src/core/ForwardDeclarations.h"
#include <string>
#include <memory>

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
    std::weak_ptr<Character> target;
    bool visible = true;
    int move_id = -1;
    std::string move_name;

    Intent() = default;

    Intent(IntentType t, int dmg, int hits, int effect, std::weak_ptr<Character> tgt)
        : type(t), base_damage(dmg), hit_count(hits), effect_value(effect), target(std::move(tgt)) {}

    Intent& withMove(int id, const std::string& name) {
        this->move_id = id;
        this->move_name = name;
        return *this;
    }

    Intent& setVisible(bool vis) {
        this->visible = vis;
        return *this;
    }
};

std::string Intent_DebugString(const Intent& intent);
