#include "Character.h"
#include "src/relic/AbstractRelic.h"
#include "src/core/Queries.h"
#include "src/utils/Logger.h"
#include <algorithm>
#include <cmath>

// ==========================================
// Character 实现
// 
// 设计原则：纯数据 + 纯计算 + 数据总线
// - 不输出日志（由 Action 负责）
// - 不发布事件（由 Action 负责）
// - 所有遗物/被动带来的乘区修正固化为面板属性
// ==========================================

// ==========================================
// 纯计算接口 (const 方法，不修改状态)
// 可用于 AI 预测、UI 显示等场景
// ==========================================

int Character::calculateFinalDamage(int base_damage, Character* source, 
                                     DamageType type) const {
    if (!source) {
        return base_damage;
    }
    
    float dmg = static_cast<float>(base_damage);
    
    // ==========================================
    // 阶段 1：攻击者的基础修饰 (加法运算为主)
    // 例如：【力量】(Strength: dmg += amount)
    //       【虚弱】(Weak: dmg *= 0.75)
    // ==========================================
    source->forEachPower([&](const auto& power) {
        dmg = power->atDamageGive(dmg, type);
    });
    source->forEachRelic([&](const auto& relic) {
        dmg = relic->atDamageGive(dmg, type);
    });
    
    // ==========================================
    // 阶段 2：防御者的基础修饰 (乘法运算为主)
    // 例如：【易伤】(Vulnerable: dmg *= 1.5)
    // ==========================================
    this->forEachPower([&](const auto& power) {
        dmg = power->atDamageReceive(dmg, type, source);
    });
    this->forEachRelic([&](const auto& relic) {
        dmg = relic->atDamageReceive(dmg, type);
    });
    
    // ==========================================
    // 阶段 3：攻击者的最终修饰 (极限乘法/覆盖)
    // 例如：【笔尖】遗物 (dmg *= 2)
    //       观者的【愤怒姿态】(dmg *= 2)
    // ==========================================
    source->forEachPower([&](const auto& power) {
        dmg = power->atDamageFinalGive(dmg, type);
    });
    source->forEachRelic([&](const auto& relic) {
        dmg = relic->atDamageFinalGive(dmg, type);
    });
    
    // ==========================================
    // 阶段 4：防御者的最终修饰 (强制截断/免伤)
    // 例如：【无实体】(Intangible: dmg = 1)
    //       【鸟居】遗物 (如果 dmg <= 5，dmg = 1)
    //       【钨合金棍】遗物 (dmg -= 1)
    // ==========================================
    this->forEachPower([&](const auto& power) {
        dmg = power->atDamageFinalReceive(dmg, type);
    });
    this->forEachRelic([&](const auto& relic) {
        dmg = relic->atDamageFinalReceive(dmg, type);
    });
    
    // 原版杀戮尖塔在最终结算时，默认向下取整
    int final_dmg = static_cast<int>(std::floor(dmg));
    
    // 伤害绝对不能是负数
    return std::max(0, final_dmg);
}

int Character::calculateFinalBlock(int base_block) const {
    float final_block = static_cast<float>(base_block);

    // 阶段 1：状态修饰 (如敏捷)
    forEachPower([&](const auto& power) {
        final_block = power->atBlockGive(final_block);
    });

    // 阶段 1：遗物修饰
    forEachRelic([&](const auto& relic) {
        final_block = relic->atBlockGive(final_block);
    });

    // 阶段 2：最终修饰
    forEachPower([&](const auto& power) {
        final_block = power->atBlockFinalGive(final_block);
    });
    forEachRelic([&](const auto& relic) {
        final_block = relic->atBlockFinalGive(final_block);
    });

    return static_cast<int>(std::floor(final_block));
}

// ==========================================
// 执行接口 (修改状态)
// 
// 设计原则：
// - takeDamage: 处理伤害（破甲 → 鸟居 → loseHp）
// - loseHp: 统一掉血入口（状态拦截 → 遗物拦截 → 扣血）
// 
// 所有掉血最终都通过 loseHp 执行
// ==========================================

Character::DamageResult Character::takeDamage(int damage, DamageType type) {
    DamageResult result;
    
    if (damage <= 0) {
        return result;
    }
    
    if (block >= damage) {
        // 护甲完全吸收
        block -= damage;
        result.damage_taken = 0;
        result.hp_lost = 0;
        return result;
    }
    
    // 计算穿透护甲后的破甲伤害
    result.damage_taken = damage - block;
    block = 0;
    
    // 鸟居：只拦截伤害类型的掉血
    int hp_to_lose = result.damage_taken;
    for (const auto& relic : relics) {
        hp_to_lose = relic->onActualHpLoss(hp_to_lose, type);
    }
    
    // 确保扣血值不为负
    if (hp_to_lose < 0) hp_to_lose = 0;
    
    // 通过 loseHp 进行最终扣血（钨合金棍在这里拦截）
    result.hp_lost = loseHp(hp_to_lose);
    
    return result;
}

int Character::loseHp(int amount) {
    if (amount <= 0) {
        return 0;
    }
    
    int final_amount = amount;
    
    // 1. 状态拦截（无实体强制变 1 等）
    for (const auto& power : powers) {
        final_amount = power->modifyHpLoss(final_amount);
    }
    
    // 2. 遗物拦截（钨合金棍减 1 等）
    for (const auto& relic : relics) {
        final_amount = relic->modifyHpLoss(final_amount);
    }
    
    // 兜底保护：掉血不可能变成回血
    final_amount = std::max(0, final_amount);
    
    // 真实扣血
    if (final_amount > 0) {
        current_hp -= final_amount;
        if (current_hp < 0) current_hp = 0;
    }
    
    return final_amount;  // 返回真实掉血的收据
}

int Character::addBlockFinal(int amount) {
    block += amount;
    return amount;
}

// ==========================================
// 查询表单处理接口（重载）
// 让身上的遗物去填表
// ==========================================

void Character::processQuery(VulnerableMultiplierQuery& query) {
    for (auto& relic : relics) {
        relic->onQuery(query);
    }
}

void Character::processQuery(WeakMultiplierQuery& query) {
    for (auto& relic : relics) {
        relic->onQuery(query);
    }
}

// ==========================================
// Power 管理接口实现
// 
// 高内聚低耦合设计：
// - 所有 Power 操作封装在 Character 内部
// - 外部只能通过接口访问
// - 叠加逻辑集中管理
// ==========================================

bool Character::addPower(std::shared_ptr<AbstractPower> power) {
    // 1. 参数有效性检查
    if (!power) {
        ENGINE_TRACE("addPower: 尝试添加空指针 Power");
        return false;
    }
    
    // 2. 查找是否存在同名状态
    for (auto& existingPower : powers) {
        if (existingPower && existingPower->name == power->name) {
            // 3. 找到同名状态，委托给状态自身处理叠加逻辑
            existingPower->stackPower(power->getAmount());
            return false;  // 叠加完成，不是新添加
        }
    }
    
    // 4. 未找到同名状态，添加新状态到数组
    power->owner = shared_from_this();
    powers.push_back(power);
    
    return true;  // 新添加成功
}

void Character::removePower(std::shared_ptr<AbstractPower> power) {
    if (!power) {
        return;
    }
    
    auto it = std::find(powers.begin(), powers.end(), power);
    if (it != powers.end()) {
        powers.erase(it);
    }
}

bool Character::hasPower(const std::string& powerName) const {
    for (const auto& power : powers) {
        if (power && power->name == powerName) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<AbstractPower> Character::getPower(const std::string& powerName) const {
    for (const auto& power : powers) {
        if (power && power->name == powerName) {
            return power;
        }
    }
    return nullptr;
}

void Character::clearPowers(GameState& state) {
    for (auto& power : powers) {
        if (power) {
            power->onRemove(state);
        }
    }
    powers.clear();
}

// ==========================================
// Relic 管理接口实现
// 
// 与 Power 管理接口设计一致：
// - 添加时自动触发 onEquip
// - 移除时自动触发 onRemove
// - 外部只能通过接口操作
// ==========================================

void Character::addRelic(std::shared_ptr<AbstractRelic> relic, GameState& state) {
    if (!relic) {
        ENGINE_TRACE("addRelic: 尝试添加空指针 Relic");
        return;
    }
    
    // 检查是否已存在同名遗物
    if (hasRelic(relic->name)) {
        ENGINE_TRACE("addRelic: 遗物 " << relic->name << " 已存在，跳过添加");
        return;
    }
    
    // 设置 owner 并添加到背包
    relic->owner = this;
    relics.push_back(relic);
    
    // 触发 onEquip 生命周期
    relic->onEquip(state, this);
}

void Character::removeRelic(std::shared_ptr<AbstractRelic> relic, GameState& state) {
    if (!relic) {
        return;
    }
    
    auto it = std::find(relics.begin(), relics.end(), relic);
    if (it != relics.end()) {
        // 触发 onRemove 生命周期
        relic->onRemove(state);
        relics.erase(it);
    }
}

bool Character::hasRelic(const std::string& relicName) const {
    for (const auto& relic : relics) {
        if (relic && relic->name == relicName) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<AbstractRelic> Character::getRelic(const std::string& relicName) const {
    for (const auto& relic : relics) {
        if (relic && relic->name == relicName) {
            return relic;
        }
    }
    return nullptr;
}

void Character::clearRelics(GameState& state) {
    for (auto& relic : relics) {
        if (relic) {
            relic->onRemove(state);
        }
    }
    relics.clear();
}
