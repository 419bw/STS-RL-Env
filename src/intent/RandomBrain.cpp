#include "RandomBrain.h"
#include "src/state/CombatState.h"
#include <random>

RandomBrain::RandomBrain(std::vector<Intent> intents)
    : possibleIntents(std::move(intents)) {}

Intent RandomBrain::decide(CombatState& combat, Monster* owner) {
    if (possibleIntents.empty()) {
        return Intent{IntentType::DEFEND, 0, 1, 0, {}};
    }
    std::uniform_int_distribution<size_t> dist(0, possibleIntents.size() - 1);
    Intent intent = possibleIntents[dist(combat.combatRng.monsterRng)];
    recordMoveId(intent.move_id);
    return intent;
}
