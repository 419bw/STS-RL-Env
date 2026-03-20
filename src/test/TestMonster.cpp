#pragma once

#include "src/test/TestFramework.h"
#include "src/gamestate/GameState.h"
#include "src/character/Character.h"
#include "src/character/monster/JawWorm.h"
#include "src/intent/Intent.h"
#include "src/intent/FixedBrain.h"
#include "src/intent/RandomBrain.h"
#include "src/intent/brains/JawWormBrain.h"
#include "src/action/Actions.h"
#include "src/action/PlayerActions.h"
#include "src/power/Powers.h"
#include "src/flow/CombatFlow.h"
#include "src/rules/BasicRules.h"
#include "src/system/ActionSystem.h"
#include <memory>
#include <vector>

using namespace TestFramework;

namespace MonsterTests {

GameState createTestState() {
    GameState state;
    state.enableLogging = false;
    state.monsters.push_back(std::make_shared<Monster>("Test Monster", 100));
    BasicRules::registerRules(state);
    return state;
}

GameState createTestStateWithPlayerHp(int hp) {
    GameState state;
    state.enableLogging = false;
    state.player->current_hp = hp;
    state.player->max_hp = hp;
    state.monsters.push_back(std::make_shared<Monster>("Test Monster", 100));
    BasicRules::registerRules(state);
    return state;
}

void test_Monster_BasicConstruction() {
    Monster monster("Basic Monster", 50);
    TEST_ASSERT_EQ(monster.name, "Basic Monster", "Monster name should be set");
    TEST_ASSERT_EQ(monster.max_hp, 50, "Monster max_hp should be 50");
    TEST_ASSERT_EQ(monster.current_hp, 50, "Monster current_hp should equal max_hp on creation");
}

void test_Monster_TakeDamage() {
    Monster monster("Test Monster", 100);
    monster.takeDamage(30);
    TEST_ASSERT_EQ(monster.current_hp, 70, "Monster should take 30 damage");
    TEST_ASSERT_EQ(monster.isDead(), false, "Monster should not be dead after 30 damage");
}

void test_Monster_TakeDamage_KillsMonster() {
    Monster monster("Test Monster", 100);
    monster.takeDamage(100);
    TEST_ASSERT_EQ(monster.current_hp, 0, "Monster hp should be 0 after lethal damage");
    TEST_ASSERT_EQ(monster.isDead(), true, "Monster should be dead after lethal damage");
}

void test_Monster_TakeDamage_ExcessDamage() {
    Monster monster("Test Monster", 100);
    monster.takeDamage(150);
    TEST_ASSERT_EQ(monster.current_hp, 0, "Monster hp should not go below 0");
    TEST_ASSERT_EQ(monster.isDead(), true, "Monster should be dead");
}

void test_Monster_GainBlock() {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(10);
    TEST_ASSERT_EQ(monster.block, 10, "Monster should have 10 block");
}

void test_Monster_GainBlock_Stacks() {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(10);
    monster.addBlockFinal(15);
    TEST_ASSERT_EQ(monster.block, 25, "Monster block should stack to 25");
}

void test_Monster_BlockPreventsDamage() {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(10);
    monster.takeDamage(5);
    TEST_ASSERT_EQ(monster.current_hp, 100, "Damage should be blocked by block");
    TEST_ASSERT_EQ(monster.block, 5, "Block should be reduced by damage");
}

void test_Monster_BlockAbsorbsFullDamage() {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(10);
    monster.takeDamage(10);
    TEST_ASSERT_EQ(monster.current_hp, 100, "Full block should prevent all damage");
    TEST_ASSERT_EQ(monster.block, 0, "Block should be depleted");
}

void test_Monster_BlockOverwhelm() {
    Monster monster("Test Monster", 100);
    monster.addBlockFinal(5);
    monster.takeDamage(12);
    TEST_ASSERT_EQ(monster.current_hp, 93, "Excess damage should pass through block (100 - (12-5) = 93)");
    TEST_ASSERT_EQ(monster.block, 0, "Block should be depleted");
}

void test_Monster_rollIntent_WithBrain() {
    GameState state = createTestState();
    auto monster = state.monsters[0];

    std::vector<Intent> sequence = {
        {IntentType::ATTACK, 10, 1, 0, state.player.get(), true, 1, "Attack"}
    };
    auto brain = std::make_shared<FixedBrain>(sequence);
    monster->setBrain(brain);

    monster->rollIntent(state);

    const Intent& intent = monster->getRealIntent();
    TEST_ASSERT_EQ(static_cast<int>(intent.type), static_cast<int>(IntentType::ATTACK), "Intent type should be ATTACK");
    TEST_ASSERT_EQ(intent.base_damage, 10, "Intent damage should be 10");
}

void test_Monster_rollIntent_WithoutBrain() {
    GameState state = createTestState();
    auto monster = state.monsters[0];

    monster->rollIntent(state);

    const Intent& intent = monster->getRealIntent();
    TEST_ASSERT_EQ(static_cast<int>(intent.type), static_cast<int>(IntentType::ATTACK), "Without brain, intent stays as default ATTACK");
}

void test_Monster_takeTurn_Attack() {
    GameState state = createTestStateWithPlayerHp(100);
    CombatFlow flow;
    auto monster = state.monsters[0];
    auto player = state.player.get();

    std::vector<Intent> sequence = {
        {IntentType::ATTACK, 10, 1, 0, player, true, 1, "Attack"}
    };
    monster->setBrain(std::make_shared<FixedBrain>(sequence));
    monster->rollIntent(state);

    int playerHpBefore = player->current_hp;
    monster->takeTurn(state);

    ActionSystem::executeUntilBlocked(state, flow);

    TEST_ASSERT_EQ(player->current_hp, playerHpBefore - 10, "Player should take 10 damage");
}

void test_Monster_takeTurn_Attack_MultipleHits() {
    GameState state = createTestStateWithPlayerHp(100);
    CombatFlow flow;
    auto monster = state.monsters[0];
    auto player = state.player.get();

    std::vector<Intent> sequence = {
        {IntentType::ATTACK, 10, 3, 0, player, true, 1, "Triple Hit"}
    };
    monster->setBrain(std::make_shared<FixedBrain>(sequence));
    monster->rollIntent(state);

    int playerHpBefore = player->current_hp;
    monster->takeTurn(state);

    ActionSystem::executeUntilBlocked(state, flow);

    TEST_ASSERT_EQ(player->current_hp, playerHpBefore - 30, "Player should take 30 damage from 3 hits");
}

void test_Monster_takeTurn_Defend() {
    GameState state = createTestState();
    CombatFlow flow;
    auto monster = state.monsters[0];

    std::vector<Intent> sequence = {
        {IntentType::DEFEND, 0, 1, 15, nullptr, true, 1, "Defend"}
    };
    monster->setBrain(std::make_shared<FixedBrain>(sequence));
    monster->rollIntent(state);

    monster->takeTurn(state);

    ActionSystem::executeUntilBlocked(state, flow);

    TEST_ASSERT_EQ(monster->block, 15, "Monster should gain 15 block");
}

void test_Monster_takeTurn_AttackDefend() {
    GameState state = createTestStateWithPlayerHp(100);
    CombatFlow flow;
    auto monster = state.monsters[0];
    auto player = state.player.get();

    std::vector<Intent> sequence = {
        {IntentType::ATTACK_DEFEND, 10, 1, 8, player, true, 1, "Strike and Guard"}
    };
    monster->setBrain(std::make_shared<FixedBrain>(sequence));
    monster->rollIntent(state);

    int playerHpBefore = player->current_hp;
    monster->takeTurn(state);

    ActionSystem::executeUntilBlocked(state, flow);

    TEST_ASSERT_EQ(player->current_hp, playerHpBefore - 10, "Player should take 10 damage");
    TEST_ASSERT_EQ(monster->block, 8, "Monster should gain 8 block");
}

void test_JawWorm_Construction() {
    JawWorm worm(0);
    TEST_ASSERT_EQ(worm.name, "Jaw Worm", "JawWorm name should be correct");
    TEST_ASSERT_EQ(worm.max_hp, 42, "JawWorm max_hp should be 42 at ascension 0");
    TEST_ASSERT_EQ(worm.current_hp, 42, "JawWorm current_hp should equal max_hp");
}

void test_JawWorm_AscensionScaling_Low() {
    JawWorm worm(0);
    TEST_ASSERT_EQ(worm.max_hp, 42, "Ascension 0: max_hp should be 42");
}

void test_JawWorm_AscensionScaling_Mid() {
    JawWorm worm(7);
    TEST_ASSERT_EQ(worm.max_hp, 44, "Ascension 7: max_hp should be 44");
}

void test_JawWorm_AscensionScaling_High() {
    JawWorm worm(17);
    TEST_ASSERT_EQ(worm.max_hp, 44, "Ascension 17: max_hp should be 44");
}

void test_JawWorm_FirstMove_AlwaysChomp() {
    GameState state;
    state.enableLogging = false;
    JawWorm worm(0);
    auto wormPtr = std::make_shared<JawWorm>(worm);
    state.monsters.push_back(wormPtr);
    BasicRules::registerRules(state);

    wormPtr->rollIntent(state);
    Intent intent = wormPtr->getRealIntent();

    TEST_ASSERT_EQ(static_cast<int>(intent.type), static_cast<int>(IntentType::ATTACK), "First move should be ATTACK (Chomp)");
    TEST_ASSERT_EQ(intent.move_id, JawWormBrain::CHOMP, "First move should be CHOMP");
    TEST_ASSERT_EQ(intent.base_damage, 11, "Chomp damage should be 11 at ascension 0");
}

void test_JawWorm_Thrash_AttackAndBlock() {
    GameState state = createTestStateWithPlayerHp(100);
    CombatFlow flow;
    JawWorm worm(0);
    auto wormPtr = std::make_shared<JawWorm>(worm);
    state.monsters.push_back(wormPtr);
    BasicRules::registerRules(state);

    auto player = state.player.get();
    std::vector<Intent> thrashIntent = {
        {IntentType::ATTACK_DEFEND, 7, 1, 5, player, true, JawWormBrain::THRASH, "Thrash"}
    };
    auto thrashBrain = std::make_shared<FixedBrain>(thrashIntent);
    wormPtr->setBrain(thrashBrain);
    wormPtr->rollIntent(state);

    int playerHpBefore = player->current_hp;
    wormPtr->takeTurn(state);

    ActionSystem::executeUntilBlocked(state, flow);

    TEST_ASSERT_EQ(player->current_hp, playerHpBefore - 7, "Thrash should deal 7 damage");
    TEST_ASSERT_EQ(wormPtr->block, 5, "Thrash should grant 5 block");
}

void test_IntentBrain_MoveHistory_LastMove() {
    GameState state;
    state.enableLogging = false;
    JawWorm worm(0);
    state.monsters.push_back(std::make_shared<JawWorm>(worm));
    BasicRules::registerRules(state);

    std::vector<Intent> sequence = {
        {IntentType::ATTACK, 10, 1, 0, state.player.get(), true, 1, "Move1"},
        {IntentType::DEFEND, 0, 1, 5, nullptr, true, 2, "Move2"},
        {IntentType::ATTACK, 10, 1, 0, state.player.get(), true, 3, "Move3"}
    };
    auto brain = std::make_shared<FixedBrain>(sequence);

    brain->decide(state, &worm);
    TEST_ASSERT_EQ(brain->lastMove(1), true, "Should remember last move was 1");
    TEST_ASSERT_EQ(brain->lastMove(2), false, "Last move should not be 2");

    brain->decide(state, &worm);
    TEST_ASSERT_EQ(brain->lastMove(2), true, "Should remember last move was 2");

    brain->decide(state, &worm);
    TEST_ASSERT_EQ(brain->lastMove(3), true, "Should remember last move was 3");
}

void test_IntentBrain_MoveHistory_LastTwoMoves() {
    GameState state;
    state.enableLogging = false;
    JawWorm worm(0);
    state.monsters.push_back(std::make_shared<JawWorm>(worm));
    BasicRules::registerRules(state);

    std::vector<Intent> sequence = {
        {IntentType::ATTACK, 10, 1, 0, state.player.get(), true, 1, "Move1"},
        {IntentType::DEFEND, 0, 1, 5, nullptr, true, 2, "Move2"},
        {IntentType::ATTACK, 10, 1, 0, state.player.get(), true, 3, "Move3"}
    };
    auto brain = std::make_shared<FixedBrain>(sequence);

    brain->decide(state, &worm);
    brain->decide(state, &worm);
    TEST_ASSERT_EQ(brain->lastTwoMoves(2), false, "Should not have two moves yet");

    brain->decide(state, &worm);
    TEST_ASSERT_EQ(brain->lastTwoMoves(3), false, "Last two are [2,3], not both 3");
    TEST_ASSERT_EQ(brain->lastTwoMoves(2), false, "Last two are [2,3], not both 2");
    TEST_ASSERT_EQ(brain->lastTwoMoves(1), false, "Last two are [2,3], not both 1");
}

void runAllTests() {
    TestSuite suite("怪物系统测试");

    suite.addResult("基础功能", true, "--- 基础功能测试 ---");
    RUN_TEST(suite, test_Monster_BasicConstruction);
    RUN_TEST(suite, test_Monster_TakeDamage);
    RUN_TEST(suite, test_Monster_TakeDamage_KillsMonster);
    RUN_TEST(suite, test_Monster_TakeDamage_ExcessDamage);
    RUN_TEST(suite, test_Monster_GainBlock);
    RUN_TEST(suite, test_Monster_GainBlock_Stacks);
    RUN_TEST(suite, test_Monster_BlockPreventsDamage);
    RUN_TEST(suite, test_Monster_BlockAbsorbsFullDamage);
    RUN_TEST(suite, test_Monster_BlockOverwhelm);

    suite.addResult("Intent 系统", true, "--- Intent 系统测试 ---");
    RUN_TEST(suite, test_Monster_rollIntent_WithBrain);
    RUN_TEST(suite, test_Monster_rollIntent_WithoutBrain);

    suite.addResult("takeTurn 行为", true, "--- takeTurn 行为测试 ---");
    RUN_TEST(suite, test_Monster_takeTurn_Attack);
    RUN_TEST(suite, test_Monster_takeTurn_Attack_MultipleHits);
    RUN_TEST(suite, test_Monster_takeTurn_Defend);
    RUN_TEST(suite, test_Monster_takeTurn_AttackDefend);

    suite.addResult("JawWorm 专属", true, "--- JawWorm 专属测试 ---");
    RUN_TEST(suite, test_JawWorm_Construction);
    RUN_TEST(suite, test_JawWorm_AscensionScaling_Low);
    RUN_TEST(suite, test_JawWorm_AscensionScaling_Mid);
    RUN_TEST(suite, test_JawWorm_AscensionScaling_High);
    RUN_TEST(suite, test_JawWorm_FirstMove_AlwaysChomp);
    RUN_TEST(suite, test_JawWorm_Thrash_AttackAndBlock);

    suite.addResult("IntentBrain 历史记录", true, "--- IntentBrain 历史记录测试 ---");
    RUN_TEST(suite, test_IntentBrain_MoveHistory_LastMove);
    RUN_TEST(suite, test_IntentBrain_MoveHistory_LastTwoMoves);

    suite.printReport();
}

}
