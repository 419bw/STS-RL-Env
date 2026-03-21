# STS_CPP 引擎资产字典 (API Dictionary)

> **版本**: v1.0
> **建立日期**: 2026-03-21
> **来源**: 静态分析 src/ 目录下所有相关源文件

---

## 1. 核心生命周期钩子 (Hooks)

### 1.1 Power 钩子 (AbstractPower)

| 函数签名 | 说明 |
|---------|------|
| `void onApply(GameState&)` | 状态挂载时触发，用于注册事件订阅 |
| `void onRemove(GameState&)` | 状态移除时触发，用于清理事件订阅 |
| `void stackPower(int)` | 状态叠加时调用，默认实现为层数相加（限制在 [-999, 999] 范围内） |
| `float atDamageGive(float damage, DamageType)` | **阶段1**: 攻击者基础修饰（力量加成） |
| `float atDamageReceive(float damage, DamageType, Character* source)` | **阶段2**: 防御者基础修饰（易伤倍率） |
| `float atDamageFinalGive(float damage, DamageType)` | **阶段3**: 攻击者最终修饰（极限乘法） |
| `float atDamageFinalReceive(float damage, DamageType)` | **阶段4**: 防御者最终修饰（强制截断） |
| `float atBlockGive(float block)` | 获得格挡时修饰（敏捷） |
| `float atBlockFinalGive(float block)` | 最终格挡修饰 |
| `int modifyHpLoss(int amount)` | 修改掉血量（无实体限制） |
| `bool canSeeEnemyIntents()` | 视野拦截（致盲状态） |

### 1.2 Relic 钩子 (AbstractRelic)

| 函数签名 | 说明 |
|---------|------|
| `void onEquip(GameState&, Character* target)` | 装备遗物时触发（注册事件订阅） |
| `void onRemove(GameState&)` | 移除遗物时触发（清理事件订阅） |
| `float atDamageGive(float damage, DamageType)` | **阶段1**: 攻击者伤害修饰（纸蛙） |
| `float atDamageReceive(float damage, DamageType)` | **阶段2**: 防御者伤害修饰（奇数蘑菇） |
| `float atDamageFinalGive(float damage, DamageType)` | **阶段3**: 攻击者最终修饰（笔尖） |
| `float atDamageFinalReceive(float damage, DamageType)` | **阶段4**: 防御者最终修饰（无实体） |
| `int onActualHpLoss(int hpLoss, DamageType type)` | 护甲之后实际扣血拦截（鸟居） |
| `float atBlockGive(float block)` | 获得格挡时修饰 |
| `float atBlockFinalGive(float block)` | 最终格挡修饰 |
| `int modifyHpLoss(int amount)` | 所有掉血拦截（钨合金棍） |
| `void onQuery(VulnerableMultiplierQuery&)` | 易伤倍率查询（遗物填表） |
| `void onQuery(WeakMultiplierQuery&)` | 虚弱倍率查询（遗物填表） |
| `bool canSeeEnemyIntents()` | 视野拦截（符文圆顶） |

### 1.3 Card 钩子

| 函数签名 | 说明 |
|---------|------|
| `void use(GameState&, std::shared_ptr<Character> target)` | 打出卡牌的核心接口（纯虚函数） |

### 1.4 Action 钩子

| 函数签名 | 说明 |
|---------|------|
| `bool update(GameState&)` | 动作执行接口，返回 true 表示完成 |

---

## 2. 标准动作库 (Actions)

| Action类名(核心构造参数) | 说明 |
|------------------------|------|
| `DummyAction(std::string name)` | 占位动作，用于状态机流程标记 |
| `RollAllMonsterIntentsAction()` | 刷新所有怪物意图 |
| `MonsterTakeTurnAction(std::shared_ptr<Monster>)` | 怪物回合执行动作 |
| `DamageAction(src, tgt, amount, DamageType)` | 物理伤害动作（走护甲管线） |
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

### 3.3 牌库流转事件 (由 DeckSystem 发布)

| 事件名称 | 参数说明 | 说明 |
|---------|---------|------|
| `ON_SHUFFLE` | - | 洗牌时 |
| `ON_CARD_DRAWN` | `AbstractCard*` | 抽牌时 |
| `ON_CARD_DISCARDED` | `AbstractCard*` | 卡牌进入弃牌堆时 |
| `ON_CARD_EXHAUSTED` | `AbstractCard*` | 卡牌进入消耗堆时 |

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

---

## 5. 枚举速查

| 枚举类型 | 枚举值 |
|---------|-------|
| `CardType` | ATTACK, SKILL, POWER, STATUS, CURSE |
| `CardTarget` | ENEMY, ALL_ENEMY, SELF, NONE, RANDOM |
| `CombatState` | BATTLE_START, ROUND_START, PLAYER_TURN_START, PLAYER_ACTION, PLAYER_TURN_END, MONSTER_TURN_START, MONSTER_TURN, MONSTER_TURN_END, ROUND_END, BATTLE_END |
| `PowerType` | BUFF, DEBUFF |
| `DamageType` | ATTACK, THORNS, HP_LOSS |
| `StatePhase` | PLAYING_CARD, WAITING_FOR_CARD_SELECTION |
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
| `VulnerablePower(int)` | atDamageReceive, onApply |
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
| `ChemicalXRelic` | onEquip |

### 7.3 具体 Card

| 类名 | targetType | 说明 |
|-----|------------|------|
| `StrikeCard()` | ENEMY | 打击：6伤+易伤 |
| `DeadlyPoisonCard()` | ENEMY | 致命毒药：5层中毒 |
| `WhirlwindCard()` | ALL_ENEMY | 旋风斩：X费，对每个怪物造成X次5伤 |
