# STS_CPP 编译指南

## CMake 编译（推荐）

### 1. 配置项目

```powershell
cmake -B cmake_build -S . -G "MinGW Makefiles"
```

### 2. 编译

```powershell
cmake --build cmake_build
```

### 3. 运行

```powershell
.\cmake_build\bin\STS_Game.exe
```

---

## g++ 手动编译

### 1. 编译所有源文件

```powershell
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\main.cpp" -o "j:\学习\项目\STS_CPP\build\main.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\action\Actions.cpp" -o "j:\学习\项目\STS_CPP\build\Actions.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\action\PlayerActions.cpp" -o "j:\学习\项目\STS_CPP\build\PlayerActions.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\action\LambdaAction.cpp" -o "j:\学习\项目\STS_CPP\build\LambdaAction.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\action\ActionManager.cpp" -o "j:\学习\项目\STS_CPP\build\ActionManager.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\character\Character.cpp" -o "j:\学习\项目\STS_CPP\build\Character.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\character\Monster.cpp" -o "j:\学习\项目\STS_CPP\build\Monster.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\character\monster\JawWorm.cpp" -o "j:\学习\项目\STS_CPP\build\JawWorm.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\card\Cards.cpp" -o "j:\学习\项目\STS_CPP\build\Cards.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\relic\Relics.cpp" -o "j:\学习\项目\STS_CPP\build\Relics.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\relic\AbstractRelic.cpp" -o "j:\学习\项目\STS_CPP\build\AbstractRelic.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\gamestate\GameState.cpp" -o "j:\学习\项目\STS_CPP\build\GameState.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\engine\GameEngine.cpp" -o "j:\学习\项目\STS_CPP\build\GameEngine.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\state\RunState.cpp" -o "j:\学习\项目\STS_CPP\build\RunState.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\state\CombatState.cpp" -o "j:\学习\项目\STS_CPP\build\CombatState.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\system\DeckSystem.cpp" -o "j:\学习\项目\STS_CPP\build\DeckSystem.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\flow\CombatFlow.cpp" -o "j:\学习\项目\STS_CPP\build\CombatFlow.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\rules\BasicRules.cpp" -o "j:\学习\项目\STS_CPP\build\BasicRules.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\power\Powers.cpp" -o "j:\学习\项目\STS_CPP\build\Powers.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\potion\Potions.cpp" -o "j:\学习\项目\STS_CPP\build\Potions.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\intent\Intent.cpp" -o "j:\学习\项目\STS_CPP\build\Intent.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\intent\IntentBrain.cpp" -o "j:\学习\项目\STS_CPP\build\IntentBrain.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\intent\FixedBrain.cpp" -o "j:\学习\项目\STS_CPP\build\FixedBrain.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\intent\RandomBrain.cpp" -o "j:\学习\项目\STS_CPP\build\RandomBrain.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\intent\AdaptiveBrain.cpp" -o "j:\学习\项目\STS_CPP\build\AdaptiveBrain.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\intent\brains\JawWormBrain.cpp" -o "j:\学习\项目\STS_CPP\build\JawWormBrain.o"
```

### 地图模块（独立测试，不依赖战斗引擎）

```powershell
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\map\GameMap.cpp" -o "j:\学习\项目\STS_CPP\build\GameMap.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\map\MapRenderer.cpp" -o "j:\学习\项目\STS_CPP\build\MapRenderer.o"
g++ -std=c++17 -I "j:\学习\项目\STS_CPP" -c "j:\学习\项目\STS_CPP\src\test\TestMapRenderer.cpp" -o "j:\学习\项目\STS_CPP\build\TestMapRenderer.o"
```

### 2. 链接生成可执行文件

```powershell
g++ -std=c++17 "j:\学习\项目\STS_CPP\build\main.o" "j:\学习\项目\STS_CPP\build\Actions.o" "j:\学习\项目\STS_CPP\build\PlayerActions.o" "j:\学习\项目\STS_CPP\build\LambdaAction.o" "j:\学习\项目\STS_CPP\build\ActionManager.o" "j:\学习\项目\STS_CPP\build\Character.o" "j:\学习\项目\STS_CPP\build\Monster.o" "j:\学习\项目\STS_CPP\build\JawWorm.o" "j:\学习\项目\STS_CPP\build\Intent.o" "j:\学习\项目\STS_CPP\build\IntentBrain.o" "j:\学习\项目\STS_CPP\build\FixedBrain.o" "j:\学习\项目\STS_CPP\build\RandomBrain.o" "j:\学习\项目\STS_CPP\build\AdaptiveBrain.o" "j:\学习\项目\STS_CPP\build\JawWormBrain.o" "j:\学习\项目\STS_CPP\build\Cards.o" "j:\学习\项目\STS_CPP\build\Relics.o" "j:\学习\项目\STS_CPP\build\AbstractRelic.o" "j:\学习\项目\STS_CPP\build\GameState.o" "j:\学习\项目\STS_CPP\build\GameEngine.o" "j:\学习\项目\STS_CPP\build\RunState.o" "j:\学习\项目\STS_CPP\build\CombatState.o" "j:\学习\项目\STS_CPP\build\DeckSystem.o" "j:\学习\项目\STS_CPP\build\CombatFlow.o" "j:\学习\项目\STS_CPP\build\BasicRules.o" "j:\学习\项目\STS_CPP\build\Powers.o" "j:\学习\项目\STS_CPP\build\Potions.o" -o "j:\学习\项目\STS_CPP\build\STS_Game.exe"
```

#### 地图渲染测试（独立可执行文件）

```powershell
g++ -std=c++17 "j:\学习\项目\STS_CPP\build\TestMapRenderer.o" "j:\学习\项目\STS_CPP\build\GameMap.o" "j:\学习\项目\STS_CPP\build\MapRenderer.o" -o "j:\学习\项目\STS_CPP\build\TestMapRenderer.exe"
```

### 3. 运行验证

```powershell
"j:\学习\项目\STS_CPP\build\STS_Game.exe"
```

## 常见错误

### fatal error: src/core/Character.h: No such file or directory
- 错误路径：应使用 `src/character/Character.h`

### fatal error: src/core/GameState.h: No such file or directory
- 错误路径：应使用 `src/gamestate/GameState.h`

### undefined reference to `ResetBrainAction::ResetBrainAction(Monster*)'
- 原因： Actions.h 中声明了 ResetBrainAction 但未实现
- 解决：使用 `ResetAllBrainsAction`（无参数版本）

### private within this context (make_unique)
- 原因：构造函数为 private，std::make_unique 无法访问
- 解决：使用 `std::unique_ptr<T>(new T(...))` 代替

## 源文件清单

| 模块 | 源文件 |
|------|--------|
| engine | GameEngine.cpp |
| state | RunState.cpp, CombatState.cpp |
| action | ActionManager.cpp, Actions.cpp, PlayerActions.cpp, LambdaAction.cpp |
| system | DeckSystem.cpp |
| main | main.cpp |
| character | Character.cpp, Monster.cpp, JawWorm.cpp |
| intent | Intent.cpp, IntentBrain.cpp, FixedBrain.cpp, RandomBrain.cpp, AdaptiveBrain.cpp, JawWormBrain.cpp |
| card | Cards.cpp |
| relic | Relics.cpp, AbstractRelic.cpp |
| gamestate | GameState.cpp |
| flow | CombatFlow.cpp |
| rules | BasicRules.cpp |
| power | Powers.cpp |
| potion | Potions.cpp |
| map | GameMap.cpp, MapRenderer.cpp |
| test | TestMapRenderer.cpp |
