#pragma once

#include <memory>
#include <vector>
#include <queue>
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
// ==========================================

class GameState {
public:
    // 回合计数
    int turnCount;
    bool isPlayerDead;
    bool isMonsterDead;

    // 宏观时间开关：记录当前是玩家回合还是怪物回合
    bool isPlayerTurn;

    // 游戏实体
    std::shared_ptr<Player> player;
    std::vector<std::shared_ptr<Monster>> monsters;
    
    // 事件总线
    EventBus eventBus;

    // 牌库系统
    std::vector<std::shared_ptr<AbstractCard>> drawPile;
    std::vector<std::shared_ptr<AbstractCard>> hand;
    std::vector<std::shared_ptr<AbstractCard>> discardPile;

    // 动作队列
    std::queue<std::unique_ptr<AbstractAction>> actionQueue;

    GameState() 
        : turnCount(0),
          isPlayerDead(false),
          isMonsterDead(false),
          isPlayerTurn(false) {
        player = std::make_shared<Player>("战士", 80);
    }

    // 添加动作到队列
    void addAction(std::unique_ptr<AbstractAction> action) {
        actionQueue.push(std::move(action));
    }
    
    // 队列是否为空
    bool isActionQueueEmpty() const {
        return actionQueue.empty();
    }
};
