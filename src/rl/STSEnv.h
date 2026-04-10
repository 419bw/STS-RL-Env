#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <random>
#include "src/engine/GameEngine.h"
#include "src/flow/CombatFlow.h"

// ==========================================
// STSEnv - Gym-style RL 训练环境
//
// 封装 STS_CPP 引擎为标准 Gym 接口：
// - reset() → observation
// - step(action) → (observation, reward, done, info)
// - getObservation() → 固定长度浮点向量
// - getLegalActionMask() → 合法动作掩码
//
// 场景：单怪物（JawWorm）对战
// 牌库：Strike x5, DeadlyPoison x3, Whirlwind x2, Shuriken x3
//
// Observation Space (72 dims):
//   Player(7) + Monster(11) + Hand×10(50) + Meta(4)
//
// Action Space (11):
//   0: EndTurn
//   1~10: Play hand[0]~hand[9]
// ==========================================

struct StepResult {
    std::vector<float> observation;
    float reward;
    bool done;
    std::map<std::string, float> info;
};

class STSEnv {
public:
    explicit STSEnv(int seed = 0);

    std::vector<float> reset();
    StepResult step(int action);
    std::vector<float> getObservation() const;
    std::vector<bool> getLegalActionMask() const;

    int getActionSpaceSize() const { return ACTION_SPACE_SIZE; }
    int getObservationSpaceSize() const { return OBS_SPACE_SIZE; }

private:
    static constexpr int OBS_SPACE_SIZE = 72;
    static constexpr int ACTION_SPACE_SIZE = 11;
    static constexpr int MAX_HAND_SIZE = 10;
    static constexpr int MAX_TICKS = 200;

    std::unique_ptr<GameEngine> engine_;
    std::unique_ptr<CombatFlow> flow_;
    int baseSeed_;
    int episodeSeed_;
    int stepCount_;
    float prevPlayerHp_;
    float prevMonsterHp_;
    std::mt19937 seedRng_;

    void setupCombat();
    bool tickUntilDecision();
    bool isBattleOver() const;
    float computeReward(bool done);
    std::shared_ptr<Character> resolveTarget(
        const std::shared_ptr<AbstractCard>& card) const;

    float getPowerAmount(const Character& c, const std::string& name) const;
    void encodePlayer(std::vector<float>& obs) const;
    void encodeMonster(std::vector<float>& obs) const;
    void encodeHand(std::vector<float>& obs) const;
    void encodeMeta(std::vector<float>& obs) const;
};
