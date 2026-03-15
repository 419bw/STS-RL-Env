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
    
    ROUND_START,         // 轮次开始 (所有角色回到起始状态)
    
    PLAYER_TURN_START,   // 玩家回合开始 (重置费用、触发回合开始遗物、抽牌)
    PLAYER_ACTION,       // 玩家行动阶段 (引擎挂起，等待 AI/玩家 输入)
    PLAYER_TURN_END,     // 玩家回合结束 (触发手牌清空、弃牌)
    
    MONSTER_TURN_START,  // 怪物回合开始 (如果有多个怪物，这里会循环)
    MONSTER_TURN,        // 怪物行动阶段 (怪物执行意图)
    MONSTER_TURN_END,    // 怪物回合结束
    
    ROUND_END,           // 轮次结束 (所有怪物行动完毕，触发各种Debuff掉层、中毒结算等)
    
    BATTLE_END           // 战斗结束 (胜利或失败)
};

// 事件类型
enum class EventType {
    // 宏观阶段事件 (由 CombatFlow 状态机发布)
    PHASE_BATTLE_START,       
    PHASE_ROUND_START,        
    PHASE_PLAYER_TURN_START,  
    PHASE_PLAYER_ACTION,      
    PHASE_PLAYER_TURN_END,    
    PHASE_MONSTER_TURN_START, 
    PHASE_MONSTER_TURN,       
    PHASE_MONSTER_TURN_END,   
    PHASE_ROUND_END,          
    PHASE_BATTLE_END,         
    
    // 微观触发事件 (由具体动作/状态发布)
    ON_CARD_PLAYING,      // 准备打出牌时
    ON_CARD_PLAYED,       // 打出牌后
    ON_ATTACK,            // 攻击时
    ON_DAMAGE_TAKEN,      // 受到伤害时
    ON_TURN_START,        // 角色回合开始时 (带参：是谁的回合)
    ON_TURN_END,          // 角色回合结束时 (带参：是谁的回合)
    ON_ROUND_END          // 整个轮次结束时 (所有带层数的状态统一在这里结算)
};

// 状态效果类型
enum class PowerType { 
    BUFF,    // 增益
    DEBUFF   // 减益
};
