#pragma once

#include <string>
#include <memory>
#include "src/core/ForwardDeclarations.h"

// ==========================================
// 遗物基类 (AbstractRelic)
// ==========================================

class AbstractRelic : public std::enable_shared_from_this<AbstractRelic> {
public:
    std::string name;
    AbstractRelic(std::string n) : name(n) {}
    virtual ~AbstractRelic() = default;

    // 在装备遗物时，向 EventBus 注册自己的回调
    virtual void onEquip(GameState& state) = 0;
};
