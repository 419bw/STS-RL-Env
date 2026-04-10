import gymnasium as gym
import numpy as np
from gymnasium import spaces

import sts_env


class STSJawWormEnv(gym.Env):
    metadata = {"render_modes": []}

    def __init__(self, seed=0):
        super().__init__()
        self.env = sts_env.STSEnv(seed)
        obs_size = self.env.observation_space_size()
        act_size = self.env.action_space_size()

        self.observation_space = spaces.Box(
            low=-1.0, high=1.0, shape=(obs_size,), dtype=np.float32
        )
        self.action_space = spaces.Discrete(act_size)

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        if seed is not None:
            self.env = sts_env.STSEnv(seed)
        obs = self.env.reset()
        return np.array(obs, dtype=np.float32), {}

    def step(self, action):
        result = self.env.step(int(action))
        obs = np.array(result.observation, dtype=np.float32)
        return obs, float(result.reward), bool(result.done), False, dict(result.info)

    def get_legal_action_mask(self):
        return np.array(self.env.get_legal_action_mask(), dtype=np.bool_)
