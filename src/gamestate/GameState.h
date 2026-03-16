#pragma once

#include <memory>
#include <vector>
#include <deque>
#include <optional>
#include "src/core/Types.h"
#include "src/event/EventBus.h"
#include "src/character/Character.h"
#include "src/action/AbstractAction.h"
#include "src/card/AbstractCard.h"

// ==========================================
// GameState - 纯数据容器 (Anemic Domain Model)
// 
// 铁律一：GameState 必须是纯数据容器
// 内部禁止包含任何游戏业务逻辑代码
// 只包含：实体数据、EventBus、ActionQueue
// 
// 铁律二：所有状态机开关都在这里
// - currentPhase: 玩家行动子状态
// - selectionCtx: 选牌上下文（可选）
// 
// 铁律三：所有业务逻辑由 ActionSystem 执行
// ==========================================

class GameState {
public:
    // 回合计数
    int turnCount;
    bool isPlayerDead;
    bool isMonsterDead;

    // 宏观时间开关：记录当前是玩家回合还是怪物回合
    bool isPlayerTurn;

    // RL 训练控制开关：默认开启，方便调试
    // 接入 AI 训练时设为 false，可实现静默训练
    bool enableLogging;

    // 游戏实体
    std::shared_ptr<Player> player;
    std::vector<std::shared_ptr<Monster>> monsters;
    
    // 事件总线
    EventBus eventBus;

    // 牌库系统
    std::vector<std::shared_ptr<AbstractCard>> drawPile;
    std::vector<std::shared_ptr<AbstractCard>> hand;
    std::vector<std::shared_ptr<AbstractCard>> discardPile;

    // ==========================================
    // 动作队列 (使用 deque 支持 push_front)
    // ==========================================
    std::deque<std::unique_ptr<AbstractAction>> actionQueue;

    // ==========================================
    // 玩家行动子状态
    // ==========================================
    StatePhase currentPhase = StatePhase::PLAYING_CARD;

    // ==========================================
    // 选牌上下文（可选）
    // 
    // 使用 std::optional 管理：
    // - 有值：引擎挂起，等待选牌
    // - 无值：自由出牌阶段
    // 
    // 区间控制示例：
    // - "必须选1张" -> minSelection=1, maxSelection=1
    // - "最多选2张" -> minSelection=0, maxSelection=2
    // ==========================================
    std::optional<CardSelectionContext> selectionCtx = std::nullopt;

    GameState() 
        : turnCount(0),
          isPlayerDead(false),
          isMonsterDead(false),
          isPlayerTurn(false),
          enableLogging(true) {
        player = std::make_shared<Player>("战士", 80);
    }

    // ==========================================
    // 轻量级容器包装函数
    // 保留这些函数以避免大规模修改现有代码
    // ==========================================
    
    // 添加动作到队列尾部
    void addAction(std::unique_ptr<AbstractAction> action) {
        actionQueue.push_back(std::move(action));
    }
    
    // 添加动作到队列头部（用于选牌后插入结算动作）
    void addActionToFront(std::unique_ptr<AbstractAction> action) {
        actionQueue.push_front(std::move(action));
    }
    
    // 队列是否为空
    bool isActionQueueEmpty() const {
        return actionQueue.empty();
    }
};
