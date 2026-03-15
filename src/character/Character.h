#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include "src/core/ForwardDeclarations.h"
#include "src/power/AbstractPower.h"

// ==========================================
// 实体基类 (Character) - 玩家和怪物的通用属性
// ==========================================

class Character : public std::enable_shared_from_this<Character> {
public:
    std::string name;
    int current_hp;
    int max_hp;
    int block; // 格挡值
    std::vector<std::shared_ptr<AbstractPower>> powers; // 角色身上的状态列表

    Character(std::string n, int hp) 
        : name(n), current_hp(hp), max_hp(hp), block(0) {}
    virtual ~Character() = default;

    // 基础受伤逻辑 (加入修饰型状态的计算)
    void damage(int amount);

    // 获得格挡
    void addBlock(int amount) {
        block += amount;
        std::cout << name << " 获得了 " << amount << " 点格挡.\n";
    }
    
    // 判断是否死亡
    bool isDead() const { return current_hp <= 0; }
};

// ==========================================
// 玩家类 (Player)
// ==========================================
class Player : public Character {
public:
    int energy; // 当前费用
    Player(std::string n, int hp) : Character(n, hp), energy(3) {}
};

// ==========================================
// 怪物类 (Monster)
// ==========================================
class Monster : public Character {
public:
    Monster(std::string n, int hp) : Character(n, hp) {}
    // 实际项目中这里需要包含怪物的意图(Intent)系统
};
