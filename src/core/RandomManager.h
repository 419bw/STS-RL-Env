#pragma once

#include <random>

// ==========================================
// RandomManager - 隔离随机数生成器
//
// 核心原则：绝对局部确定性
// - 每个用途使用独立的 RNG，互不干扰
// - 保证多线程环境下可复现
// - 严禁使用全局 RNG
//
// RNG 隔离策略：
// - shuffleRng: 专用于洗牌
// - monsterRng: 专用于怪物意图生成
// - combatRng: 专用于战斗内随机效果
// - mapAndDropRng: 专用于局外大盘掉落
// ==========================================

struct RandomManager {
    // 洗牌专用 RNG
    std::mt19937 shuffleRng;

    // 怪物意图专用 RNG
    std::mt19937 monsterRng;

    // 战斗随机效果 RNG
    std::mt19937 combatRng;

    // 地图和掉落专用 RNG
    std::mt19937 mapAndDropRng;

    // ==========================================
    // 构造函数
    // @masterSeed: 主种子，初始化所有 RNG
    // ==========================================
    RandomManager(unsigned int masterSeed = 1337) {
        std::mt19937 masterGenerator(masterSeed);
        shuffleRng.seed(masterGenerator());
        monsterRng.seed(masterGenerator());
        combatRng.seed(masterGenerator());
        mapAndDropRng.seed(masterGenerator());
    }
};
