#include "AbstractRelic.h"
#include "src/character/Character.h"
#include "src/engine/GameEngine.h"

void AbstractRelic::onEquip(GameEngine& engine, Character* target) {
    // 基类只做最基础的设置
    // owner 已由 Character::addRelic 设置
    // 子类重写时添加事件订阅逻辑
}
