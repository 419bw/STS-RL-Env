#include "AbstractRelic.h"
#include "src/character/Character.h"

// ==========================================
// AbstractRelic 实现
// 
// onEquip 实现需要 Character 的完整定义
// 所以放在 .cpp 文件中避免循环依赖
// 
// 注意：owner 设置和 relics.push_back
// 现在由 Character::addRelic 统一处理
// 这里只负责事件订阅等逻辑
// ==========================================

void AbstractRelic::onEquip(GameState& state, Character* target) {
    // 基类只做最基础的设置
    // owner 已由 Character::addRelic 设置
    // 子类重写时添加事件订阅逻辑
}
