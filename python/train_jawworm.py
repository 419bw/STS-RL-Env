import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "cmake_build", "bin"))

from sb3_contrib import MaskablePPO
from sb3_contrib.common.maskable.utils import get_action_masks
from stable_baselines3.common.callbacks import BaseCallback
from stable_baselines3.common.vec_env import SubprocVecEnv
from sts_gym_wrapper import STSJawWormEnv


class RewardCallback(BaseCallback):
    def __init__(self, verbose=0):
        super().__init__(verbose)
        self.episode_rewards = []
        self.current_reward = 0.0

    def _on_step(self):
        self.current_reward += self.locals["rewards"][0]
        if self.locals["dones"][0]:
            self.episode_rewards.append(self.current_reward)
            if len(self.episode_rewards) % 100 == 0:
                avg = sum(self.episode_rewards[-100:]) / 100
                print(f"Episode {len(self.episode_rewards)}, avg reward (100): {avg:.3f}")
            self.current_reward = 0.0
        return True


def make_env(rank):
    def _init():
        return STSJawWormEnv(seed=rank + int(1e6))
    return _init


def main():
    n_envs = 4
    vec_env = SubprocVecEnv([make_env(i) for i in range(n_envs)])

    model = MaskablePPO(
        "MlpPolicy",
        vec_env,
        verbose=1,
        n_steps=2048,
        batch_size=64,
        learning_rate=3e-4,
        gamma=0.99,
        ent_coef=0.01,
    )

    callback = RewardCallback()
    model.learn(total_timesteps=1_000_000, callback=callback)

    save_path = os.path.join(os.path.dirname(__file__), "ppo_jawworm")
    model.save(save_path)
    print(f"Model saved to {save_path}")

    vec_env.close()


if __name__ == "__main__":
    main()
