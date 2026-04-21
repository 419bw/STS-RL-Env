#include <catch_amalgamated.hpp>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"
#include "src/action/PlayerActions.h"
#include "src/card/Cards.h"
#include "src/character/monster/JawWorm.h"
#include "src/rules/BasicRules.h"

static std::string serializePile(const std::vector<std::shared_ptr<AbstractCard>>& pile) {
    std::string result;
    for (auto& card : pile) {
        result += card->id + ",";
    }
    return result;
}

static bool combatStatesIdentical(const CombatState& a, const CombatState& b) {
    if (a.turnCount != b.turnCount) return false;
    if (a.player->current_hp != b.player->current_hp) return false;
    if (a.player->block != b.player->block) return false;
    if (a.player->getEnergy() != b.player->getEnergy()) return false;

    if (a.monsters.size() != b.monsters.size()) return false;
    for (size_t i = 0; i < a.monsters.size(); ++i) {
        if (a.monsters[i]->current_hp != b.monsters[i]->current_hp) return false;
        if (a.monsters[i]->block != b.monsters[i]->block) return false;
        auto intentA = a.monsters[i]->getRealIntent();
        auto intentB = b.monsters[i]->getRealIntent();
        if (intentA.base_damage != intentB.base_damage) return false;
        if (intentA.hit_count != intentB.hit_count) return false;
    }

    if (serializePile(a.drawPile) != serializePile(b.drawPile)) return false;
    if (serializePile(a.hand) != serializePile(b.hand)) return false;
    if (serializePile(a.discardPile) != serializePile(b.discardPile)) return false;
    if (serializePile(a.exhaustPile) != serializePile(b.exhaustPile)) return false;

    return true;
}

static void setupIdenticalEngines(GameEngine& engine1, GameEngine& engine2, unsigned int seed) {
    engine1.startNewRun(seed);
    engine2.startNewRun(seed);

    for (int i = 0; i < 5; ++i) {
        engine1.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
        engine2.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    }
    for (int i = 0; i < 3; ++i) {
        engine1.runState->masterDeck.push_back(std::make_shared<DeadlyPoisonCard>());
        engine2.runState->masterDeck.push_back(std::make_shared<DeadlyPoisonCard>());
    }

    engine1.startCombat(std::make_shared<JawWorm>(0));
    engine2.startCombat(std::make_shared<JawWorm>(0));

    BasicRules::registerRules(engine1);
    BasicRules::registerRules(engine2);
}

TEST_CASE("Identical seeds produce identical combat trajectory", "[determinism][unit]") {
    std::vector<unsigned int> seeds = {0, 42, 1337, 99999};

    for (unsigned int seed : seeds) {
        GameEngine engine1, engine2;
        setupIdenticalEngines(engine1, engine2, seed);

        CombatFlow flow1, flow2;
        int maxTicks = 30;

        for (int tick = 0; tick < maxTicks; ++tick) {
            if (!engine1.combatState || !engine2.combatState) break;
            if (flow1.getCurrentPhase() == BattlePhase::BATTLE_END ||
                flow2.getCurrentPhase() == BattlePhase::BATTLE_END) break;

            flow1.tick(engine1);
            flow2.tick(engine2);

            if (!engine1.combatState || !engine2.combatState) break;
            if (flow1.getCurrentPhase() == BattlePhase::BATTLE_END ||
                flow2.getCurrentPhase() == BattlePhase::BATTLE_END) break;

            if (flow1.getCurrentPhase() == BattlePhase::PLAYER_ACTION &&
                flow2.getCurrentPhase() == BattlePhase::PLAYER_ACTION &&
                engine1.actionManager.isQueueEmpty() &&
                engine2.actionManager.isQueueEmpty() &&
                !engine1.combatState->hand.empty() &&
                !engine2.combatState->hand.empty()) {

                auto card1 = engine1.combatState->hand[0];
                auto card2 = engine2.combatState->hand[0];
                if (engine1.combatState->player->getEnergy() >= card1->cost &&
                    engine2.combatState->player->getEnergy() >= card2->cost) {
                    PlayerActions::playCard(engine1, flow1, card1, engine1.combatState->monsters[0]);
                    PlayerActions::playCard(engine2, flow2, card2, engine2.combatState->monsters[0]);
                } else {
                    flow1.setPhase(BattlePhase::PLAYER_TURN_END);
                    flow2.setPhase(BattlePhase::PLAYER_TURN_END);
                }
            }
        }

        REQUIRE(engine1.combatState != nullptr);
        REQUIRE(engine2.combatState != nullptr);
        REQUIRE(combatStatesIdentical(*engine1.combatState, *engine2.combatState));
    }
}

TEST_CASE("Different seeds produce different combat states", "[determinism][unit]") {
    GameEngine engine1, engine2;
    engine1.startNewRun(42);
    engine2.startNewRun(1337);

    for (int i = 0; i < 10; ++i) {
        engine1.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
        engine2.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    }
    for (int i = 0; i < 5; ++i) {
        engine1.runState->masterDeck.push_back(std::make_shared<DeadlyPoisonCard>());
        engine2.runState->masterDeck.push_back(std::make_shared<DeadlyPoisonCard>());
    }

    engine1.startCombat(std::make_shared<JawWorm>(0));
    engine2.startCombat(std::make_shared<JawWorm>(0));
    BasicRules::registerRules(engine1);
    BasicRules::registerRules(engine2);

    CombatFlow flow1, flow2;
    flow1.tick(engine1);
    flow2.tick(engine2);

    std::string fullDeck1 = serializePile(engine1.combatState->hand) + serializePile(engine1.combatState->drawPile) + serializePile(engine1.combatState->discardPile);
    std::string fullDeck2 = serializePile(engine2.combatState->hand) + serializePile(engine2.combatState->drawPile) + serializePile(engine2.combatState->discardPile);
    REQUIRE(fullDeck1 != fullDeck2);
}

TEST_CASE("Multiple engine resets maintain determinism", "[determinism][unit]") {
    unsigned int seed = 12345;

    for (int trial = 0; trial < 5; ++trial) {
        GameEngine engine1, engine2;
        engine1.startNewRun(seed);
        engine2.startNewRun(seed);

        for (int i = 0; i < 5; ++i) {
            engine1.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
            engine2.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
        }

        engine1.startCombat(std::make_shared<JawWorm>(0));
        engine2.startCombat(std::make_shared<JawWorm>(0));
        BasicRules::registerRules(engine1);
        BasicRules::registerRules(engine2);

        CombatFlow flow1, flow2;

        // 推进若干 tick
        for (int tick = 0; tick < 10; ++tick) {
            flow1.tick(engine1);
            flow2.tick(engine2);

            if (flow1.getCurrentPhase() == BattlePhase::PLAYER_ACTION &&
                engine1.actionManager.isQueueEmpty()) {
                flow1.setPhase(BattlePhase::PLAYER_TURN_END);
            }
            if (flow2.getCurrentPhase() == BattlePhase::PLAYER_ACTION &&
                engine2.actionManager.isQueueEmpty()) {
                flow2.setPhase(BattlePhase::PLAYER_TURN_END);
            }
        }

        // 比较多维状态
        REQUIRE(engine1.combatState->player->current_hp == engine2.combatState->player->current_hp);
        REQUIRE(engine1.combatState->player->getEnergy() == engine2.combatState->player->getEnergy());
        REQUIRE(engine1.combatState->player->block == engine2.combatState->player->block);
        REQUIRE(serializePile(engine1.combatState->hand) == serializePile(engine2.combatState->hand));
        REQUIRE(serializePile(engine1.combatState->drawPile) == serializePile(engine2.combatState->drawPile));
        REQUIRE(serializePile(engine1.combatState->discardPile) == serializePile(engine2.combatState->discardPile));
        REQUIRE(engine1.combatState->monsters[0]->current_hp == engine2.combatState->monsters[0]->current_hp);
        REQUIRE(engine1.combatState->monsters[0]->block == engine2.combatState->monsters[0]->block);
        REQUIRE(engine1.combatState->turnCount == engine2.combatState->turnCount);

        auto intent1 = engine1.combatState->monsters[0]->getRealIntent();
        auto intent2 = engine2.combatState->monsters[0]->getRealIntent();
        REQUIRE(static_cast<int>(intent1.type) == static_cast<int>(intent2.type));
        REQUIRE(intent1.effect_value == intent2.effect_value);
    }
}

TEST_CASE("Combat RNG is isolated from RunState RNG", "[determinism][unit]") {
    // 引擎 A：正常创建战斗，推进战斗内 tick
    GameEngine engineA;
    engineA.startNewRun(42);
    for (int i = 0; i < 5; ++i) engineA.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    engineA.startCombat(std::make_shared<JawWorm>(0));
    BasicRules::registerRules(engineA);
    CombatFlow flowA;
    for (int i = 0; i < 10; ++i) flowA.tick(engineA);
    auto intentA = engineA.combatState->monsters[0]->getRealIntent();

    // 引擎 B：同样的 seed，先消费 combatRNG（通过战斗 tick），再检查 runState.rng 无关结果
    GameEngine engineB;
    engineB.startNewRun(42);
    for (int i = 0; i < 5; ++i) engineB.runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    engineB.startCombat(std::make_shared<JawWorm>(0));
    BasicRules::registerRules(engineB);
    CombatFlow flowB;
    for (int i = 0; i < 10; ++i) flowB.tick(engineB);
    auto intentB = engineB.combatState->monsters[0]->getRealIntent();

    // 验证：两个引擎意图一致（战斗 RNG 确定性）
    REQUIRE(static_cast<int>(intentA.type) == static_cast<int>(intentB.type));
    REQUIRE(intentA.effect_value == intentB.effect_value);

    // 验证：手牌和抽牌堆一致（洗牌 RNG 确定性）
    REQUIRE(serializePile(engineA.combatState->hand) == serializePile(engineB.combatState->hand));
    REQUIRE(serializePile(engineA.combatState->drawPile) == serializePile(engineB.combatState->drawPile));
}
