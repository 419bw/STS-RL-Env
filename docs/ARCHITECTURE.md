# STS_CPP 架构设计文档

## 1. 代码架构层次划分

### 1.1 模块目录结构

```
src/
├── action/          # Action 队列系统（所有状态变更的唯一入口）
├── card/            # 卡牌系统（卡牌定义与使用逻辑）
├── character/       # 角色系统（玩家/怪物基类）
│   └── monster/     # 具体怪物实现
├── core/            # 核心类型、枚举、前向声明、查询表单
├── event/           # EventBus 事件总线
├── flow/            # CombatFlow 战斗流程状态机
├── gamestate/       # GameState 游戏状态容器
├── intent/          # Intent 意图系统（AI 决策）
│   └── brains/      # 具体 Brain 实现
├── power/           # Power 状态效果系统
├── relic/           # Relic 遗物系统
├── rules/           # 基础规则
├── system/          # ActionSystem 执行器、DeckSystem
├── test/            # 单元测试
└── utils/           # Logger 工具
```

### 1.2 各模块职责

| 模块 | 职责 | 关键文件 |
|------|------|----------|
| **GameState** | 纯数据容器：实体、EventBus、ActionQueue、牌堆 | [GameState.h](file:///j:\学习\项目\STS_CPP\src\gamestate\GameState.h) |
| **CombatFlow** | 战斗状态机：管理阶段跃迁、发布宏观事件 | [CombatFlow.h](file:///j:\学习\项目\STS_CPP\src\flow\CombatFlow.h) |
| **ActionSystem** | 动作执行器：驱动 Action 队列流转 | [ActionSystem.h](file:///j:\学习\项目\STS_CPP\src\system\ActionSystem.h) |
| **EventBus** | 事件总线：订阅/发布模式，自动清理僵尸监听者 | [EventBus.h](file:///j:\学习\项目\STS_CPP\src\event\EventBus.h) |
| **Character** | 实体基类：属性计算、Power/Relic 管理 | [Character.h](file:///j:\学习\项目\STS_CPP\src\character\Character.h) |
| **AbstractAction** | 动作基类：所有状态变更的原子单元 | [AbstractAction.h](file:///j:\学习\项目\STS_CPP\src\action\AbstractAction.h) |
| **AbstractCard** | 卡牌基类：提供 use() 接口 | [AbstractCard.h](file:///j:\学习\项目\STS_CPP\src\card\AbstractCard.h) |
| **AbstractPower** | 状态效果基类：四阶段伤害计算管线 | [AbstractPower.h](file:///j:\学习\项目\STS_CPP\src\power\AbstractPower.h) |
| **AbstractRelic** | 遗物基类：EventBus + Query Pipeline 双路线 | [AbstractRelic.h](file:///j:\学习\项目\STS_CPP\src\relic\AbstractRelic.h) |
| **IntentBrain** | AI 决策策略接口 | [IntentBrain.h](file:///j:\学习\项目\STS_CPP\src\intent\IntentBrain.h) |

---

## 2. 核心设计模式

### 2.1 Action 队列模式（核心）

**设计原则**：所有游戏状态变更必须通过 Action 队列，禁止直接修改实体属性。

```cpp
// AbstractAction.h - 动作基类
class AbstractAction {
public:
    virtual ~AbstractAction() = default;
    virtual bool update(GameState& state) = 0;  // 返回 true 表示完成
};
```

**执行流程**：
1. CombatFlow 推动状态跃迁
2. 各种来源（Card::use()、Brain::decide()）向 `actionQueue` 添加 Action
3. ActionSystem::executeUntilBlocked() 循环弹出并执行 Action
4. 每个 Action 执行完毕后进行 SBA 全局巡视

**Action 类型**（[Actions.h](file:///j:\学习\项目\STS_CPP\src\action\Actions.h)）：
- `DamageAction` - 伤害动作（受护甲/易伤/虚弱影响）
- `LoseHpAction` - 直接扣血（无视护甲，如中毒）
- `GainBlockAction` - 获得格挡
- `ApplyPowerAction` - 施加状态
- `DrawCardsAction` / `DiscardHandAction` / `ShuffleDiscardIntoDrawAction` - 牌库操作
- `RequestCardSelectionAction` - 选牌请求（阻塞引擎）
- `UseCardAction` - 卡牌善后处理
- `RandomDamageAction` - 随机目标伤害（复合 Action 模式）

**Action Deriving Action 模式**：

`RandomDamageAction` 代表了一种复合 Action 模式：

```
RandomDamageAction::update()
    │
    ├─ 使用 state.rng.combatRng 随机选择目标
    │
    └─ 创建 DamageAction 委托执行
            ↓
        ActionQueue.add(DamageAction)  // 子 Action 入队

架构意义：
- 父 Action 在同一帧内完成（返回 true）
- 子 Action 由 ActionSystem 后续执行
- 这与 RequestCardSelectionAction 的阻塞模式形成对比
```

### 2.2 EventBus 事件总线模式

**设计原则**：[EventBus.h](file:///j:\学习\项目\STS_CPP\src\event\EventBus.h) 采用 Erase-Remove Idiom + weak_ptr 实现自清理。

```cpp
class EventBus {
    // 回调返回 false 自动移除（处理僵尸监听者）
    void subscribe(EventType type, std::function<bool(GameState&, void*)> callback);
    void publish(EventType type, GameState& state, void* context = nullptr);
};
```

**事件分类**（[Types.h#L38-L68](file:///j:\学习\项目\STS_CPP\src\core\Types.h#L38-L68)）：

| 类型 | 事件 | 触发者 |
|------|------|--------|
| **宏观阶段** | `PHASE_*` | CombatFlow 状态机 |
| **微观触发** | `ON_CARD_PLAYING/ON_CARD_PLAYED/ON_ATTACK/ON_ATTACKED...` | Action 执行时 |
| **牌库流转** | `ON_SHUFFLE/ON_CARD_DRAWN/ON_CARD_DISCARDED...` | DeckSystem |

### 2.3 Intent 策略模式

**设计原则**：[IntentBrain.h](file:///j:\学习\项目\STS_CPP\src\intent\IntentBrain.h) 将怪物 AI 决策抽象为策略接口。

```cpp
class IntentBrain {
public:
    virtual Intent decide(GameState& state, Monster* owner) = 0;
    virtual void initializeStats(int ascensionLevel);
    virtual void reset();
};
```

**实现类**：
- `FixedBrain` - 固定意图循环
- `RandomBrain` - 随机选择
- `AdaptiveBrain` - 自适应（根据血量调整）
- `JawWormBrain` - 锯颚虫专属行为

### 2.4 Power/Relic 系统

**设计原则**：
- **Power** 是纯粹的无状态计算器，只读面板属性
- **Relic** 通过 EventBus 和 Query Pipeline 双路线影响游戏

**四阶段伤害计算管线**（[AbstractPower.h#L63-L91](file:///j:\学习\项目\STS_CPP\src\power\AbstractPower.h#L63-L91)）：

```
base_damage
    │
    ├─[阶段1] atDamageGive()      攻击者基础修饰 (力量、虚弱)
    │
    ├─[阶段2] atDamageReceive()   防御者基础修饰 (易伤)
    │         ↓ VulnerableMultiplierQuery (遗物填表)
    │
    ├─[阶段3] atDamageFinalGive() 攻击者最终修饰 (笔尖)
    │
    └─[阶段4] atDamageFinalReceive() 防御者最终修饰 (无实体)
              ↓ onActualHpLoss() (鸟居、钨合金棍)
```

**查询表单系统**（[Queries.h](file:///j:\学习\项目\STS_CPP\src\core\Queries.h)）：
- `VulnerableMultiplierQuery` - 易伤倍率查询
- `WeakMultiplierQuery` - 虚弱倍率查询

---

## 3. 模块间依赖关系

### 3.1 模块引用图

```
                    ┌─────────────┐
                    │  GameState  │  (纯数据容器)
                    └──────┬──────┘
                           │
           ┌───────────────┼───────────────┐
           │               │               │
           ▼               ▼               ▼
    ┌─────────────┐  ┌───────────┐  ┌──────────────┐
    │  EventBus   │  │  Action    │  │  CombatFlow  │
    │  (事件总线)  │  │  Queue     │  │  (状态机)    │
    └──────┬──────┘  └───────────┘  └──────┬───────┘
           │                              │
           ▼                              ▼
    ┌─────────────┐              ┌───────────────┐
    │ Power/Relic  │              │ ActionSystem  │
    │ (订阅事件)   │              │ (执行器)      │
    └─────────────┘              └───────┬───────┘
                                          │
              ┌───────────────────────────┼───────────────────────────┐
              │                           │                           │
              ▼                           ▼                           ▼
       ┌─────────────┐            ┌──────────────┐            ┌──────────────┐
       │  Character  │            │ AbstractCard │            │ IntentBrain   │
       │  (实体基类)  │            │  (卡牌基类)  │            │  (AI策略)     │
       └──────┬──────┘            └──────┬───────┘            └───────────────┘
              │                          │
              ▼                          ▼
       ┌─────────────┐            ┌──────────────┐
       │ Player/     │            │ Card::use()  │
       │ Monster     │            │ 添加Action   │
       └─────────────┘            └──────────────┘
```

### 3.2 数据流向

```
1. 玩家出牌：
   PlayerAction → Card::use() → ActionQueue.add(Action)
                                    ↓
                            ActionSystem::executeUntilBlocked()
                                    ↓
                            AbstractAction::update() → EventBus.publish()
                                    ↓
                            Character::takeDamage() → Power/Relic 修饰

2. 怪物回合：
   CombatFlow → Monster::rollIntent() → IntentBrain::decide()
                                    ↓
                            ActionQueue.add(MonsterTakeTurnAction)
                                    ↓
                            Monster::takeTurn() → ActionQueue.add(DamageAction)
```

---

## 4. 接口设计规范

### 4.1 Character 类接口

[Character.h](file:///j:\学习\项目\STS_CPP\src\character\Character.h) 定义：

```cpp
class Character {
    // 纯计算接口（不修改状态）
    int calculateFinalDamage(int base_damage, Character* source, DamageType type) const;
    int calculateFinalBlock(int base_block) const;

    // 执行接口（修改状态）
    DamageResult takeDamage(int damage, DamageType type);  // 先扣护甲再扣血
    int loseHp(int amount);                                  // 无视护甲
    int addBlockFinal(int amount);                           // 增加格挡

    // Power 管理（封装 powers 数组）
    bool addPower(std::shared_ptr<AbstractPower> power);
    void removePower(std::shared_ptr<AbstractPower> power);
    bool hasPower(const std::string& powerName) const;
    std::shared_ptr<AbstractPower> getPower(const std::string& powerName) const;

    // Relic 管理（封装 relics 数组）
    void addRelic(std::shared_ptr<AbstractRelic> relic, GameState& state);
    void removeRelic(std::shared_ptr<AbstractRelic> relic, GameState& state);

    // 查询表单处理
    void processQuery(VulnerableMultiplierQuery& query);
    void processQuery(WeakMultiplierQuery& query);
};
```

### 4.2 AbstractAction 接口

[AbstractAction.h](file:///j:\学习\项目\STS_CPP\src\action\AbstractAction.h) 定义：

```cpp
class AbstractAction {
public:
    virtual ~AbstractAction() = default;
    virtual bool update(GameState& state) = 0;  // 返回 true = 完成，false = 阻塞
};
```

**设计铁律**：
- Action 对象必须携带完整上下文（source、target、amount 等）
- 操作实体前必须检查 `isDead()`
- 使用 `std::unique_ptr` 管理生命周期
- **委托模式约束**：若 Action 内部创建子 Action，父 Action 应在当帧完成（返回 true），子 Action 由 ActionSystem 驱动执行

### 4.3 AbstractCard 接口

[AbstractCard.h](file:///j:\学习\项目\STS_CPP\src\card\AbstractCard.h) 定义：

```cpp
class AbstractCard {
public:
    std::string id;
    int cost;           // -1 表示 X 费牌
    CardType type;
    CardTarget targetType;  // 目标类型（敌方全体/单目标/自身/无目标）
    int energyOnUse;    // X 费牌打出时的费用
    bool isExhaust;     // 是否消耗

    virtual void use(GameState& state, std::shared_ptr<Character> target) = 0;
};
```

**CardTarget 枚举值**（[Types.h](file:///j:\学习\项目\STS_CPP\src\core\Types.h)）：

| 枚举值 | 含义 | 出牌路由 |
|--------|------|----------|
| `ENEMY` | 敌方单体 | `PlayerActions::playAttackCard()` |
| `ALL_ENEMY` | 敌方全体 | `PlayerActions::playAttackCard()` |
| `SELF` | 自身 | `PlayerActions::playSkillCard()` |
| `NONE` | 无目标 | `PlayerActions::playPowerCard()` |
| `RANDOM` | 随机敌方 | 由卡牌 use() 内部通过 RandomDamageAction 或 LambdaAction 处理 |

**PlayerActions 路由校验**：卡牌使用时根据 `targetType` 分发到对应的 PlayerActions 方法。若目标类型与卡牌类型不匹配（如技能牌 targetType=ENEMY 但指向了错误目标），路由层应抛出 `std::invalid_argument`。

---

## 5. 核心文件分析

### 5.1 GameState.h - 游戏状态管理

[GameState.h](file:///j:\学习\项目\STS_CPP\src\gamestate\GameState.h)

**设计原则**：纯数据容器（Anemic Domain Model），禁止包含业务逻辑。

**核心组成**：
- 实体指针：`player`、`monsters`
- 事件总线：`eventBus`
- 随机数管理器：`rng`（4 个隔离 RNG）
- 牌堆系统：`drawPile`、`hand`、`discardPile`、`exhaustPile`、`limbo`
- 动作队列：`actionQueue`（通过 `addAction/addActionToFront` 访问）
- 选牌上下文：`selectionCtx`（`std::optional`）

**RandomManager 隔离策略**：
```cpp
struct RandomManager {
    std::mt19937 shuffleRng;     // 洗牌专用
    std::mt19937 monsterRng;      // 怪物意图生成专用
    std::mt19937 combatRng;       // 战斗内随机效果
    std::mt19937 mapAndDropRng;   // 局外大盘掉落
};
```

### 5.2 CombatFlow.cpp - 战斗流程控制

[CombatFlow.cpp](file:///j:\学习\项目\STS_CPP\src\flow\CombatFlow.cpp)

**状态跃迁链**：
```
BATTLE_START → ROUND_START → PLAYER_TURN_START → PLAYER_ACTION
                                          ↓
                              PLAYER_TURN_END → MONSTER_TURN_START
                                                      ↓
                              MONSTER_TURN → MONSTER_TURN_END → ROUND_END
                                                                      ↓
                                                              ROUND_START (循环)
                                                                      ↓
                                                              BATTLE_END
```

**铁律**：
- CombatFlow 绝对不知道具体的 Action 类
- 只负责推动时间流逝和发布广播
- 所有动作队列执行由 ActionSystem 统一负责

### 5.3 ActionSystem.h - Action 执行系统

[ActionSystem.h](file:///j:\学习\项目\STS_CPP\src\system\ActionSystem.h)

**核心驱动器**：
```cpp
static void executeUntilBlocked(GameState& state, CombatFlow& flow) {
    int loopCount = 0;

    while (state.currentPhase == StatePhase::PLAYING_CARD) {
        if (++loopCount > 1000) {
            break;  // 防死锁看门狗
        }

        if (!state.currentAction) {
            if (state.actionQueue.empty()) {
                break;
            }
            state.currentAction = std::move(state.actionQueue.front());
            state.actionQueue.pop_front();
        }

        bool isDone = state.currentAction->update(state);

        if (isDone) {
            state.currentAction.reset();
            flow.sbaGlobalCheck(state);      // SBA 全局巡视
            flow.checkBattleEndCondition(state);
        } else {
            break;  // currentAction 返回 false → 阻塞等待
        }
    }
}
```

**currentAction 模式**：
- `state.currentAction` 持有当前正在执行的 Action 指针
- 当 Action::update() 返回 true（完成）时，重置 currentAction，然后执行 SBA 全局巡视
- 当 Action::update() 返回 false（阻塞）时，保留 currentAction，退出循环等待下一帧继续
- 下次调用 executeUntilBlocked 时，若 currentAction 存在则直接继续执行

**addActionToFront 工作原理**：
```cpp
// GameState.h
void addActionToFront(std::unique_ptr<AbstractAction> action) {
    actionQueue.push_front(std::move(action));
}
```
- 直接将 Action 插入队列头部（O(1)）
- 插入后，下一轮 `executeUntilBlocked` 会优先执行该 Action
- **注意**：不会影响正在执行的 `currentAction`（如果有的话）

**防死锁看门狗机制**：
- 1000 次循环上限，防止极端情况下无限循环
- 触发后跳出循环，等待下一帧继续（由外部回合逻辑驱动）

### 5.4 EventBus.h - 事件总线

[EventBus.h](file:///j:\学习\项目\STS_CPP\src\event\EventBus.h)

**特性**：
- 使用 Erase-Remove Idiom 自动清理僵尸监听者
- 配合 `weak_ptr` 实现安全的生命周期管理
- 共 23 种事件类型（见 [Types.h#L38-L68](file:///j:\学习\项目\STS_CPP\src\core\Types.h#L38-L68)）

### 5.5 Character.h - 角色基类

[Character.h](file:///j:\学习\项目\STS_CPP\src\character\Character.h)

**Player 子类**：扩展 `energy` 管理（`spendEnergy/gainEnergy/resetEnergy`）

**Monster 子类**：扩展 `IntentBrain` 管理（`rollIntent/decide/takeTurn`）

---

## 6. 关键设计铁律汇总

| 铁律 | 描述 |
|------|------|
| **Action 队列中心化** | 所有状态变更必须通过 Action 队列，禁止直接修改实体属性 |
| **GameState 纯数据** | GameState 禁止包含任何业务逻辑 |
| **CombatFlow 瞎子广播员** | 不知道具体 Action，只管状态跃迁和事件发布 |
| **ActionSystem 执行器** | 所有业务逻辑由 ActionSystem 执行 |
| **零全局 RNG** | 所有随机操作必须使用 `rng` 中的隔离 RNG |
| **内存安全** | 禁止 raw new/delete，使用智能指针；操作实体前检查 `isDead()` |
| **Erase-Remove 自清理** | EventBus 回调返回 false 自动移除，配合 weak_ptr 使用 |
| **四阶段伤害管线** | Power/Relic 通过 `atDamageGive/Receive/FinalGive/FinalReceive` 修饰 |

---

## 7. 防御性编程规范

### 7.1 指针安全

| 检查项 | 要求 |
|--------|------|
| 解引用前 | 必须检查 `if (!ptr)` |
| 容器访问前 | 必须检查 `.empty()` 和边界 |
| 操作实体前 | 必须检查 `isDead()` |
| shared_ptr 提取裸指针 | 使用 `get()` 后立即检查 |

### 7.2 内存安全

| 检查项 | 要求 |
|--------|------|
| new/delete | 禁止使用 |
| 智能指针 | 强制使用 `std::shared_ptr` / `std::unique_ptr` |
| weak_ptr | 用于可选引用，避免循环引用 |

### 7.3 异常安全

| 检查项 | 要求 |
|--------|------|
| switch | 必须带 `default` 分支 |
| 策略查找失败 | 返回绝对安全的默认值 |
| 除零检查 | 严密拦截除零、随机数区间反转等 UB |

### 7.4 LambdaAction 闭包规范

- 严禁使用引用捕获 `[&]`
- 必须使用值捕获 `[=]` 或显式值捕获（如 `[damage, source]`）
- 违反此规范将导致悬空引用，触发未定义行为

---

## 8. 代码审查清单

### 8.1 Action 队列合规性

- [ ] 所有状态变更是否通过 Action 队列
- [ ] 是否直接修改了实体属性（禁止）
- [ ] Action 是否正确使用智能指针

### 8.2 生命周期管理

- [ ] 操作实体前是否检查 `isDead()`
- [ ] EventBus 回调是否使用 `weak_ptr`
- [ ] 死亡实体是否调用 `clearPowers(state)`

### 8.3 随机数使用

- [ ] 是否使用 `state.rng` 中的 RNG
- [ ] 是否使用了正确的 RNG 子类（shuffle/combat/monster）

### 8.4 防御性检查

- [ ] 指针解引用前是否为空检查
- [ ] 容器访问前是否检查边界
- [ ] switch 是否包含 default 分支

---

*文档版本：1.0*
*最后更新：2026-03-20*
