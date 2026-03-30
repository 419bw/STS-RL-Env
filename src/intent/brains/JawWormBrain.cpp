#include "JawWormBrain.h"
#include "src/state/CombatState.h"
#include <random>

constexpr double BELLOW_WEIGHT = 45.0;
constexpr double THRASH_WEIGHT_IN_BUGGED_POOL = 35.0;
constexpr double PROB_BELLOW_VS_THRASH = 45.0 / (45.0 + 35.0);
constexpr double PROB_CHOMP_VS_BELLOW_FROM_THRASH = 0.357;
constexpr double PROB_CHOMP_VS_THRASH_FROM_BELLOW = 0.416;

JawWormBrain::JawWormBrain(int ascensionLevel) : ascensionLevel_(ascensionLevel) {}

void JawWormBrain::initializeStats(int ascensionLevel) {
    if (ascensionLevel >= 17) {
        bellowStr = 5;
        bellowBlock = 9;
        chompDmg = 12;
        thrashDmg = 7;
        thrashBlock = 5;
    } else if (ascensionLevel >= 2) {
        bellowStr = 4;
        bellowBlock = 6;
        chompDmg = 12;
        thrashDmg = 7;
        thrashBlock = 5;
    } else {
        bellowStr = 3;
        bellowBlock = 6;
        chompDmg = 11;
        thrashDmg = 7;
        thrashBlock = 5;
    }
}

Intent JawWormBrain::decide(CombatState& combat, Monster* owner) {
    if (firstMove) {
        firstMove = false;
        recordMoveId(CHOMP);
        return Intent(IntentType::ATTACK, chompDmg, 1, 0, combat.player)
               .withMove(CHOMP, "Chomp");
    }

    int randomNum = std::uniform_int_distribution<int>(0, 99)(combat.combatRng.monsterRng);
    Intent intent = decideNextMove(combat, owner, randomNum);
    recordMoveId(intent.move_id);
    return intent;
}

Intent JawWormBrain::decideNextMove(CombatState& combat, Monster* owner, int randomNum) {
    if (randomNum < 25) {
        if (lastMove(CHOMP)) {
            if (std::bernoulli_distribution(PROB_BELLOW_VS_THRASH)(combat.combatRng.monsterRng)) {
                return Intent(IntentType::BUFF, 0, 1, bellowBlock, {})
                       .withMove(BELLOW, "Bellow");
            } else {
                return Intent(IntentType::ATTACK_DEFEND, thrashDmg, 1, thrashBlock, combat.player)
                       .withMove(THRASH, "Thrash");
            }
        } else {
            return Intent(IntentType::ATTACK, chompDmg, 1, 0, combat.player)
                   .withMove(CHOMP, "Chomp");
        }
    } else if (randomNum < 55) {
        if (lastTwoMoves(THRASH)) {
            if (std::bernoulli_distribution(PROB_CHOMP_VS_BELLOW_FROM_THRASH)(combat.combatRng.monsterRng)) {
                return Intent(IntentType::ATTACK, chompDmg, 1, 0, combat.player)
                       .withMove(CHOMP, "Chomp");
            } else {
                return Intent(IntentType::BUFF, 0, 1, bellowBlock, {})
                       .withMove(BELLOW, "Bellow");
            }
        } else {
            return Intent(IntentType::ATTACK_DEFEND, thrashDmg, 1, thrashBlock, combat.player)
                   .withMove(THRASH, "Thrash");
        }
    } else {
        if (lastMove(BELLOW)) {
            if (std::bernoulli_distribution(PROB_CHOMP_VS_THRASH_FROM_BELLOW)(combat.combatRng.monsterRng)) {
                return Intent(IntentType::ATTACK, chompDmg, 1, 0, combat.player)
                       .withMove(CHOMP, "Chomp");
            } else {
                return Intent(IntentType::ATTACK_DEFEND, thrashDmg, 1, thrashBlock, combat.player)
                       .withMove(THRASH, "Thrash");
            }
        } else {
            return Intent(IntentType::BUFF, 0, 1, bellowBlock, {})
                   .withMove(BELLOW, "Bellow");
        }
    }
}
