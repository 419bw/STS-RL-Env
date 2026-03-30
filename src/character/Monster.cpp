#include "Character.h"
#include "src/engine/GameEngine.h"
#include "src/action/Actions.h"
#include "src/power/Powers.h"
#include "src/utils/Logger.h"
#include "src/intent/Intent.h"

Monster::Monster(std::string n, int hp) : Character(n, hp), deathReported(false), brain(nullptr) {}

void Monster::rollIntent(GameEngine& engine) {
    auto& state = *engine.combatState;
    if (brain) {
        Intent oldIntent = currentIntent;
        currentIntent = brain->decide(state, this);
        if (!currentIntent.target) {
            currentIntent.target = state.player.get();
        }
        STS_LOG(state, "    [" << name << "] 刷新意图: " << Intent_DebugString(currentIntent) << "\n");
    }
}

void Monster::setBrain(IntentBrainPtr b) {
    brain = b;
}

Intent Monster::getVisibleIntent(const CombatState& combat) const {
    Intent intent = currentIntent;
    if (combat.player && !combat.player->canSeeEnemyIntents()) {
        intent.type = IntentType::UNKNOWN;
        intent.visible = false;
        intent.base_damage = -1;
        STS_LOG(combat, "    [" << name << "] 意图被遮挡，显示为 UNKNOWN\n");
    } else {
        intent.visible = true;
    }
    return intent;
}

void Monster::takeTurn(GameEngine& engine) {
    auto& state = *engine.combatState;
    STS_LOG(state, "    [" << name << "] 开始回合，意图: " << Intent_DebugString(currentIntent) << "\n");

    if (currentIntent.type == IntentType::ATTACK || currentIntent.type == IntentType::ATTACK_DEFEND) {
        if (!currentIntent.target || currentIntent.target->isDead()) {
            STS_LOG(state, "    [" << name << "] 攻击意图但目标为空或已死亡，跳过攻击\n");
        } else {
            auto targetPtr = currentIntent.target->shared_from_this();
            STS_LOG(state, "    [" << name << "] 攻击 " << currentIntent.target->name
                << " x" << currentIntent.hit_count << " (伤害:" << currentIntent.base_damage << ")\n");
            for (int i = 0; i < currentIntent.hit_count; ++i) {
                engine.actionManager.addAction(std::make_unique<DamageAction>(
                    shared_from_this(),
                    targetPtr,
                    currentIntent.base_damage));
            }
        }
    }

    if (currentIntent.type == IntentType::DEFEND || currentIntent.type == IntentType::ATTACK_DEFEND) {
        STS_LOG(state, "    [" << name << "] 防御，获得护盾: " << currentIntent.effect_value << "\n");
        engine.actionManager.addAction(std::make_unique<GainBlockAction>(
            shared_from_this(),
            currentIntent.effect_value));
    }

    if (currentIntent.type == IntentType::BUFF || currentIntent.type == IntentType::DEBUFF) {
        STS_LOG(state, "    [" << name << "] 执行特殊意图: "
            << (currentIntent.type == IntentType::BUFF ? "BUFF" : "DEBUFF") << "\n");
        this->executeSpecialIntent(engine);
    }
}
