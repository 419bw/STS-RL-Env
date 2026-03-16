#include "AbstractRelic.h"
#include "src/character/Character.h"

// ==========================================
// AbstractRelic 实现
// 
// onEquip 实现需要 Character 的完整定义
// 所以放在 .cpp 文件中避免循环依赖
// ==========================================

void AbstractRelic::onEquip(GameState& state, Character* target) {
    this->owner = target;
    // 把自己塞进主人的背包里，打通 Query 路线
    if (target) {
        target->relics.push_back(shared_from_this());
    }
}
