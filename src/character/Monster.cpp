#include "Character.h"
#include "src/gamestate/GameState.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/utils/Logger.h"

Monster::Monster(std::string n, int hp) : Character(n, hp), deathReported(false) {}

void Monster::rollIntent(GameState& state) {
    if (brain) {
        currentIntent = brain->decide(state, this);
        if (!currentIntent.target) {
            currentIntent.target = state.player.get();
        }
    }
}

void Monster::setBrain(IntentBrainPtr b) {
    brain = b;
}

Intent Monster::getVisibleIntent(const GameState& state) const {
    Intent intent = currentIntent;
    if (state.player && !state.player->canSeeEnemyIntents()) {
        intent.type = IntentType::UNKNOWN;
        intent.visible = false;
        intent.base_damage = -1;
    } else {
        intent.visible = true;
    }
    return intent;
}

void Monster::takeTurn(GameState& state) {
    if (currentIntent.type == IntentType::ATTACK || currentIntent.type == IntentType::ATTACK_DEFEND) {
        if (!currentIntent.target || currentIntent.target->isDead()) {
        } else {
            for (int i = 0; i < currentIntent.hit_count; ++i) {
                state.addAction(std::make_unique<DamageAction>(
                    shared_from_this(),
                    currentIntent.target->shared_from_this(),
                    currentIntent.base_damage));
            }
        }
    }

    if (currentIntent.type == IntentType::DEFEND || currentIntent.type == IntentType::ATTACK_DEFEND) {
        state.addAction(std::make_unique<GainBlockAction>(
            shared_from_this(),
            currentIntent.effect_value));
    }

    if (currentIntent.type == IntentType::BUFF || currentIntent.type == IntentType::DEBUFF) {
        this->executeSpecialIntent(state);
    }
}
