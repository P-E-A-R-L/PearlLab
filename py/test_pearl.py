import cv2
import gymnasium as gym
import torch
import numpy as np

import torch.nn as nn
from tqdm import tqdm

from pearl.agents.TourchDQN import TorchDQN
from pearl.enviroments.GymRLEnv import GymRLEnv
from pearl.enviroments.ObservationWrapper import ObservationWrapper
from pearl.provided.AssaultEnv import AssaultEnvShapMask
from pearl.methods.ShapExplainability import ShapExplainability


class DQN(nn.Module):
    def __init__(self, n_actions: int):
        super().__init__()
        self.net = nn.Sequential(
            nn.Conv2d(4, 32, kernel_size=8, stride=4), nn.ReLU(),
            nn.Conv2d(32, 64, kernel_size=4, stride=2),      nn.ReLU(),
            nn.Conv2d(64, 64, kernel_size=3, stride=1),      nn.ReLU(),
            nn.Flatten(),
            nn.Linear(7*7*64, 512),                          nn.ReLU(),
            nn.Linear(512, n_actions)
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        # x: (N, H, W, C) -> (N, C, H, W)
        # print(x.shape)
        return self.net(x)

def preprocess(obs):
    gray = cv2.cvtColor(obs, cv2.COLOR_RGB2GRAY)
    resized = cv2.resize(gray, (84, 84), interpolation=cv2.INTER_AREA)
    return resized

class Wrapper(ObservationWrapper):
    def __init__(self, env):
        super().__init__(env)
        self.observation_space = gym.spaces.Box(low=0, high=255, shape=(84, 84, 1), dtype=np.uint8)

    def observation(self, obs):
        state_t = np.expand_dims(obs, axis=0)
        state_t = np.transpose(state_t, (0, 3, 1, 2))
        state_t = state_t.astype(np.double) / 255.0
        return state_t

    def get_observations(self):
        return self.observation(self.env.get_observations())

    def reset(self, **kwargs):
        obs, info = self.env.reset(**kwargs)
        return self.observation(obs), info

    def step(self, action):
        obs, reward_dict, terminated, truncated, info = self.env.step(action)
        return self.observation(obs), reward_dict, terminated, truncated, info


def cudaDevice():
    return torch.device("cuda" if torch.cuda.is_available() else "cpu")

if __name__ == "__main__":
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    mask = AssaultEnvShapMask()
    explainer = ShapExplainability(device, mask)
    env = Wrapper(GymRLEnv(
        env_name='ALE/Assault-v5',
        stack_size=4,
        frame_skip=4,
        render_mode='rgb_array',
        observation_preprocessing=preprocess,
    ))

    n_actions = env.action_space.n

    policy_net_good = DQN(n_actions)
    agent_good = TorchDQN('models/models/dqn_assault_5m.pth', policy_net_good, device)

    policy_net_bad = DQN(n_actions)
    agent_bad = TorchDQN('models/models/dqn_assault_1k.pth', policy_net_bad, device)

    agents = [agent_good, agent_bad]
    env.reset()
    explainer.set(env)
    explainer.prepare(agents)
    scores = [0, 0]

    for i in tqdm(range(2000)):  # max 2000 steps for now
        obs = env.get_observations()
        obs_tensor = torch.as_tensor(obs, dtype=torch.float, device=device)

        new_scores = explainer.value(obs)
        scores[0] += new_scores[0]
        scores[1] += new_scores[1]

        best_agent = np.argmax(scores)
        agent = agents[best_agent]

        action = agent.predict(obs_tensor)
        state, reward_dict, terminated, truncated, info = env.step(action)
        if terminated:
            break

    print(f"5M Model: {float(scores[0])}     1K Model: {float(scores[1])}")

