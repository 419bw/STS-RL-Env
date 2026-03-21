#include "PlayerActions.h"
#include "src/gamestate/GameState.h"
#include "src/flow/CombatFlow.h"
#include "src/action/Actions.h"
#include "src/card/AbstractCard.h"
#include "src/system/ActionSystem.h"
#include "src/utils/Logger.h"
#include <algorithm>
#include <iostream>
#include <random>

// ==========================================
// 玩家动作实现
// 
// 状态机拦截：
// - playCard: 只在 PLAYING_CARD 阶段可用
// - chooseCard/chooseCards: 只在 WAITING_FOR_CARD_SELECTION 阶段可用
// 
// 铁律：所有动作执行后调用 ActionSystem::executeUntilBlocked
// ==========================================

bool PlayerActions::playCard(GameState& state, 
                              CombatFlow& flow,
                              std::shared_ptr<AbstractCard> card,
                              std::shared_ptr<Character> target) {
    // ★ 真正的安全锁：系统是不是在自由出牌状态？
    if (state.currentPhase != StatePhase::PLAYING_CARD) {
        STS_LOG(state, "[警告] 系统正在等待你做出选择，无法打出新牌！\n");
        return false;
    }

    // ==========================================
    // 步骤 1：扣除费用
    // ==========================================
    if (card->cost == -1) {
        // X 费牌：消耗所有能量
        card->energyOnUse = state.player->getEnergy();
        state.player->spendEnergy(-1);  // -1 表示消耗所有
    } else if (state.player->hasEnoughEnergy(card->cost)) {
        // 普通牌：扣除对应费用
        state.player->spendEnergy(card->cost);
    } else {
        // 费用不足
        STS_LOG(state, "[系统拦截] 费用不足，无法打出 " << card->id << "!\n");
        return false;
    }

    // ==========================================
    // 步骤 2：进入滞留区
    // 将该卡牌从 state.hand 移除，并 push_back 进入 state.limbo
    // 防止洗牌时被错误洗回抽牌堆
    // ==========================================
    auto it = std::find(state.hand.begin(), state.hand.end(), card);
    if (it != state.hand.end()) {
        state.hand.erase(it);
    }
    state.limbo.push_back(card);

    // ==========================================
    // 步骤 3：效果排队
    // 调用 card->use(state, target)，将卡牌自身产生的业务动作推入队列
    // ==========================================
    state.eventBus.publish(EventType::ON_CARD_PLAYING, state, card.get());

    // CardTarget 校验路由
    switch (card->targetType) {
        case CardTarget::ENEMY:
            if (!target || target->isDead()) {
                STS_LOG(state, "[警告] " << card->id << " 需要指定一个存活的敌人目标！\n");
                return false;
            }
            break;
        case CardTarget::ALL_ENEMY:
            target = nullptr;
            break;
        case CardTarget::SELF:
            target = state.player;
            break;
        case CardTarget::NONE:
            target = nullptr;
            break;
        case CardTarget::RANDOM:
            {
                std::vector<std::shared_ptr<Monster>> alive;
                for (auto& m : state.monsters) {
                    if (!m->isDead()) {
                        alive.push_back(m);
                    }
                }
                if (!alive.empty()) {
                    std::uniform_int_distribution<> dist(0, static_cast<int>(alive.size()) - 1);
                    target = alive[dist(state.rng.combatRng)];
                } else {
                    STS_LOG(state, "[警告] RANDOM 目标无可用敌人！\n");
                    return false;
                }
            }
            break;
        default:
            STS_LOG(state, "[错误] 未知 CardTarget 类型！\n");
            return false;
    }

    card->use(state, target);
    state.eventBus.publish(EventType::ON_CARD_PLAYED, state, card.get());

    // ==========================================
    // 步骤 4：善后排队
    // 在队列尾部追加善后动作：UseCardAction
    // 负责将卡牌从 limbo 移入最终归宿
    // ==========================================
    state.addAction(std::make_unique<UseCardAction>(card));

    // ==========================================
    // 步骤 5：通电执行
    // 调用 ActionSystem::executeUntilBlocked，瞬间清算所有队列
    // ==========================================
    ActionSystem::executeUntilBlocked(state, flow);

    return true;
}

// ==========================================
// chooseCard 实现（兼容接口）
// 
// 单张选牌，内部调用 chooseCards
// ==========================================
void PlayerActions::chooseCard(GameState& state, CombatFlow& flow, int choiceIndex) {
    chooseCards(state, flow, {choiceIndex});
}

// ==========================================
// chooseCards 实现（批量接口）
// 
// 智能路由：根据 Purpose，批量推入物理结算 Action
// 倒序推入队列，保证原本的先后顺序
// ==========================================
void PlayerActions::chooseCards(GameState& state, CombatFlow& flow, const std::vector<int>& choiceIndices) {
    if (state.currentPhase != StatePhase::WAITING_FOR_CARD_SELECTION) {
        return;  // 防御性编程：没让你选牌你别瞎选
    }

    // 检查选牌上下文是否存在
    if (!state.selectionCtx.has_value()) {
        STS_LOG(state, "[警告] 没有有效的选牌上下文！\n");
        return;
    }

    auto& ctx = state.selectionCtx.value();

    // 安全校验：AI 传来的数量合不合法？
    int chosenCount = static_cast<int>(choiceIndices.size());
    if (chosenCount < ctx.minSelection || chosenCount > ctx.maxSelection) {
        STS_LOG(state, "[警告] 选择的数量不合法！要求: " << ctx.minSelection 
                  << "-" << ctx.maxSelection << "，实际: " << chosenCount << "\n");
        return;
    }

    // 校验索引有效性
    for (int idx : choiceIndices) {
        if (idx < 0 || idx >= static_cast<int>(ctx.choices.size())) {
            STS_LOG(state, "[警告] 无效的选牌索引: " << idx << "\n");
            return;
        }
    }

    // ★ 批量路由：遍历 AI 选出的所有牌，倒序推入队列！
    // 为什么要倒序推入？因为 push_front 是插在队头，倒序插才能保证原本的先后顺序！
    for (auto it = choiceIndices.rbegin(); it != choiceIndices.rend(); ++it) {
        auto selectedCard = ctx.choices[*it];
        
        switch (ctx.purpose) {
            case SelectionPurpose::EXHAUST_FROM_HAND:
                state.addActionToFront(std::make_unique<SpecificCardExhaustAction>(selectedCard));
                break;
            case SelectionPurpose::MOVE_TO_HAND:
                state.addActionToFront(std::make_unique<MoveCardToHandAction>(selectedCard));
                break;
            case SelectionPurpose::DISCARD_FROM_HAND:
                state.addActionToFront(std::make_unique<SpecificCardDiscardAction>(selectedCard));
                break;
        }
    }

    // 输出选牌日志
    STS_LOG(state, "[选牌完成] 你选择了 " << chosenCount << " 张牌: ");
    for (int i = 0; i < chosenCount; ++i) {
        STS_LOG(state, ctx.choices[choiceIndices[i]]->id);
        if (i < chosenCount - 1) STS_LOG(state, ", ");
    }
    STS_LOG(state, "\n");

    // 善后：清空选项，解除冻结
    state.selectionCtx = std::nullopt;
    state.currentPhase = StatePhase::PLAYING_CARD;

    // 唤醒引擎，继续光速推演！
    ActionSystem::executeUntilBlocked(state, flow);
}

void PlayerActions::endTurn(GameState& state, CombatFlow& flow) {
    if (flow.currentState == CombatState::PLAYER_ACTION && 
        state.currentPhase == StatePhase::PLAYING_CARD &&
        state.isActionQueueEmpty()) {
        STS_LOG(state, "[外部输入] -> 玩家点击了【结束回合】\n");
        flow.currentState = CombatState::PLAYER_TURN_END;
    } else {
        STS_LOG(state, "[警告] 当前状态不允许结束回合，或动作尚未结算完毕！\n");
    }
}
