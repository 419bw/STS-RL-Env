#pragma once

#include <functional>
#include <memory>
#include "src/action/AbstractAction.h"

class LambdaAction : public AbstractAction {
public:
    static std::unique_ptr<LambdaAction> make(
        std::weak_ptr<Character> source,
        std::function<void(GameState&, Character* source)> closure
    );

    bool update(GameState& state) override;

private:
    explicit LambdaAction(
        std::weak_ptr<Character> source,
        std::function<void(GameState&, Character* source)> closure
    );

    std::weak_ptr<Character> source_;
    std::function<void(GameState&, Character* source)> closure_;
};