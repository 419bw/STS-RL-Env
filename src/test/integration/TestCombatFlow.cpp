#include <catch_amalgamated.hpp>
#include <iostream>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/character/monster/JawWorm.h"
#include "src/card/Cards.h"
#include "src/rules/BasicRules.h"

// BattlePhase 编号对照（按枚举顺序）:
// 0: BATTLE_START       - 战斗开始（洗牌、触发初始遗物）
// 1: ROUND_START        - 轮次开始（所有角色回到起始状态）
// 2: PLAYER_TURN_START  - 玩家回合开始（重置费用、抽牌）
// 3: PLAYER_ACTION      - 玩家行动（引擎挂起，等待 AI/玩家 输入）
// 4: PLAYER_TURN_END    - 玩家回合结束（弃牌）
// 5: MONSTER_TURN_START - 怪物回合开始
// 6: MONSTER_TURN       - 怪物行动
// 7: MONSTER_TURN_END   - 怪物回合结束
// 8: ROUND_END          - 轮次结束（结算状态效果、中毒等）
// 9: BATTLE_END         - 战斗结束

// =============================================================================
// TestCombatFlow::CombatFlow transitions through all phases
// =============================================================================
// 验证目标：战斗状态机经历所有关键阶段。
//
// 背景说明：
// - CombatFlow 是回合制状态机，通过 tick() 驱动 phase 流转
// - 每个 tick() 只处理一个 phase 的 case，然后返回
// - phase 流转路径: 0 → 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → (9 或回到1)
// - Phase 3 (PLAYER_ACTION) 是 RL/AI 的接口，设计上会在此等待外部输入
//
// 测试策略：
// 1. 手动调用 flow.tick(engine) 推进状态机
// 2. 在 tick 前获取当前 phase 并记录（如果与上一个不同）
// 3. 在 tick 后检查：如果进入 PLAYER_ACTION(3) 且队列为空，
//    说明没有 AI/玩家输入，自动将 phase 推进到 PLAYER_TURN_END(4)
//    这是必要的，因为 PLAYER_ACTION 是空循环，不会自动推进
// 4. 记录所有访问过的 phase，验证所有关键阶段都曾出现
// =============================================================================
TEST_CASE("CombatFlow transitions through all phases", "[combatflow][integration]") {
    GameEngine engine;
    engine.startNewRun(42);

    for (int i = 0; i < 5; ++i) {
        engine.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    }

    engine.startCombat(std::make_shared<JawWorm>(0));
    BasicRules::registerRules(engine);

    CombatFlow flow;
    std::vector<BattlePhase> visitedPhases;
    int maxTicks = 2000;
    int tick = 0;

    while (tick++ < maxTicks &&
           engine.combatState &&
           flow.getCurrentPhase() != BattlePhase::BATTLE_END) {

        BattlePhase currentPhase = flow.getCurrentPhase();

        // Record phase before any advance
        if (visitedPhases.empty() || visitedPhases.back() != currentPhase) {
            visitedPhases.push_back(currentPhase);
        }

        // Simulate AI/player: advance PLAYER_ACTION when queue is empty
        if (CurrentPhase == BattlePhase::PLAYER_ACTION &&
            engine.actionManager.isQueueEmpty()) {
            flow.setPhase(BattlePhase::PLAYER_TURN_END);
            // Also record the advanced phase
            if (visitedPhases.back() != BattlePhase::PLAYER_TURN_END) {
                visitedPhases.push_back(BattlePhase::PLAYER_TURN_END);
            }
        }

        flow.tick(engine);
    }

    // 验证：所有关键阶段都必须出现过
    bool foundBattleStart = false;
    bool foundRoundStart = false;
    bool foundPlayerTurnStart = false;
    bool foundPlayerAction = false;
    bool foundPlayerTurnEnd = false;
    bool foundMonsterTurnStart = false;
    bool foundMonsterTurn = false;
    bool foundMonsterTurnEnd = false;
    bool foundRoundEnd = false;

    for (auto phase : visitedPhases) {
        if (phase == BattlePhase::BATTLE_START) foundBattleStart = true;
        if (phase == BattlePhase::ROUND_START) foundRoundStart = true;
        if (phase == BattlePhase::PLAYER_TURN_START) foundPlayerTurnStart = true;
        if (phase == BattlePhase::PLAYER_ACTION) foundPlayerAction = true;
        if (phase == BattlePhase::PLAYER_TURN_END) foundPlayerTurnEnd = true;
        if (phase == BattlePhase::MONSTER_TURN_START) foundMonsterTurnStart = true;
        if (phase == BattlePhase::MONSTER_TURN) foundMonsterTurn = true;
        if (phase == BattlePhase::MONSTER_TURN_END) foundMonsterTurnEnd = true;
        if (phase == BattlePhase::ROUND_END) foundRoundEnd = true;
    }

    REQUIRE(foundBattleStart);
    REQUIRE(foundRoundStart);
    REQUIRE(foundPlayerTurnStart);
    REQUIRE(foundPlayerAction);
    REQUIRE(foundPlayerTurnEnd);
    REQUIRE(foundMonsterTurnStart);
    REQUIRE(foundMonsterTurn);
    REQUIRE(foundMonsterTurnEnd);
    REQUIRE(foundRoundEnd);
}

// =============================================================================
// TestCombatFlow::CombatFlow enters BATTLE_END after player death
// =============================================================================
// 验证目标：战斗最终能进入 BATTLE_END(9) 阶段（玩家死亡后）
//
// 策略：
// - 和上面测试一样，通过 advance 机制让 PLAYER_ACTION 能自动推进
// - 不关注具体 phase 序列，只关注能否在 maxTicks 内到达 BATTLE_END
// - 如果战斗能在 2000 ticks 内结束，测试通过
// - 如果 PLAYER_ACTION 无法推进，战斗永远无法结束，while 循环会在 tick=2001 时退出并失败
// =============================================================================
TEST_CASE("CombatFlow enters BATTLE_END after player death", "[combatflow][integration]") {
    GameEngine engine;
    engine.startNewRun(42);

    for (int i = 0; i < 5; ++i) {
        engine.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    }

    engine.startCombat(std::make_shared<JawWorm>(0));
    BasicRules::registerRules(engine);

    CombatFlow flow;
    int maxTicks = 2000;
    int tick = 0;

    while (tick++ < maxTicks && engine.combatState &&
           flow.getCurrentPhase() != BattlePhase::BATTLE_END) {

        flow.tick(engine);

        // 与上面测试相同的 advance 逻辑
        // 当 PLAYER_ACTION 队列为空时，自动跳到 PLAYER_TURN_END
        // 这样玩家不会永远卡在等待输入的状态，战斗能继续推进
        if (flow.getCurrentPhase() == BattlePhase::PLAYER_ACTION &&
            engine.actionManager.isQueueEmpty()) {
            flow.setPhase(BattlePhase::PLAYER_TURN_END);
        }
    }

    // 验证：战斗在 maxTicks 内结束
    // 如果 advance 没有正确工作，tick 会超过 maxTicks，测试失败
    REQUIRE(tick <= maxTicks);
}
