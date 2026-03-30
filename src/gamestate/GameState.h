#pragma once

#include <memory>
#include <vector>
#include <deque>
#include <optional>
#include <random>
#include "src/core/Types.h"
#include "src/core/RandomManager.h"
#include "src/event/EventBus.h"
#include "src/character/Character.h"
#include "src/action/AbstractAction.h"
#include "src/card/AbstractCard.h"

// ==========================================
// RandomManager - 隔离随机数生成器
// 已移至 src/core/RandomManager.h
// ==========================================

// ==========================================
// GameState - 纯数据容器 (Anemic Domain Model) 已弃用
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
// 
// 铁律四：绝对局部确定性
// - 所有随机操作必须使用 rng 中的隔离 RNG
// - 严禁使用全局随机数生成器
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

    // ==========================================
    // 隔离随机数生成器
    // 保证多线程环境下的绝对局部确定性
    // ==========================================
    RandomManager rng;

    // ==========================================
    // 牌库系统 (5 个物理牌堆)
    // 
    // drawPile:    抽牌堆
    // hand:        手牌
    // discardPile: 弃牌堆
    // exhaustPile: 消耗堆
    // limbo:       滞留区 - 极其重要！
    //              用于存放正在执行自身效果的卡牌
    //              防止洗牌时被错误洗回抽牌堆
    // ==========================================
    std::vector<std::shared_ptr<AbstractCard>> drawPile;
    std::vector<std::shared_ptr<AbstractCard>> hand;
    std::vector<std::shared_ptr<AbstractCard>> discardPile;
    std::vector<std::shared_ptr<AbstractCard>> exhaustPile;
    std::vector<std::shared_ptr<AbstractCard>> limbo;
    std::vector<std::shared_ptr<AbstractPotion>> potions;

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

    // ==========================================
    // 构造函数
    // seed: 随机种子，默认 1337，保证可复现性
    // ==========================================
    GameState(unsigned int seed = 1337) 
        : turnCount(0),
          isPlayerDead(false),
          isMonsterDead(false),
          isPlayerTurn(false),
          enableLogging(true),
          rng(seed) {
        player = std::make_shared<Player>("战士", 80);
    }

    // ==========================================
    // 动作队列接口（封装 actionQueue）
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

private:
    // ==========================================
    // 动作队列（私有，外部通过接口访问）
    // 使用 deque 支持 push_front
    // ==========================================
    std::deque<std::unique_ptr<AbstractAction>> actionQueue;

    // ==========================================
    // 当前正在执行的动作（指针转移模式）
    // 避免引用队列头部带来的迭代器失效问题
    // ==========================================
    std::unique_ptr<AbstractAction> currentAction = nullptr;

    // 允许 ActionSystem 直接访问队列进行执行
    friend class ActionSystem;
};
