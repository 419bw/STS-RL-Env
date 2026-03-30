#pragma once

#include "IntentBrain.h"
#include <functional>
#include <optional>

using IntentEvaluator = std::function<std::optional<Intent>(CombatState&, Monster*, Character*)>;

class AdaptiveBrain : public IntentBrain {
private:
    std::vector<IntentEvaluator> evaluators;

public:
    AdaptiveBrain& addRule(IntentEvaluator eval);
    Intent decide(CombatState& combat, Monster* owner) override;
};
