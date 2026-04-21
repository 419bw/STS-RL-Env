#include <catch_amalgamated.hpp>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/character/monster/JawWorm.h"
#include "src/card/Cards.h"
#include "src/rules/BasicRules.h"

// =============================================================================
// advanceUntilPlayerTurnEnd
// =============================================================================
// 模拟 AI/玩家自动结束回合的工具函数。
//
// 问题背景：
// CombatFlow 的 PLAYER_ACTION(3) 阶段是空循环，设计上是等待外部 AI/玩家输入。
// 如果没有外部输入，phase 永远不会推进，战斗会永远卡在这里。
//
// 解决方案：
// 在 tick 后检查：如果当前 phase 是 PLAYER_ACTION 且 actionManager 队列为空，
// 说明没有待执行的动作，直接将 phase 设置为 PLAYER_TURN_END(4)，让战斗继续推进。
//
// 为什么不修改 CombatFlow 源码？
// PLAYER_ACTION 是刻意留空的，这是 RL/AI 的接口。修改源码会破坏这个设计。
// 测试中手动推进 phase 是合理的模拟方式。
// =============================================================================
static void advanceUntilPlayerTurnEnd(GameEngine& engine, CombatFlow& flow) {
    if (flow.getCurrentPhase() == BattlePhase::PLAYER_ACTION &&
        engine.actionManager.isQueueEmpty()) {
        flow.setPhase(BattlePhase::PLAYER_TURN_END);
    }
}

// =============================================================================
// TestFullRound::Full combat round completes without crash
// =============================================================================
// 验证目标：完整战斗流程能正常结束，不会因为死循环而崩溃。
//
// 场景设置：
// - 玩家：5 张 Strike + 3 张 DeadlyPoison（无遗物、无力量）
// - 怪物：Jaw Worm（42 HP，Ascension 0）
// - 玩家不会出牌，只会承受怪物攻击并最终死亡
//
// 战斗流程：
// 1. BATTLE_START → ROUND_START → PLAYER_TURN_START
// 2. PLAYER_ACTION（无动作，被 advance 跳过）
// 3. PLAYER_TURN_END → MONSTER_TURN_START → MONSTER_TURN → MONSTER_TURN_END
// 4. ROUND_END（结算状态效果）
// 5. 重复 1-4 直到玩家死亡
// 6. 玩家死亡后 checkBattleEndCondition → BATTLE_END
//
// 测试策略：
// - 使用 advanceUntilPlayerTurnEnd 模拟玩家跳过出牌阶段
// - 不断调用 tick() 直到战斗结束
// - 如果 advance 工作正常，战斗应该在 2000 ticks 内结束
// - 如果 advance 不工作（advance 检查条件错误），战斗永远卡在 PLAYER_ACTION，
//   while 循环会在 tick=2001 时退出并触发 REQUIRE 失败
// =============================================================================
TEST_CASE("Full combat round completes without crash", "[integration]") {
    GameEngine engine;
    engine.startNewRun(42);

    for (int i = 0; i < 5; ++i) {
        engine.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    }
    for (int i = 0; i < 3; ++i) {
        engine.runState->masterDeck.push_back(std::make_shared<DeadlyPoisonCard>());
    }

    engine.startCombat(std::make_shared<JawWorm>(0));
    BasicRules::registerRules(engine);

    CombatFlow flow;
    int maxTicks = 2000;
    int ticks = 0;

    while (ticks++ < maxTicks &&
           engine.combatState &&
           flow.getCurrentPhase() != BattlePhase::BATTLE_END) {

        flow.tick(engine);

        // 关键：每次 tick 后都要检查并推进 PLAYER_ACTION
        // 如果没有这行，PLAYER_ACTION 会永远卡住
        advanceUntilPlayerTurnEnd(engine, flow);
    }

    // 验证：战斗在 maxTicks 内结束，且结果正确
    REQUIRE(ticks <= maxTicks);
    REQUIRE(flow.getCurrentPhase() == BattlePhase::BATTLE_END);
    REQUIRE(engine.combatState->isPlayerDead == true);
    REQUIRE(engine.combatState->isMonsterDead == false);
}

// =============================================================================
// TestFullRound::Player dies after lethal monster damage
// =============================================================================
// 验证目标：玩家在承受足够的怪物伤害后正确死亡。
//
// 场景设置：
// - 玩家：5 张 Strike，初始 HP 75
// - 怪物：Jaw Worm（Ascension 0）
//   - Chomp（第一回合）：11 伤害
//   - Thrash（后续回合）：7 伤害 + 5 格挡
//   - Bellow（力量 buff）：+3 力量 + 6 格挡
//
// 预计战斗回合数：
// - 回合 1：Chomp 11 伤害，玩家 HP 75 → 64
// - 回合 2：力量 3，Tharsh 7+3=10 伤害 + 5 格挡，玩家 HP 64 → 54
// - 回合 3：力量 6，Tharsh 7+6=13 伤害 + 5 格挡，玩家 HP 54 → 41
// - ...（力量持续叠加）
// - 约 6-7 回合后玩家死亡
//
// 验证：
// - 不使用 maxTicks 断言（因为不关心战斗是否超时）
// - 只验证 engine.combatState != nullptr（战斗正常结束，没有崩溃）
// - 不验证战斗结果（输赢），因为测试设计就是玩家会输
// =============================================================================
TEST_CASE("Player dies after lethal monster damage", "[integration]") {
    GameEngine engine;
    engine.startNewRun(42);

    for (int i = 0; i < 5; ++i) {
        engine.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    }

    engine.startCombat(std::make_shared<JawWorm>(0));
    BasicRules::registerRules(engine);

    CombatFlow flow;
    int maxTicks = 2000;
    int ticks = 0;

    while (ticks++ < maxTicks &&
           engine.combatState &&
           flow.getCurrentPhase() != BattlePhase::BATTLE_END) {

        flow.tick(engine);
        advanceUntilPlayerTurnEnd(engine, flow);
    }

    // 验证：战斗正常结束，玩家正确死亡
    REQUIRE(flow.getCurrentPhase() == BattlePhase::BATTLE_END);
    REQUIRE(engine.combatState->isPlayerDead == true);
    REQUIRE(engine.combatState->player->current_hp == 0);
}
