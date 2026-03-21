#include "LambdaAction.h"
#include "src/character/Character.h"
#include "src/gamestate/GameState.h"

LambdaAction::LambdaAction(
    std::weak_ptr<Character> source,
    std::function<void(GameState&, Character* source)> closure)
    : source_(std::move(source)), closure_(std::move(closure)) {}

std::unique_ptr<LambdaAction> LambdaAction::make(
    std::weak_ptr<Character> source,
    std::function<void(GameState&, Character* source)> closure
) {
    return std::unique_ptr<LambdaAction>(
        new LambdaAction(std::move(source), std::move(closure)));
}

bool LambdaAction::update(GameState& state) {
    auto locked = source_.lock();
    if (!locked) return true;
    if (locked->isDead()) return true;

    Character* raw = locked.get();
    closure_(state, raw);

    return true;
}