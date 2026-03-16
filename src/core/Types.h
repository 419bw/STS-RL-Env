#pragma once

#include <memory>
#include <vector>

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
    ON_HP_LOST,           // 失去生命值时（无视护甲的掉血，如中毒）
    ON_TURN_START,        // 角色回合开始时 (带参：是谁的回合)
    ON_TURN_END,          // 角色回合结束时 (带参：是谁的回合)
    ON_ROUND_END,         // 整个轮次结束时 (所有带层数的状态统一在这里结算)
    
    // 牌库流转事件 (由 DeckSystem 发布)
    ON_SHUFFLE,           // 洗牌时
    ON_CARD_DRAWN,        // 抽牌时
    ON_CARD_DISCARDED,    // 卡牌进入弃牌堆时
    ON_CARD_EXHAUSTED     // 卡牌进入消耗堆时
};

// 状态效果类型
enum class PowerType { 
    BUFF,    // 增益
    DEBUFF   // 减益
};

// ==========================================
// 玩家行动子状态 (State Phase)
// 用于细化 PLAYER_ACTION 阶段
// ==========================================

enum class StatePhase {
    PLAYING_CARD,                 // 自由出牌阶段
    WAITING_FOR_CARD_SELECTION    // 队列挂起，等待选牌阶段
};

// ==========================================
// 选牌目的枚举 (Selection Purpose)
// 用于路由选牌后的物理结算动作
// ==========================================

enum class SelectionPurpose {
    EXHAUST_FROM_HAND,      // 从手牌选一张消耗 (如: 坚毅)
    MOVE_TO_HAND,           // 选一张放入手牌 (如: 全息影像、寻求)
    DISCARD_FROM_HAND       // 从手牌选一张丢弃 (如: 绝境求生)
    // [未来扩展预留位置]
};

// ==========================================
// 选牌上下文结构体 (Card Selection Context)
// 
// 封装所有选牌相关状态，使用 optional 管理
// 专供 RL 环境的 Observation 读取
// 
// 区间控制示例：
// - "必须选1张" -> minSelection=1, maxSelection=1
// - "最多选2张" -> minSelection=0, maxSelection=2
// ==========================================
struct CardSelectionContext {
    std::vector<std::shared_ptr<class AbstractCard>> choices;  // 供 AI 选择的牌库快照
    SelectionPurpose purpose;                                  // 选完之后用来干嘛
    int minSelection = 1;                                      // 最少选几张
    int maxSelection = 1;                                      // 最多选几张
};
