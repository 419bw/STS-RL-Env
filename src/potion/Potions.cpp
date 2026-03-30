#include "Potions.h"
#include "src/engine/GameEngine.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"

StrengthPotion::StrengthPotion() : AbstractPotion("strength_potion", PotionTarget::SELF) {}

void StrengthPotion::use(GameEngine& engine, std::shared_ptr<Character> target) {
    engine.actionManager.addAction(std::make_unique<ApplyPowerAction>(
        engine.combatState->player,
        engine.combatState->player,
        std::make_shared<StrengthPower>(3)));
}
