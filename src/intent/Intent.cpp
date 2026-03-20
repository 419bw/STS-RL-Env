#include "Intent.h"
#include <string>

std::string Intent_DebugString(const Intent& intent) {
    std::string typeStr = intentTypeToString(intent.type);
    if (intent.type == IntentType::ATTACK || intent.type == IntentType::ATTACK_DEFEND) {
        return typeStr + "(伤害:" + std::to_string(intent.base_damage) +
               " x" + std::to_string(intent.hit_count) + ")";
    } else if (intent.type == IntentType::DEFEND) {
        return typeStr + "(护盾:" + std::to_string(intent.effect_value) + ")";
    } else if (intent.type == IntentType::BUFF || intent.type == IntentType::DEBUFF) {
        return typeStr + "(效果值:" + std::to_string(intent.effect_value) + ")";
    }
    return typeStr;
}