#include "Relics.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/card/AbstractCard.h"
#include <iostream>

// ==========================================
// CustomVajraRelic 实现
// ==========================================
void CustomVajraRelic::onEquip(GameState& state) {
    // 使用 weak_ptr 捕获，避免延长对象生命周期
    std::weak_ptr<AbstractRelic> weakSelf = shared_from_this();
    
    state.eventBus.subscribe(EventType::ON_CARD_PLAYED, 
        [weakSelf](GameState& gs, void* context) {
            // 检查遗物是否存活
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }
            
            AbstractCard* playedCard = static_cast<AbstractCard*>(context);
            if (playedCard->type == CardType::ATTACK) {
                std::cout << "[" << self->name << "] 触发！这是一张攻击牌，产生额外效果。\n";
                // 可以往队列里继续塞 Action
            }
        });
}

// ==========================================
// ChemicalXRelic 实现
// ==========================================
void ChemicalXRelic::onEquip(GameState& state) {
    // 使用 weak_ptr 捕获
    std::weak_ptr<AbstractRelic> weakSelf = shared_from_this();
    
    state.eventBus.subscribe(EventType::ON_CARD_PLAYING, 
        [weakSelf](GameState& gs, void* context) {
            // 检查遗物是否存活
            auto self = weakSelf.lock();
            if (!self) {
                return;
            }
            
            AbstractCard* playingCard = static_cast<AbstractCard*>(context);
            
            // 完美解耦：遗物自己判断这张牌是不是 X 费牌
            if (playingCard->cost == -1) {
                playingCard->energyOnUse += 2; // 偷偷把效能 + 2
                std::cout << "[" << self->name << "] 触发！为 X 费牌注入额外能量。当前实际释放能量: " 
                          << playingCard->energyOnUse << "\n";
            }
        });
}
