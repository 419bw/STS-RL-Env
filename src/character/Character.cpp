#include "Character.h"
#include <iostream>

// ==========================================
// Character 实现
// ==========================================
void Character::damage(int amount) {
    float final_damage = static_cast<float>(amount);
    
    // 1. 计算所有状态的修饰效果 (例如：易伤放大伤害)
    for (auto& power : powers) {
        final_damage = power->modifyDamageTaken(final_damage);
    }
    
    int actual_damage = static_cast<int>(final_damage);

    // 2. 扣除格挡与血量
    if (block >= actual_damage) {
        block -= actual_damage;
    } else {
        actual_damage -= block;
        block = 0;
        current_hp -= actual_damage;
        if (current_hp < 0) current_hp = 0;
    }
    std::cout << name << " 受到了伤害，剩余血量: " << current_hp 
              << ", 剩余格挡: " << block << "\n";
}
