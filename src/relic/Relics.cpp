#include "Relics.h"
#include "src/gamestate/GameState.h"
#include "src/event/EventBus.h"
#include "src/card/AbstractCard.h"
#include "src/utils/Logger.h"
#include <iostream>

// ==========================================
// CustomVajraRelic 实现
// 
// 回调返回值：
// - true：遗物存活，继续监听
// - false：遗物已销毁，自动移除监听者
// ==========================================
void CustomVajraRelic::onEquip(GameState& state, Character* target) {
    // 1. 调用基类，把自己塞进 owner 的背包
    AbstractRelic::onEquip(state, target);
    
    // 2. 订阅 EventBus 事件
    std::weak_ptr<AbstractRelic> weakSelf = shared_from_this();
    
    state.eventBus.subscribe(EventType::ON_CARD_PLAYED, 
        [weakSelf](GameState& gs, void* context) -> bool {
            auto self = weakSelf.lock();
            if (!self) {
                return false;
            }
            
            AbstractCard* playedCard = static_cast<AbstractCard*>(context);
            if (playedCard->type == CardType::ATTACK) {
                STS_LOG(gs, "[" << self->name << "] 触发！这是一张攻击牌，产生额外效果。\n");
            }
            return true;
        });
}

// ==========================================
// ChemicalXRelic 实现
// ==========================================
void ChemicalXRelic::onEquip(GameState& state, Character* target) {
    // 1. 调用基类，把自己塞进 owner 的背包
    AbstractRelic::onEquip(state, target);
    
    // 2. 订阅 EventBus 事件
    std::weak_ptr<AbstractRelic> weakSelf = shared_from_this();
    
    state.eventBus.subscribe(EventType::ON_CARD_PLAYING, 
        [weakSelf](GameState& gs, void* context) -> bool {
            auto self = weakSelf.lock();
            if (!self) {
                return false;
            }
            
            AbstractCard* playingCard = static_cast<AbstractCard*>(context);
            
            if (playingCard->cost == -1) {
                playingCard->energyOnUse += 2;
                STS_LOG(gs, "[" << self->name << "] 触发！为 X 费牌注入额外能量。当前实际释放能量: " 
                          << playingCard->energyOnUse << "\n");
            }
            return true;
        });
}
