# STS_CPP 引擎资产字典 (API Dictionary)

> **版本**: v1.0
> **建立日期**: 2026-03-21
> **来源**: 静态分析 src/ 目录下所有相关源文件

---

## 1. 核心生命周期钩子 (Hooks)

### 1.1 Power 钩子 (AbstractPower)

| 函数签名 | 说明 |
|---------|------|
| `void onApply(GameEngine&)` | 状态挂载时触发，用于注册事件订阅 |
| `void onRemove(GameEngine&)` | 状态移除时触发，用于清理事件订阅 |
| `void stackPower(int)` | 状态叠加时调用，默认实现为层数相加（限制在 [-999, 999] 范围内） |
| `float atDamageGive(float damage, DamageType)` | **阶段1**: 攻击者基础修饰（力量加成） |
| `float atDamageReceive(float damage, DamageType, Character* source = nullptr)` | **阶段2**: 防御者基础修饰（易伤倍率） |
| `float atDamageFinalGive(float damage, DamageType)` | **阶段3**: 攻击者最终修饰（极限乘法） |
| `float atDamageFinalReceive(float damage, DamageType)` | **阶段4**: 防御者最终修饰（强制截断） |
| `float atBlockGive(float block)` | 获得格挡时修饰（敏捷） |
| `float atBlockFinalGive(float block)` | 最终格挡修饰 |
| `int modifyHpLoss(int amount) const` | 修改掉血量（无实体限制） |
| `bool canSeeEnemyIntents() const` | 视野拦截（致盲状态） |

### 1.2 Relic 钩子 (AbstractRelic)

| 函数签名 | 说明 |
|---------|------|
| `void onEquip(GameEngine&, Character* target)` | 装备遗物时触发（注册事件订阅，非纯虚，有默认空实现） |
| `void onRemove(GameEngine&)` | 移除遗物时触发（清理事件订阅） |
| `float atDamageGive(float damage, DamageType)` | **阶段1**: 攻击者伤害修饰（纸蛙） |
| `float atDamageReceive(float damage, DamageType)` | **阶段2**: 防御者伤害修饰（奇数蘑菇） |
| `float atDamageFinalGive(float damage, DamageType)` | **阶段3**: 攻击者最终修饰（笔尖） |
| `float atDamageFinalReceive(float damage, DamageType)` | **阶段4**: 防御者最终修饰（无实体） |
| `int onActualHpLoss(int hpLoss, DamageType type)` | 护甲之后实际扣血拦截（鸟居） |
| `float atBlockGive(float block)` | 获得格挡时修饰 |
| `float atBlockFinalGive(float block)` | 最终格挡修饰 |
| `int modifyHpLoss(int amount) const` | 所有掉血拦截（钨合金棍） |
| `void onQuery(VulnerableMultiplierQuery&)` | 易伤倍率查询（遗物填表） |
| `void onQuery(WeakMultiplierQuery&)` | 虚弱倍率查询（遗物填表） |
| `bool canSeeEnemyIntents() const` | 视野拦截（符文圆顶） |

### 1.3 Card 钩子

| 函数签名 | 说明 |
|---------|------|
| `void use(GameEngine&, std::shared_ptr<Character> target)` | 打出卡牌的核心接口（纯虚函数） |

### 1.4 Action 钩子

| 函数签名 | 说明 |
|---------|------|
| `bool update(GameEngine&)` | 动作执行接口，返回 true 表示完成 |

---

## 2. 标准动作库 (Actions)

| Action类名(核心构造参数) | 说明 |
|------------------------|------|
| `DummyAction(std::string name)` | 占位动作，用于状态机流程标记 |
| `RollAllMonsterIntentsAction()` | 刷新所有怪物意图 |
| `MonsterTakeTurnAction(std::shared_ptr<Monster>)` | 怪物回合执行动作 |
| `DamageAction(src, tgt, amount, DamageType)` | 物理伤害动作（走护甲管线） |
| `RandomDamageAction(src, dmg, DamageType)` | 随机攻击动作（乱叉：随机选中一个存活怪物，委托 DamageAction 执行） |
| `LoseHpAction(target, amount)` | 直接扣血动作（无视护甲） |
| `GainBlockAction(target, amount)` | 获得格挡动作 |
| `ApplyPowerAction(src, tgt, power)` | 施加状态效果（含叠加逻辑） |
| `ReducePowerAction(target, power, reduceAmount)` | 减少状态层数（归零时触发移除） |
| `RemoveSpecificPowerAction(target, power)` | 强制移除指定状态 |
| `RequestCardSelectionAction(PileType, SelectionPurpose, min, max)` | 选牌请求（冻结引擎） |
| `SpecificCardExhaustAction(card)` | 消耗指定卡牌 |
| `MoveCardToHandAction(card)` | 将卡牌移入手牌（含爆牌判定） |
| `ResetAllBrainsAction()` | 重置所有怪物大脑 |
| `SpecificCardDiscardAction(card)` | 丢弃指定卡牌 |
| `UseCardAction(card)` | 卡牌使用后善后（判定去向） |
| `DrawCardsAction(amount)` | 抽牌动作（含洗牌/爆牌判定） |
| `ShuffleDiscardIntoDrawAction()` | 弃牌堆洗入抽牌堆 |
| `DiscardHandAction()` | 丢弃所有手牌 |

### 2.X LambdaAction（通用延迟动作）

| 签名 | 说明 |
|------|------|
| `static std::unique_ptr<LambdaAction> make(std::weak_ptr<Character> source, std::function<void(GameEngine&, Character*)> closure)` | 工厂函数，创建带 source 生命周期监听的 LambdaAction |

**闭包类型**：`std::function<void(GameEngine&, Character* source)>`

**执行流程**：
1. `weak_ptr.lock()` 检查 source 是否存活
2. 若 lock 失败或 source->isDead() → return true（跳过执行）
3. 提取 `raw = locked.get()` 传给闭包
4. 调用闭包，闭包内部自行查询 target
5. return true（当帧完成）

**闭包规范**：
- 严禁使用引用捕获 `[&]`，必须使用值捕获 `[=]`

### 2.Y ActionManager 核心接口

| 接口签名 | 说明 |
|---------|------|
| `void addAction(std::unique_ptr<AbstractAction>)` | 添加 Action 到队列尾部 |
| `void addActionToFront(std::unique_ptr<AbstractAction>)` | 添加 Action 到队列头部（O(1) push_front） |
| `void executeUntilBlocked(GameEngine&, CombatFlow&)` | O(1) 执行循环，驱动队列排空 |
| `bool isQueueEmpty() const` | 检查动作队列是否为空 |

**addActionToFront 行为**：
- 直接将 Action 插入队列头部（O(1)）
- 插入后，下一轮 `executeUntilBlocked` 会优先执行该 Action
- **注意**：不会影响正在执行的 `currentAction`（如果有的话）
- **典型用途**：SBA 全局检查插入的 DamageAction、DeathCheckAction 等高优先级动作

**防死锁看门狗**：
- `executeUntilBlocked` 内部含 1000 次循环上限计数器
- 超出上限时强制 break，防止状态机死锁
- 触发条件：极端多的微小 Action 或递归委托链过深

### 2.Z PlayerActions 接口

| 接口 | 说明 |
|------|------|
| `static bool usePotion(GameEngine& engine, CombatFlow& flow, std::shared_ptr<AbstractPotion> potion, std::shared_ptr<Character> target = nullptr)` | 使用药水，目标验证后调用 potion->use()，成功返回 true，失败返回 false |

**返回 false 的情况：**
- 当前阶段不是 PLAYING_CARD
- 不是玩家回合
- 药水不在 engine.potions 中
- ENEMY 类型目标不能是玩家自己 |

---

## 3. 事件总线频道 (EventBus Events)

### 3.1 宏观阶段事件 (由 CombatFlow 状态机发布)

| 事件名称 | 参数说明 | 说明 |
|---------|---------|------|
| `PHASE_BATTLE_START` | - | 战斗开始（洗牌、触发初始遗物） |
| `PHASE_ROUND_START` | - | 轮次开始 |
| `PHASE_PLAYER_TURN_START` | - | 玩家回合开始（重置费用、抽牌） |
| `PHASE_PLAYER_ACTION` | - | 玩家行动阶段 |
| `PHASE_PLAYER_TURN_END` | - | 玩家回合结束 |
| `PHASE_MONSTER_TURN_START` | - | 怪物回合开始 |
| `PHASE_MONSTER_TURN` | - | 怪物行动阶段 |
| `PHASE_MONSTER_TURN_END` | - | 怪物回合结束 |
| `PHASE_ROUND_END` | - | 轮次结束（状态掉层结算） |
| `PHASE_BATTLE_END` | - | 战斗结束 |
| `ON_POTION_USED` | - | 药水被使用时 |

### 3.2 微观触发事件 (由 Actions 发布)

| 事件名称 | 参数说明 | 说明 |
|---------|---------|------|
| `ON_CARD_PLAYING` | `AbstractCard*` | 准备打出牌时 |
| `ON_CARD_PLAYED` | `AbstractCard*` | 打出牌后 |
| `ON_ATTACK` | `DamageContext*` | 攻击时 |
| `ON_ATTACKED` | `DamageContext*` | 被攻击时（肢体接触，唤醒荆棘） |
| `ON_UNBLOCKED_DAMAGE_TAKEN` | `DamageContext*` | 护甲击穿时（唤醒静电释放） |
| `ON_HP_LOST` | `DamageContext*` | 真实掉血时（唤醒撕裂，红骷髅） |
| `ON_TURN_START` | `Character*` | 角色回合开始时 |
| `ON_TURN_END` | `Character*` | 角色回合结束时 |
| `ON_ROUND_END` | - | 整个轮次结束时 |

### 3.3 EventBus 类

| 接口 | 说明 |
|------|------|
| `void subscribe(EventType type, std::function<bool(GameEngine&, void*)> callback)` | 订阅事件，返回 false 的监听者会被移除 |
| `void publish(EventType type, GameEngine& engine, void* context = nullptr)` | 发布事件，自动清理僵尸监听者 |

### 3.4 牌库流转事件 (由 DeckSystem 发布)

| 事件名称 | 参数说明 | 说明 |
|---------|---------|------|
| `ON_SHUFFLE` | - | 洗牌时 |
| `ON_CARD_DRAWN` | `AbstractCard*, GameEngine&` | 抽牌时 |
| `ON_CARD_DISCARDED` | `AbstractCard*, GameEngine&` | 卡牌进入弃牌堆时 |
| `ON_CARD_EXHAUSTED` | `AbstractCard*, GameEngine&` | 卡牌进入消耗堆时 |

---

## 4. 关键数据结构

### 4.1 DamageContext (事件广播用的上下文)

```cpp
struct DamageContext {
    Character* target;      // 受害者
    DamageInfo info;        // 伤害信息
};
struct DamageInfo {
    Character* source;      // 肇事者
    DamageType type;        // 伤害类型
    int amount;             // 具体数值
};
```

### 4.2 CardSelectionContext (选牌上下文)

```cpp
struct CardSelectionContext {
    std::vector<std::shared_ptr<AbstractCard>> choices;  // 可选牌库快照
    SelectionPurpose purpose;                              // 选完后的用途
    int minSelection = 1;                                  // 最少选几张
    int maxSelection = 1;                                  // 最多选几张
};
```

### 4.3 Query 表单 (零开销抽象)

```cpp
struct VulnerableMultiplierQuery {
    Character* source;
    Character* target;
    float multiplier = 1.5f;  // 默认易伤倍率
};
struct WeakMultiplierQuery {
    Character* source;
    Character* target;
    float multiplier = 0.75f;  // 默认虚弱倍率
};
```

### 4.X Intent (怪物意图数据结构)

```cpp
struct Intent {
    IntentType type = IntentType::ATTACK;
    int base_damage = -1;
    int hit_count = 1;
    int effect_value = 0;
    std::weak_ptr<Character> target;  // 弱引用，需使用 lock() 验证
    bool visible = true;
    int move_id = -1;                 // AI历史追踪用
    std::string move_name;

    // 流式接口
    Intent& withMove(int, const std::string&);
    Intent& setVisible(bool);
};
```

**字段说明**：

| 字段 | 类型 | 说明 |
|------|------|------|
| `type` | IntentType | 意图类型（ATTACK/DEFEND/BUFF/DEBUFF等） |
| `base_damage` | int | 基础伤害值 |
| `hit_count` | int | 攻击次数 |
| `effect_value` | int | 效果值（如格挡层数、buff层数） |
| `target` | `weak_ptr<Character>` | **弱引用**，需使用 `lock()` 验证有效性 |
| `visible` | bool | 是否对玩家可见 |
| `move_id` | int | 动作ID（用于AI历史记录追踪） |
| `move_name` | string | 动作名称（调试用） |

**流式接口使用示例**：
```cpp
Intent(IntentType::ATTACK, 12, 1, 0, combat.player)
    .withMove(CHOMP, "Chomp")
    .setVisible(true);
```

---

---

## 5. 枚举速查

| 枚举类型 | 枚举值 |
|---------|-------|
| `CardType` | ATTACK, SKILL, POWER, STATUS, CURSE |
| `CardTarget` | ENEMY, ALL_ENEMY, SELF, NONE, RANDOM |
| `PotionTarget` | ENEMY, ALL_ENEMY, SELF, NONE |
| `IntentType` | ATTACK, DEFEND, BUFF, DEBUFF, ATTACK_DEFEND, ATTACK_DEBUFF, UNKNOWN |
| `BattlePhase` | BATTLE_START, ROUND_START, PLAYER_TURN_START, PLAYER_ACTION, PLAYER_TURN_END, MONSTER_TURN_START, MONSTER_TURN, MONSTER_TURN_END, ROUND_END, BATTLE_END |
| `StatePhase` | BATTLE_START, ROUND_START, PLAYER_TURN_START, PLAYING_CARD, PLAYER_TURN_END, WAITING_FOR_CARD_SELECTION, MONSTER_TURN_START, MONSTER_TURN, MONSTER_TURN_END, ROUND_END, BATTLE_END |
| `EventType` | PHASE_BATTLE_START, PHASE_ROUND_START, PHASE_PLAYER_TURN_START, PHASE_PLAYER_ACTION, PHASE_PLAYER_TURN_END, PHASE_MONSTER_TURN_START, PHASE_MONSTER_TURN, PHASE_MONSTER_TURN_END, PHASE_ROUND_END, PHASE_BATTLE_END, ON_CARD_PLAYING, ON_CARD_PLAYED, ON_ATTACK, ON_ATTACKED, ON_UNBLOCKED_DAMAGE_TAKEN, ON_HP_LOST, ON_TURN_START, ON_TURN_END, ON_ROUND_END, ON_SHUFFLE, ON_CARD_DRAWN, ON_CARD_DISCARDED, ON_CARD_EXHAUSTED, ON_POTION_USED |
| `PowerType` | BUFF, DEBUFF |
| `DamageType` | ATTACK, THORNS, HP_LOSS |
| `SelectionPurpose` | EXHAUST_FROM_HAND, MOVE_TO_HAND, DISCARD_FROM_HAND |
| `PileType` | HAND, DRAW_PILE, DISCARD_PILE, EXHAUST_PILE, LIMBO |

---

## 6. 四阶段伤害计算管线

```
base_damage
    │
    ▼ [阶段1: atDamageGive] 攻击者基础修饰
    │  - Power::atDamageGive (力量 +X)
    │  - Relic::atDamageGive (纸蛙等)
    ▼
    │ [阶段2: atDamageReceive] 防御者基础修饰
    │  - Power::atDamageReceive (易伤 x1.5)
    │  - Relic::atDamageReceive (奇数蘑菇)
    ▼
    │ [阶段3: atDamageFinalGive] 攻击者最终修饰
    │  - Power::atDamageFinalGive (愤怒姿态 x2)
    │  - Relic::atDamageFinalGive (笔尖 x2)
    ▼
    │ [阶段4: atDamageFinalReceive] 防御者最终修饰
    │  - Power::atDamageFinalReceive (无实体 =1)
    │  - Relic::atDamageFinalReceive (鸟居 ≤5时=1)
    ▼
final_damage → takeDamage() → 护甲计算 → onActualHpLoss → loseHp() → modifyHpLoss → 真实扣血
```

---

## 7. 具体实现资产清单

### 7.1 具体 Power

| 类名 | Hook重写 |
|-----|---------|
| `VulnerablePower(int)` | atDamageReceive(= nullptr), onApply |
| `PoisonPower(int)` | onApply |
| `AgilityPower(int)` | atBlockGive |
| `StrengthPower(int)` | atDamageGive |

### 7.2 具体 Relic

| 类名 | Hook重写 |
|-----|---------|
| `ToriiRelic` | onActualHpLoss |
| `TungstenRodRelic` | modifyHpLoss |
| `RunicDomeRelic` | canSeeEnemyIntents |
| `CustomVajraRelic` | onEquip |
| `ChemicalXRelic` | onEquip（订阅 ON_CARD_PLAYING，为 X 费牌额外注入 2 点能量） |

### 7.3 具体 Card

| 类名 | targetType | 说明 |
|-----|------------|------|
| `StrikeCard()` | ENEMY | 打击：6伤 |
| `DeadlyPoisonCard()` | ENEMY | 致命毒药：5层中毒 |
| `WhirlwindCard()` | ALL_ENEMY | 旋风斩：X费，对每个怪物造成X次5伤 |
| `ShurikenCard()` | RANDOM | 造成3次随机目标的3点伤害 |
| `PainCard()` | ENEMY | 痛击：8伤+2层易伤 |

### 7.3 AbstractPotion 基类

| 接口 | 说明 |
|------|------|
| `std::string id` | 药水唯一标识符 |
| `PotionTarget targetType` | 目标类型 (ENEMY/ALL_ENEMY/SELF/NONE) |
| `virtual void use(GameEngine& engine, std::shared_ptr<Character> target) = 0` | 使用药水，纯虚方法 |

**子类需实现的接口：**
- `use()`: 具体药水的效果实现

### 7.4 具体 Potion

| 类名 | targetType | 说明 |
|-----|------------|------|
| `StrengthPotion()` | SELF | 使用后给角色添加 3 层 StrengthPower |
