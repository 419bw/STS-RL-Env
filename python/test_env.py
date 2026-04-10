import sys
sys.path.insert(0, r'j:\学习\项目\STS_CPP\cmake_build_rl')
import sts_env
import random

env = sts_env.STSEnv(42)
obs = env.reset()
total_reward = 0.0
steps = 0

while True:
    mask = env.get_legal_action_mask()
    legal = [i for i, v in enumerate(mask) if v]
    action = random.choice(legal)
    result = env.step(action)
    total_reward += result.reward
    steps += 1
    if result.done:
        print(f"Episode done in {steps} steps, total reward: {total_reward:.3f}")
        print(f"Player HP: {result.info['player_hp']:.0f}, Monster HP: {result.info['monster_hp']:.0f}")
        break
    if steps > 200:
        print("Timeout!")
        break

print("\n--- Running 10 episodes ---")
wins = 0
for ep in range(10):
    env = sts_env.STSEnv(ep * 1000)
    obs = env.reset()
    total_reward = 0.0
    steps = 0
    while True:
        mask = env.get_legal_action_mask()
        legal = [i for i, v in enumerate(mask) if v]
        action = random.choice(legal)
        result = env.step(action)
        total_reward += result.reward
        steps += 1
        if result.done:
            won = result.info['monster_hp'] <= 0
            if won:
                wins += 1
            print(f"  Ep {ep}: steps={steps}, reward={total_reward:.2f}, "
                  f"player_hp={result.info['player_hp']:.0f}, "
                  f"monster_hp={result.info['monster_hp']:.0f}, "
                  f"{'WIN' if won else 'LOSS'}")
            break
        if steps > 200:
            print(f"  Ep {ep}: TIMEOUT")
            break

print(f"\nWin rate: {wins}/10 ({wins*10}%)")
