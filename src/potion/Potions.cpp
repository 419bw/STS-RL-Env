#include "Potions.h"
#include "src/gamestate/GameState.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"

StrengthPotion::StrengthPotion() : AbstractPotion("strength_potion", PotionTarget::SELF) {}

void StrengthPotion::use(GameState& state, std::shared_ptr<Character> target) {
    state.addAction(std::make_unique<ApplyPowerAction>(
        state.player,
        state.player,
        std::make_shared<StrengthPower>(3)));
}
