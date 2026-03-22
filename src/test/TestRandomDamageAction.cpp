#include "src/test/TestFramework.h"
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include "src/action/Actions.h"
#include "src/action/LambdaAction.h"
#include "src/rules/BasicRules.h"
#include "src/system/ActionSystem.h"
#include "src/core/Types.h"
#include <map>

using namespace TestFramework;

namespace RandomDamageActionTests {

// ==========================================
// 辅助函数
// ==========================================

GameState createTestState() {
    GameState state;
    state.enableLogging = false;
    state.monsters.push_back(std::make_shared<Monster>("Monster A", 100));
    state.monsters.push_back(std::make_shared<Monster>("Monster B", 100));
    BasicRules::registerRules(state);
    return state;
}

GameState createEmptyState() {
    GameState state;
    state.enableLogging = false;
    BasicRules::registerRules(state);
    return state;
}

// ==========================================
// T1: 单目标 - 随机选择并造成伤害
// ==========================================

void test_SingleTarget_SelectsAndDamages() {
    std::cout << "[DEBUG T1] 开始测试\n";
    GameState state = createEmptyState();
    state.monsters.push_back(std::make_shared<Monster>("Lone Monster", 100));

    auto monster = state.monsters[0];
    int initialHp = monster->current_hp;
    std::cout << "[DEBUG T1] 初始HP: " << initialHp << "\n";

    state.addAction(std::make_unique<RandomDamageAction>(
        state.player, 10, DamageType::ATTACK));
    std::cout << "[DEBUG T1] Action已加入队列\n";

    state.currentPhase = StatePhase::PLAYING_CARD;
    std::cout << "[DEBUG T1] 调用executeUntilBlocked...\n";
    CombatFlow flow;
    ActionSystem::executeUntilBlocked(state, flow);
    std::cout << "[DEBUG T1] executeUntilBlocked返回\n";

    std::cout << "[DEBUG T1] 最终HP: " << monster->current_hp << "\n";
    TEST_ASSERT_EQ(monster->current_hp, initialHp - 10,
        "Single target should take 10 damage");
}

// ==========================================
// T2: 多目标随机选择
// ==========================================

void test_MultipleTargets_RandomSelection() {
    std::cout << "[DEBUG T2] 开始测试\n";
    GameState state = createTestState();

    std::map<std::string, int> hitCount;
    const int iterations = 10;

    for (int i = 0; i < iterations; ++i) {
        std::cout << "[DEBUG T2] 迭代 " << i << " 开始\n";
        GameState localState = createTestState();
        localState.rng.combatRng.seed(1337 + i);
        localState.addAction(std::make_unique<RandomDamageAction>(
            localState.player, 10, DamageType::ATTACK));

        localState.currentPhase = StatePhase::PLAYING_CARD;
        CombatFlow flow;
        ActionSystem::executeUntilBlocked(localState, flow);

        for (auto& m : localState.monsters) {
            if (m->current_hp < 100) {
                hitCount[m->name]++;
                std::cout << "[DEBUG T2] 迭代 " << i << " 命中: " << m->name << "\n";
                break;
            }
        }
        std::cout << "[DEBUG T2] 迭代 " << i << " 完成\n";
    }

    std::cout << "[DEBUG T2] 统计结果: Monster A=" << hitCount["Monster A"]
              << " Monster B=" << hitCount["Monster B"] << "\n";

    TEST_ASSERT(hitCount["Monster A"] > 0 && hitCount["Monster B"] > 0,
        "Both targets should be selected over iterations");

    int expectedPerTarget = iterations / 2;
    int tolerance = iterations * 20 / 100;
    bool aInRange = hitCount["Monster A"] >= (expectedPerTarget - tolerance) && hitCount["Monster A"] <= (expectedPerTarget + tolerance);
    bool bInRange = hitCount["Monster B"] >= (expectedPerTarget - tolerance) && hitCount["Monster B"] <= (expectedPerTarget + tolerance);
    TEST_ASSERT(aInRange && bInRange, "Distribution should be approximately uniform");
}

// ==========================================
// T3: 空目标列表场景
// ==========================================

void test_EmptyTargetList_NoCrash() {
    GameState state = createEmptyState();

    state.addAction(std::make_unique<RandomDamageAction>(
        state.player, 10, DamageType::ATTACK));

    state.currentPhase = StatePhase::PLAYING_CARD;
    CombatFlow flow;

    bool noCrash = true;
    try {
        ActionSystem::executeUntilBlocked(state, flow);
    } catch (...) {
        noCrash = false;
    }

    TEST_ASSERT(noCrash, "Empty target list should not cause crash");
}

// ==========================================
// T4: 所有目标死亡场景
// ==========================================

void test_AllMonstersDead_NoCrash() {
    GameState state = createEmptyState();
    state.monsters.push_back(std::make_shared<Monster>("Dead Monster", 100));
    state.monsters[0]->current_hp = 0;

    state.addAction(std::make_unique<RandomDamageAction>(
        state.player, 10, DamageType::ATTACK));

    state.currentPhase = StatePhase::PLAYING_CARD;
    CombatFlow flow;

    bool noCrash = true;
    try {
        ActionSystem::executeUntilBlocked(state, flow);
    } catch (...) {
        noCrash = false;
    }

    TEST_ASSERT(noCrash, "All monsters dead should not cause crash");
}

// ==========================================
// T5: source死亡场景
// ==========================================

void test_SourceDead_NoCrash() {
    GameState state = createTestState();

    auto deadPlayer = std::make_shared<Monster>("Dead Player", 100);
    deadPlayer->current_hp = 0;

    state.addAction(std::make_unique<RandomDamageAction>(
        deadPlayer, 10, DamageType::ATTACK));

    state.currentPhase = StatePhase::PLAYING_CARD;
    CombatFlow flow;

    bool noCrash = true;
    try {
        ActionSystem::executeUntilBlocked(state, flow);
    } catch (...) {
        noCrash = false;
    }

    TEST_ASSERT(noCrash, "Dead source should not cause crash");
}

// ==========================================
// T7: LambdaAction 集成测试
// ==========================================

void test_IntegrationWithLambdaAction() {
    std::cout << "[DEBUG T7] 开始测试\n";
    GameState state = createTestState();
    auto sharedFlag = std::make_shared<bool>(false);

    std::cout << "[DEBUG T7] 添加LambdaAction\n";
    state.addAction(LambdaAction::make(
        std::weak_ptr<Character>(state.player),
        [sharedFlag](GameState& s, Character* src) {
            *sharedFlag = true;
        }
    ));

    std::cout << "[DEBUG T7] 添加RandomDamageAction\n";
    state.addAction(std::make_unique<RandomDamageAction>(
        state.player, 10, DamageType::ATTACK));

    state.currentPhase = StatePhase::PLAYING_CARD;
    std::cout << "[DEBUG T7] 调用executeUntilBlocked...\n";
    CombatFlow flow;
    ActionSystem::executeUntilBlocked(state, flow);
    std::cout << "[DEBUG T7] executeUntilBlocked返回\n";

    std::cout << "[DEBUG T7] sharedFlag值: " << (*sharedFlag ? "true" : "false") << "\n";
    TEST_ASSERT(*sharedFlag, "Lambda action should execute");
}

// ==========================================
// 运行所有测试
// ==========================================

void runAllTests() {
    TestSuite suite("RandomDamageAction 测试");

    suite.addResult("基础功能", true, "--- T1: 单目标测试 ---");
    RUN_TEST(suite, test_SingleTarget_SelectsAndDamages);

    suite.addResult("随机性", true, "--- T2: 多目标随机选择 ---");
    RUN_TEST(suite, test_MultipleTargets_RandomSelection);

    suite.addResult("边界场景", true, "--- T3-T5: 边界场景 ---");
    RUN_TEST(suite, test_EmptyTargetList_NoCrash);
    RUN_TEST(suite, test_AllMonstersDead_NoCrash);
    RUN_TEST(suite, test_SourceDead_NoCrash);

    suite.addResult("集成测试", true, "--- T7: LambdaAction 集成 ---");
    RUN_TEST(suite, test_IntegrationWithLambdaAction);

    suite.printReport();
}

} // namespace RandomDamageActionTests
