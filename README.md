# STS_CPP

STS_CPP 是一个使用 C++17 编写、面向强化学习训练的类《杀戮尖塔》无头战斗引擎原型。项目重点不在图形界面，而在复刻卡牌战斗、怪物意图、状态效果、遗物、药水、地图生成、确定性随机数与强化学习训练接口等核心系统。

项目目前还没有完成完整游戏流程，现阶段只实现了简单战斗场景的训练与验证：创建玩家、大鄂虫与两个固定行为怪物，构建示例牌组，然后通过简单 AI 或 RL 环境推动战斗。项目还提供 Catch2 测试套件，以及可选的 pybind11 / Gymnasium 风格 RL 环境。

## 功能概览

- 三层游戏状态架构：`GameEngine`、`RunState`、`CombatState`
- Action 队列：所有战斗状态变更通过 `AbstractAction` 派生动作统一执行
- CombatFlow 状态机：驱动战斗阶段、回合推进和宏观事件
- EventBus 事件总线：支持遗物、状态、规则订阅事件并自动清理失效监听者
- 卡牌系统：攻击牌、技能牌、X 费牌、随机目标牌、选牌动作
- 怪物意图系统：固定、随机、自适应和大鄂虫专属 AI
- Power 系统：易伤、中毒、力量、敏捷等状态效果
- Relic 系统：金刚杵、化学物 X、鸟居、钨合金棍、符文圆顶等遗物示例
- Potion 系统：药水目标校验、使用后移除、与 Action 队列集成
- 地图系统：地图拓扑生成、房间类型分配、ANSI ASCII 渲染
- 确定性 RNG：RunState 与 CombatState 隔离随机数，支持复现实验
- RL 训练环境：`sts_env` Python 扩展、Gymnasium 包装器、MaskablePPO 训练脚本
- Catch2 测试：覆盖伤害管线、事件总线、ActionManager、牌堆、地图、怪物、药水、确定性等模块

## 技术栈

| 类别 | 技术 |
| --- | --- |
| 语言 | C++17 |
| 构建 | CMake 3.10+ |
| 推荐编译器 | MinGW 或 MSVC |
| 测试 | Catch2 3.4.0 amalgamated，本地 vendored 于 `third_party/` |
| Python 绑定 | pybind11，可选 |
| RL 训练 | Gymnasium、stable-baselines3、sb3-contrib、NumPy |

## 目录结构

```text
.
├── CMakeLists.txt                 # 主构建脚本，包含游戏、测试、可选 RL 模块
├── BUILD_GUIDE.md                 # 更完整的编译与常见错误说明
├── docs/
│   ├── ARCHITECTURE.md            # 架构设计文档
│   ├── API_DICTIONARY.md          # 引擎 API / Hook / 枚举速查
│   └── 地图生成系统详细分析.md     # 地图生成与房间分配规则分析
├── python/
│   ├── sts_gym_wrapper.py         # Gymnasium 环境包装
│   ├── test_env.py                # Python 侧环境测试
│   └── train_jawworm.py           # MaskablePPO 训练脚本
├── src/
│   ├── action/                    # Action 队列与所有原子动作
│   ├── card/                      # 卡牌基类和具体卡牌
│   ├── character/                 # Character / Player / Monster / 大鄂虫（JawWorm）
│   ├── core/                      # 核心枚举、查询结构、随机数管理
│   ├── engine/                    # GameEngine 顶层入口
│   ├── event/                     # EventBus
│   ├── flow/                      # CombatFlow 战斗状态机
│   ├── gamestate/                 # 旧 GameState，保留兼容，已被 CombatState 替代
│   ├── intent/                    # 怪物意图与 Brain 策略
│   ├── map/                       # 地图生成与渲染
│   ├── potion/                    # 药水系统
│   ├── power/                     # 状态效果系统
│   ├── relic/                     # 遗物系统
│   ├── rl/                        # pybind11 RL 环境
│   ├── rules/                     # 基础战斗规则
│   ├── state/                     # RunState / CombatState
│   ├── system/                    # DeckSystem，旧 ActionSystem
│   ├── test/                      # Catch2 单元与集成测试
│   └── utils/                     # Logger
└── third_party/                   # Catch2 amalgamated 源码
```

`build/`、`cmake_build*/`、IDE 目录、二进制和日志文件是生成物，不应作为源代码提交。

## 快速开始

### 1. 配置

```powershell
cmake -B cmake_build -S . -G "MinGW Makefiles"
```

如果使用 Visual Studio / MSVC，可以省略 `-G "MinGW Makefiles"`，或按本机安装的生成器调整。

### 2. 编译

```powershell
cmake --build cmake_build
```

默认会构建：

- `STS_Game`：战斗演示程序
- `STS_Tests`：Catch2 测试程序

### 3. 运行演示

```powershell
.\cmake_build\bin\STS_Game.exe
```

演示程序会：

1. 使用固定种子创建一局游戏。
2. 构建玩家牌组：`Strike`、`Deadly Poison`、`Whirlwind`、`Shuriken`。
3. 创建大鄂虫和两个 FixedBrain 怪物。
4. 注册基础规则。
5. 由简单 AI 自动出牌，直到胜利、失败或达到最大回合数。

### 4. 运行测试

```powershell
ctest --test-dir cmake_build --output-on-failure
```

也可以直接运行测试二进制：

```powershell
.\cmake_build\bin\STS_Tests.exe
```

## 核心架构

### GameEngine

`GameEngine` 是顶层聚合对象，持有：

- `runState`：整局游戏的持久状态
- `combatState`：当前战斗状态，不在战斗中时为 `nullptr`
- `actionManager`：全局动作队列执行器
- `eventBus`：全局事件总线

它不是单例，可以同时创建多个实例，便于测试和 RL 并行训练。

### RunState

`RunState` 表示一整局游戏的持久层：

- 玩家实体
- 金币、钥匙、当前 Act / 楼层
- master deck
- 遗物与药水
- 地图、奖励、掉落等战斗外 RNG

战斗结束后仍需保留的数据应放在这里。

### CombatState

`CombatState` 表示一场战斗的易失层：

- 玩家引用与怪物列表
- 回合计数、死亡标记、当前玩家/怪物回合状态
- 抽牌堆、手牌、弃牌堆、消耗堆、limbo
- 战斗 RNG
- 选牌上下文 `CardSelectionContext`

它是纯数据容器，业务逻辑应放在 Action、CombatFlow、DeckSystem、PlayerActions 等模块中。

### CombatFlow

`CombatFlow` 是战斗流程状态机，阶段顺序为：

```text
BATTLE_START
  -> ROUND_START
  -> PLAYER_TURN_START
  -> PLAYER_ACTION
  -> PLAYER_TURN_END
  -> MONSTER_TURN_START
  -> MONSTER_TURN
  -> MONSTER_TURN_END
  -> ROUND_END
  -> ROUND_START ...
  -> BATTLE_END
```

CombatFlow 只负责推进时间、发布阶段事件、执行 SBA 全局检查和判定战斗结束。具体伤害、抽牌、弃牌、施加状态等行为由 Action 队列完成。

### Action 队列

所有状态变更都应通过 `AbstractAction` 派生类进入 `ActionManager`：

```cpp
class AbstractAction {
public:
    virtual ~AbstractAction() = default;
    virtual bool update(GameEngine& engine) = 0;
};
```

`update()` 返回值语义：

- `true`：动作完成，ActionManager 继续执行后续动作
- `false`：动作阻塞，保留当前动作，等待下一次推进

典型动作包括：

- `DamageAction`
- `LoseHpAction`
- `GainBlockAction`
- `ApplyPowerAction`
- `ReducePowerAction`
- `DrawCardsAction`
- `DiscardHandAction`
- `ShuffleDiscardIntoDrawAction`
- `RequestCardSelectionAction`
- `UseCardAction`
- `RandomDamageAction`
- `MonsterTakeTurnAction`
- `RollAllMonsterIntentsAction`
- `LambdaAction`

### EventBus

事件总线支持宏观阶段事件、微观战斗触发和牌库流转事件。订阅回调返回 `false` 时会自动移除，适合与 `weak_ptr` 配合处理生命周期。

事件类型定义在 `src/core/Types.h`，主要包括：

- 阶段事件：`PHASE_BATTLE_START`、`PHASE_PLAYER_TURN_START`、`PHASE_ROUND_END` 等
- 战斗事件：`ON_CARD_PLAYING`、`ON_CARD_PLAYED`、`ON_ATTACK`、`ON_ATTACKED`、`ON_HP_LOST` 等
- 牌库事件：`ON_SHUFFLE`、`ON_CARD_DRAWN`、`ON_CARD_DISCARDED`、`ON_CARD_EXHAUSTED`
- 药水事件：`ON_POTION_USED`

### 伤害与格挡管线

伤害计算使用多阶段修饰：

```text
base damage
  -> atDamageGive            # 攻击者基础修饰，例如力量、虚弱
  -> atDamageReceive         # 防御者基础修饰，例如易伤
  -> atDamageFinalGive       # 攻击者最终修饰
  -> atDamageFinalReceive    # 防御者最终修饰
  -> 护甲结算
  -> onActualHpLoss / modifyHpLoss
```

遗物可通过 Query Pipeline 修改易伤、虚弱等倍率，也可通过最终扣血钩子实现鸟居、钨合金棍等效果。

## 已实现内容速查

### 卡牌

| 卡牌 | 类型 | 目标 | 说明 |
| --- | --- | --- | --- |
| `Strike` | Attack | 单体敌人 | 基础攻击 |
| `Deadly Poison` | Skill | 单体敌人 | 施加中毒 |
| `Whirlwind` | Attack | 全体敌人 | X 费 AOE |
| `Shuriken` | Attack | 随机敌人 | 随机目标伤害示例 |
| `Pain` | Attack | 单体敌人 | 高费攻击示例 |
| `Defend` | Skill | 自身 | 获得格挡 |

### 状态效果

| Power | 类型 | 说明 |
| --- | --- | --- |
| `VulnerablePower` | Debuff | 易伤，提高受到的攻击伤害 |
| `PoisonPower` | Debuff | 中毒，回合开始掉血并递减 |
| `StrengthPower` | Buff | 力量，提高攻击伤害 |
| `AgilityPower` | Buff | 敏捷，提高格挡 |

### 遗物

| Relic | 说明 |
| --- | --- |
| `CustomVajraRelic` | 魔改金刚杵，攻击牌触发额外效果 |
| `ChemicalXRelic` | 化学物 X，增强 X 费牌 |
| `ToriiRelic` | 鸟居，小额实际伤害改为 1 |
| `TungstenRodRelic` | 钨合金棍，所有掉血减少 1 |
| `RunicDomeRelic` | 符文圆顶，隐藏敌人意图 |

### 药水

| Potion | 目标 | 说明 |
| --- | --- | --- |
| `StrengthPotion` | 自身 | 使用后通过 Action 队列获得力量 |

### 怪物与 AI

| 类 | 说明 |
| --- | --- |
| `Monster` | 通用怪物类 |
| `JawWorm` | 大鄂虫实现，支持进阶等级缩放 |
| `FixedBrain` | 固定意图循环 |
| `RandomBrain` | 随机意图 |
| `AdaptiveBrain` | 根据状态自适应决策 |
| `JawWormBrain` | 大鄂虫专属 move history 决策 |

## 地图系统

地图模块位于 `src/map/`：

- `MapGenerator`：生成地图节点、路径拓扑并分配房间类型
- `MapRenderer`：用 ANSI ASCII 方式渲染地图
- `MapGeneratorParams`：配置高度、宽度、路径密度、祖先间距、进阶等级和种子

默认参数：

```cpp
struct MapGeneratorParams {
    int height = 15;
    int width = 7;
    int pathDensity = 6;
    int minAncestorGap = 3;
    int maxAncestorGap = 5;
    int ascensionLevel = 0;
    uint32_t seed = 0;
};
```

地图生成规则的详细推导见 `docs/地图生成系统详细分析.md`。

## RL 训练环境

可选 RL 模块位于 `src/rl/` 和 `python/`。

### 环境接口

C++ 侧 `STSEnv` 提供 Gym 风格接口：

- `reset() -> observation`
- `step(action) -> StepResult`
- `getObservation()`
- `getLegalActionMask()`
- `action_space_size()`
- `observation_space_size()`

默认场景：

- 单怪物：大鄂虫（代码类名 `JawWorm`）
- 默认牌库：`Strike x5`、`Deadly Poison x3`、`Whirlwind x2`、`Shuriken x3`
- Observation：72 维
- Action：11 个，`0` 为结束回合，`1~10` 对应打出手牌槽位

### 编译 RL 模块

先安装 Python 依赖：

```powershell
pip install pybind11 stable-baselines3 sb3-contrib gymnasium numpy
```

配置并编译：

```powershell
cmake -B cmake_build_rl -S . -G "MinGW Makefiles" -DBUILD_RL_ENV=ON
cmake --build cmake_build_rl
```

如果 CMake 找不到 pybind11，可显式传入：

```powershell
cmake -B cmake_build_rl -S . -G "MinGW Makefiles" -DBUILD_RL_ENV=ON -Dpybind11_DIR="$(python -m pybind11 --cmakedir)"
```

验证导入时，确保生成的 `sts_env*.pyd` 所在目录在 Python 搜索路径中：

```powershell
python -c "import sys; sys.path.insert(0, 'cmake_build_rl'); import sts_env; env = sts_env.STSEnv(0); print(len(env.reset()), env.action_space_size())"
```

当前 `python/train_jawworm.py` 默认向 `sys.path` 添加 `../cmake_build/bin`。如果你使用 `cmake_build_rl` 输出目录，需要调整脚本中的路径，或把生成的 `sts_env*.pyd` 放到脚本可导入的位置。

### 训练

```powershell
cd python
python train_jawworm.py
```

训练脚本使用 `MaskablePPO` 和合法动作掩码，默认并行环境数为 4，总训练步数为 1,000,000。

## 测试体系

测试入口为 `src/test/test_main.cpp`，测试文件显式注册在 `CMakeLists.txt` 的 `TEST_SOURCES` 中，CMake 不会自动 glob 新测试文件。

当前测试覆盖重点：

- 伤害与格挡管线
- 易伤、弱化倍率查询和遗物修饰
- ActionManager 队列顺序、阻塞动作和队首插入
- LambdaAction 生命周期安全
- 选牌动作和选牌上下文
- 牌堆抽牌、洗牌、弃牌、手牌上限
- EventBus 发布、订阅和自动移除
- 地图生成确定性与渲染 smoke test
- 怪物行为、大鄂虫进阶缩放和 move history
- 药水使用、目标校验和移除
- CombatFlow 集成流程
- 完整回合和玩家死亡流程
- RNG 确定性与 RunState / CombatState 隔离

新增测试时：

1. 单模块行为放在 `src/test/unit/`。
2. 跨模块流程放在 `src/test/integration/`。
3. 将新 `.cpp` 文件加入 `CMakeLists.txt` 的 `TEST_SOURCES`。
4. 运行 `ctest --test-dir cmake_build --output-on-failure`。

## 开发约定

### 编码风格

- 使用 C++17。
- 非 MSVC 编译使用 `-Wall -Wextra -Wpedantic`。
- MSVC 编译使用 `/W4`。
- 4 空格缩进。
- 类型使用 PascalCase，例如 `GameEngine`。
- 函数和变量使用 camelCase，例如 `startNewRun`、`turnCount`。
- 宏或编译期常量使用 `UPPER_SNAKE_CASE`。
- 文件命名沿用现有风格，例如 `Monster.h` / `Monster.cpp`，测试文件以 `Test*.cpp` 命名。

### 架构约束

- 所有战斗状态变更应通过 Action 队列。
- `CombatState` 和 `RunState` 保持纯数据容器定位。
- 随机行为应使用对应状态中的 `RandomManager`，不要引入全局 RNG。
- Entity 生命周期使用智能指针管理，避免 raw `new` / `delete`。
- 操作角色、怪物、卡牌、药水前检查空指针、死亡状态和容器边界。
- EventBus 订阅尽量使用 `weak_ptr` 或返回 `false` 清理失效监听者。
- `LambdaAction` 不要按引用捕获局部变量，因为 Action 会延迟执行。
- 遗物和 Power 的数值修饰应接入现有 Hook / Query Pipeline，而不是绕过伤害管线。

### 添加新卡牌

1. 在 `src/card/Cards.h` 声明派生自 `CloneableCard<T>` 的卡牌类。
2. 在 `src/card/Cards.cpp` 实现 `use(GameEngine&, std::shared_ptr<Character>)`。
3. 在 `use()` 中向 `engine.actionManager` 添加动作，不直接改 HP、block、power。
4. 根据 `CardType` 和 `CardTarget` 确保 `PlayerActions::playCard` 能正确路由。
5. 添加对应单元测试，必要时补充集成测试。

### 添加新怪物

1. 在 `src/character/monster/` 添加怪物类。
2. 继承 `Monster` 并配置 HP、名称、进阶缩放。
3. 为其设置专属或通用 `IntentBrain`。
4. 在 Brain 中返回 `Intent`，在怪物行动阶段通过 `MonsterTakeTurnAction` 转化为具体动作。
5. 添加意图、行动和进阶缩放测试。

### 添加新遗物

1. 在 `src/relic/Relics.h` 声明派生自 `AbstractRelic` 的类。
2. 对事件型遗物，在 `onEquip()` 中订阅 `engine.eventBus`。
3. 对数值修饰型遗物，优先实现现有 Hook 或 Query 方法。
4. 避免长期持有不安全裸指针。
5. 添加覆盖触发条件、边界值和叠加行为的测试。

### 添加新测试

`CMakeLists.txt` 不自动扫描测试文件。新增测试文件后，必须手动加入：

```cmake
set(TEST_SOURCES
    ...
    src/test/unit/TestYourFeature.cpp
)
```

## 常见问题

### CMake 找不到编译器

确认 MinGW 或 Visual Studio Build Tools 已安装，并且编译器在 PATH 中。MinGW 用户建议使用：

```powershell
cmake -B cmake_build -S . -G "MinGW Makefiles"
```

### CMake 找不到 pybind11

先安装：

```powershell
pip install pybind11
```

然后传入：

```powershell
-Dpybind11_DIR="$(python -m pybind11 --cmakedir)"
```

### Python 无法 import sts_env

确认 `sts_env*.pyd` 已生成，并把它所在目录加入 `sys.path`。MinGW 构建的 `.pyd` 还可能需要同目录可找到 MinGW 运行时 DLL，例如 `libgcc_s_seh-1.dll`、`libstdc++-6.dll`、`libwinpthread-1.dll`。

### 新测试没有运行

检查测试文件是否已加入 `CMakeLists.txt` 的 `TEST_SOURCES`，然后重新配置或重新构建：

```powershell
cmake -B cmake_build -S . -G "MinGW Makefiles"
cmake --build cmake_build
ctest --test-dir cmake_build --output-on-failure
```

### 行为和预期不一致但测试偶发失败

优先检查是否误用了全局随机数、是否从错误状态读取 RNG、是否在 LambdaAction 中按引用捕获了局部变量，或者是否直接修改状态绕过了 Action 队列。

## 参考文档

- `docs/ARCHITECTURE.md`：详细架构设计、模块职责、核心约束
- `docs/API_DICTIONARY.md`：Hook、Action、Event、枚举和资产字典
- `docs/地图生成系统详细分析.md`：地图路径生成与房间分配规则
- `BUILD_GUIDE.md`：构建、运行、RL 编译和常见错误
- `third_party/README.md`：Catch2 第三方依赖说明

## 当前状态说明

这个项目目前更接近引擎与实验平台，而不是完整可发布游戏：

- 没有正式图形界面。
- 主程序是自动战斗演示，不是交互式客户端。
- `src/gamestate/GameState.*` 和 `src/system/ActionSystem.h` 属于遗留接口，主架构已转向 `RunState`、`CombatState` 与 `ActionManager`。
- RL 模块是可选构建项，需要额外 Python 依赖。
