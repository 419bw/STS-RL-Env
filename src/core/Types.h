#pragma once

// ==========================================
// 核心枚举类型 (Enums)
// ==========================================

// 卡牌类型
enum class CardType { 
    ATTACK,   // 攻击牌
    SKILL,    // 技能牌
    POWER,    // 能力牌
    STATUS,   // 状态牌
    CURSE     // 诅咒牌
};

// 战斗状态枚举：战斗流程的生命周期
enum class CombatState {
    BATTLE_START,        // 战斗开始 (洗牌、触发初始遗物)
    TURN_START,          // 回合开始 (重置费用、触发状态效果、抽牌)
    PLAYER_ACTION,       // 玩家行动阶段 (引擎挂起，等待 AI/玩家 输入)
    TURN_END,            // 玩家回合结束 (触发回合结束效果、弃牌)
    MONSTER_TURN_START,  // 怪物回合开始 (触发怪物回合开始效果)
    MONSTER_TURN,        // 怪物行动阶段 (怪物执行意图)
    MONSTER_TURN_END,    // 怪物回合结束 (触发怪物回合结束效果)
    BATTLE_END           // 战斗结束 (胜利或失败)
};

// 事件类型
enum class EventType {
    // 宏观阶段事件 (由 CombatFlow 发布)
    PHASE_BATTLE_START,       // 战斗开始阶段
    PHASE_TURN_START,         // 回合开始阶段
    PHASE_PLAYER_ACTION,      // 玩家行动阶段
    PHASE_TURN_END,           // 玩家回合结束阶段
    PHASE_MONSTER_TURN_START, // 怪物回合开始阶段
    PHASE_MONSTER_TURN,       // 怪物行动阶段
    PHASE_MONSTER_TURN_END,   // 怪物回合结束阶段
    PHASE_BATTLE_END,         // 战斗结束阶段
    
    // 微观触发事件 (由具体动作发布)
    ON_CARD_PLAYING,      // 准备打出牌时（用于拦截并修改卡牌参数）
    ON_CARD_PLAYED,       // 打出牌后
    ON_ATTACK,            // 攻击时
    ON_DAMAGE_TAKEN,      // 受到伤害时
    ON_TURN_START,        // 回合开始时 (角色级别)
    ON_TURN_END           // 回合结束时 (角色级别)
};

// 状态效果类型
enum class PowerType { 
    BUFF,    // 增益
    DEBUFF   // 减益
};
