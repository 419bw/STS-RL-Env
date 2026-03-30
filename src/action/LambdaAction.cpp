#include "LambdaAction.h"
#include "src/character/Character.h"
#include "src/engine/GameEngine.h"

// ==========================================
// LambdaAction 构造函数
// ==========================================
LambdaAction::LambdaAction(
    std::weak_ptr<Character> source,
    std::function<void(GameEngine&, Character* source)> closure)
    : source_(std::move(source)), closure_(std::move(closure)) {}

// ==========================================
// make - 工厂函数
// ==========================================
std::unique_ptr<LambdaAction> LambdaAction::make(
    std::weak_ptr<Character> source,
    std::function<void(GameEngine&, Character* source)> closure
) {
    return std::unique_ptr<LambdaAction>(
        new LambdaAction(std::move(source), std::move(closure)));
}

// ==========================================
// update - 执行动作
//
// 执行流程：
// 1. 尝试 lock() source_
// 2. 若失败（对象已销毁），返回 true（动作完成）
// 3. 若 source 已死亡，返回 true（动作完成）
// 4. 调用闭包函数
// 5. 返回 true（LambdaAction 总是同步完成）
//
// 安全机制：
// - weak_ptr.lock() 检测对象是否已销毁
// - isDead() 检测目标是否已死亡
// - 两者都安全，不会悬挂
// ==========================================
bool LambdaAction::update(GameEngine& engine) {
    auto locked = source_.lock();
    if (!locked) return true;
    if (locked->isDead()) return true;

    Character* raw = locked.get();
    closure_(engine, raw);

    return true;
}
