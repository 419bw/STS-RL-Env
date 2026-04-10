#include "src/rl/STSEnv.h"
#include "src/action/PlayerActions.h"
#include "src/card/Cards.h"
#include "src/character/monster/JawWorm.h"
#include "src/rules/BasicRules.h"
#include "src/power/AbstractPower.h"
#include "src/core/Types.h"

STSEnv::STSEnv(int seed)
    : baseSeed_(seed), episodeSeed_(seed), stepCount_(0),
      prevPlayerHp_(0.0f), prevMonsterHp_(0.0f), seedRng_(seed) {}

// ==========================================
// reset - 初始化新战斗
//
// 完整重建 GameEngine 以避免状态残留
// 禁用日志以最大化推演速度
// ==========================================
std::vector<float> STSEnv::reset() {
    episodeSeed_ = seedRng_();
    stepCount_ = 0;

    engine_ = std::make_unique<GameEngine>();
    engine_->startNewRun(episodeSeed_);
    engine_->runState->enableLogging = false;

    for (int i = 0; i < 5; ++i)
        engine_->runState->masterDeck.push_back(std::make_shared<StrikeCard>());
    for (int i = 0; i < 5; ++i)
        engine_->runState->masterDeck.push_back(std::make_shared<DefendCard>());
    engine_->runState->masterDeck.push_back(std::make_shared<PainCard>());

    setupCombat();

    prevPlayerHp_ = static_cast<float>(engine_->combatState->player->current_hp);
    prevMonsterHp_ = static_cast<float>(engine_->combatState->monsters[0]->current_hp);

    return getObservation();
}

void STSEnv::setupCombat() {
    auto jawWorm = std::make_shared<JawWorm>(0);
    engine_->startCombat(jawWorm);
    engine_->combatState->enableLogging = false;

    BasicRules::registerRules(*engine_);

    flow_ = std::make_unique<CombatFlow>();
    tickUntilDecision();
}

// ==========================================
// tickUntilDecision - 推进引擎直到需要玩家决策
//
// 返回 true 表示战斗结束
// 安全上限 MAX_TICKS 防止异常死循环
// ==========================================
bool STSEnv::tickUntilDecision() {
    int ticks = 0;
    while (ticks++ < MAX_TICKS) {
        if (!engine_->combatState) return true;
        if (engine_->combatState->isPlayerDead || engine_->combatState->isMonsterDead) return true;
        if (flow_->getCurrentPhase() == BattlePhase::BATTLE_END) return true;

        if (flow_->getCurrentPhase() == BattlePhase::PLAYER_ACTION &&
            engine_->actionManager.isQueueEmpty()) {
            return false;
        }

        flow_->tick(*engine_);
    }
    return true;
}

bool STSEnv::isBattleOver() const {
    if (!engine_->combatState) return true;
    if (engine_->combatState->isPlayerDead || engine_->combatState->isMonsterDead) return true;
    if (flow_->getCurrentPhase() == BattlePhase::BATTLE_END) return true;
    return false;
}

// ==========================================
// step - 执行一个动作
//
// action=0: EndTurn → tick 直到下一个决策点
// action=1~10: Play hand[action-1] → 立即返回
//
// action 合法性由 Gymnasium MaskablePPO 保证，防御性加一层 clamp
// 无效 action 视为 no-op（不推进状态，不扣奖励）
// ==========================================
StepResult STSEnv::step(int action) {
    bool done = false;

    if (action < 0 || action >= ACTION_SPACE_SIZE) {
        action = 0;
    }

    if (action == 0) {
        PlayerActions::endTurn(*engine_, *flow_);
        done = tickUntilDecision();
    } else {
        int handIndex = action - 1;
        auto& hand = engine_->combatState->hand;
        if (handIndex >= 0 && handIndex < static_cast<int>(hand.size())) {
            auto& card = hand[handIndex];
            auto target = resolveTarget(card);
            PlayerActions::playCard(*engine_, *flow_, card, target);
        }
        done = isBattleOver();
    }

    stepCount_++;
    float reward = computeReward(done);
    auto obs = getObservation();

    std::map<std::string, float> info;
    if (engine_->combatState) {
        info["player_hp"] = static_cast<float>(engine_->combatState->player->current_hp);
        info["monster_hp"] = static_cast<float>(engine_->combatState->monsters[0]->current_hp);
        info["turn"] = static_cast<float>(engine_->combatState->turnCount);
    }

    return {obs, reward, done, info};
}

// ==========================================
// resolveTarget - 单怪物场景的目标解析
//
// ENEMY → JawWorm (如果存活)
// ALL_ENEMY / RANDOM / NONE → nullptr (卡牌内部处理)
// SELF → player
// ==========================================
std::shared_ptr<Character> STSEnv::resolveTarget(
    const std::shared_ptr<AbstractCard>& card) const {
    switch (card->targetType) {
        case CardTarget::ENEMY: {
            for (auto& m : engine_->combatState->monsters) {
                if (!m->isDead()) return m;
            }
            return nullptr;
        }
        case CardTarget::SELF:
            return engine_->combatState->player;
        default:
            return nullptr;
    }
}

// ==========================================
// computeReward - Dense + Sparse 混合奖励
//
// 造成伤害 +1.0, 受到伤害 -2.0 (2倍惩罚驱动最小战损)
// 胜利 +1.0, 失败 -1.0
// 每步 -0.01 鼓励速战速决
// ==========================================
float STSEnv::computeReward(bool done) {
    float reward = 0.0f;
    if (!engine_->combatState) return 0.0f;

    auto& player = engine_->combatState->player;
    auto& monster = engine_->combatState->monsters[0];

    float monsterDmg = (prevMonsterHp_ - monster->current_hp) / monster->max_hp;
    float playerDmg = (prevPlayerHp_ - player->current_hp) / player->max_hp;
    reward += monsterDmg * 1.0f;
    reward -= playerDmg * 2.0f;

    if (done) {
        if (engine_->combatState->isMonsterDead) reward += 1.0f;
        if (engine_->combatState->isPlayerDead) reward -= 1.0f;
    }

    reward -= 0.01f;

    prevPlayerHp_ = static_cast<float>(player->current_hp);
    prevMonsterHp_ = static_cast<float>(monster->current_hp);

    return reward;
}

// ==========================================
// getLegalActionMask - 合法动作掩码
//
// action=0 (EndTurn): 始终合法
// action=i+1 (Play hand[i]): 需要卡牌存在 + 费用足够 + 目标合法
// ==========================================
std::vector<bool> STSEnv::getLegalActionMask() const {
    std::vector<bool> mask(ACTION_SPACE_SIZE, false);
    mask[0] = true;

    if (!engine_->combatState) return mask;

    auto& hand = engine_->combatState->hand;
    auto& player = engine_->combatState->player;

    for (int i = 0; i < MAX_HAND_SIZE; ++i) {
        if (i >= static_cast<int>(hand.size())) break;

        auto& card = hand[i];
        bool canAfford = (card->cost == -1) || (card->cost <= player->getEnergy());

        bool hasTarget = true;
        if (card->targetType == CardTarget::ENEMY) {
            hasTarget = false;
            for (auto& m : engine_->combatState->monsters) {
                if (!m->isDead()) { hasTarget = true; break; }
            }
        }

        mask[i + 1] = canAfford && hasTarget;
    }

    return mask;
}

// ==========================================
// Observation 编码
// ==========================================

float STSEnv::getPowerAmount(const Character& c, const std::string& name) const {
    auto p = c.getPower(name);
    return p ? static_cast<float>(p->getAmount()) : 0.0f;
}

void STSEnv::encodePlayer(std::vector<float>& obs) const {
    auto& p = *engine_->combatState->player;
    obs.push_back(static_cast<float>(p.current_hp) / p.max_hp);
    obs.push_back(std::min(static_cast<float>(p.block) / p.max_hp, 1.0f));
    obs.push_back(static_cast<float>(p.getEnergy()) / 3.0f);
    obs.push_back(getPowerAmount(p, "力量") / 999.0f);
    obs.push_back(getPowerAmount(p, "敏捷") / 999.0f);
    obs.push_back(getPowerAmount(p, "易伤") / 999.0f);
    obs.push_back(getPowerAmount(p, "中毒") / 999.0f);
}

void STSEnv::encodeMonster(std::vector<float>& obs) const {
    auto& m = *engine_->combatState->monsters[0];
    obs.push_back(static_cast<float>(m.current_hp) / m.max_hp);
    obs.push_back(std::min(static_cast<float>(m.block) / m.max_hp, 1.0f));
    obs.push_back(m.isDead() ? 1.0f : 0.0f);

    auto intent = m.getRealIntent();
    auto it = intent.type;
    obs.push_back((it == IntentType::ATTACK || it == IntentType::ATTACK_DEFEND ||
                   it == IntentType::ATTACK_DEBUFF) ? 1.0f : 0.0f);
    obs.push_back((it == IntentType::DEFEND || it == IntentType::ATTACK_DEFEND) ? 1.0f : 0.0f);
    obs.push_back(it == IntentType::BUFF ? 1.0f : 0.0f);
    obs.push_back(static_cast<float>(intent.base_damage) / 50.0f);
    obs.push_back(static_cast<float>(intent.effect_value) / 20.0f);

    obs.push_back(getPowerAmount(m, "力量") / 999.0f);
    obs.push_back(getPowerAmount(m, "易伤") / 999.0f);
    obs.push_back(getPowerAmount(m, "中毒") / 999.0f);
}

// ==========================================
// encodeHand - 手牌编码 (10 slots × 5 features = 50 dims)
//
// 每个槽位: [has_card, is_attack, is_skill, cost/3, is_playable]
// 空槽位用零填充
// X费牌 cost=-1 编码为 1.0 (消耗全部能量)
// ==========================================
void STSEnv::encodeHand(std::vector<float>& obs) const {
    auto& hand = engine_->combatState->hand;
    auto& player = engine_->combatState->player;

    for (int i = 0; i < MAX_HAND_SIZE; ++i) {
        if (i < static_cast<int>(hand.size())) {
            auto& card = hand[i];
            obs.push_back(1.0f);
            obs.push_back(card->type == CardType::ATTACK ? 1.0f : 0.0f);
            obs.push_back(card->type == CardType::SKILL ? 1.0f : 0.0f);
            float costNorm = (card->cost == -1) ? 1.0f : static_cast<float>(card->cost) / 3.0f;
            obs.push_back(costNorm);

            bool canAfford = (card->cost == -1) || (card->cost <= player->getEnergy());
            bool hasTarget = true;
            if (card->targetType == CardTarget::ENEMY) {
                hasTarget = false;
                for (auto& m : engine_->combatState->monsters) {
                    if (!m->isDead()) { hasTarget = true; break; }
                }
            }
            obs.push_back((canAfford && hasTarget) ? 1.0f : 0.0f);
        } else {
            obs.push_back(0.0f);
            obs.push_back(0.0f);
            obs.push_back(0.0f);
            obs.push_back(0.0f);
            obs.push_back(0.0f);
        }
    }
}

void STSEnv::encodeMeta(std::vector<float>& obs) const {
    auto& state = *engine_->combatState;
    obs.push_back(static_cast<float>(state.turnCount) / 50.0f);
    obs.push_back(static_cast<float>(state.drawPile.size()) / 20.0f);
    obs.push_back(static_cast<float>(state.discardPile.size()) / 20.0f);
    obs.push_back(state.isPlayerTurn ? 1.0f : 0.0f);
}

std::vector<float> STSEnv::getObservation() const {
    std::vector<float> obs;
    obs.reserve(OBS_SPACE_SIZE);
    if (!engine_->combatState) {
        obs.resize(OBS_SPACE_SIZE, 0.0f);
        return obs;
    }
    encodePlayer(obs);
    encodeMonster(obs);
    encodeHand(obs);
    encodeMeta(obs);
    return obs;
}
