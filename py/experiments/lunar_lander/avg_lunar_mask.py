import gymnasium as gym
import torch
import numpy as np
import torch.nn as nn
from lime.lime_tabular import LimeTabularExplainer
import torch.nn.functional as F
from tqdm import tqdm

class DQN(nn.Module):
    def __init__(self, n_observations, n_actions):
        super().__init__()
        self.layer1 = nn.Linear(n_observations, 128)
        self.layer2 = nn.Linear(128, 128)
        self.layer3 = nn.Linear(128, n_actions)

    def forward(self, x):
        x = F.relu(self.layer1(x))
        x = F.relu(self.layer2(x))
        return self.layer3(x)
    
class TorchDQN:
    def __init__(self, model_path: str, module, device):    
        self.m_agent = module.to(device)
        self.m_agent.load_state_dict(torch.load(model_path, map_location=device))
        self.m_agent.eval()
        self.q_net = self.m_agent.net if hasattr(self.m_agent, 'net') else self.m_agent.network if hasattr(self.m_agent, 'network') else self.m_agent
        self.device = device

    def predict(self, observation):
        self.q_net.eval()
        observation = torch.as_tensor(observation, dtype=torch.float, device=self.device)
        with torch.no_grad():
            q_vals = self.m_agent(observation)
            q_vals = q_vals.cpu().numpy() if q_vals.is_cuda else q_vals.numpy()
            q_vals = np.exp(q_vals - np.max(q_vals)) / np.sum(np.exp(q_vals - np.max(q_vals)))
            return q_vals.reshape(-1)

    def get_q_net(self):
        return self.q_net

# Setup
env = gym.make("LunarLander-v3")# , render_mode="human")
obs_dim = env.observation_space.shape[0]
n_actions = env.action_space.n
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# Load trained agent
agent = TorchDQN(
    "lunar_lander_pretrained_judge.pth",
    DQN(obs_dim, n_actions),
    device
)

# Prepare LIME
feature_names = ["x_pos", "y_pos", "x_vel", "y_vel", "angle", "angular_vel", "leg1", "leg2"]
lime_explainer = LimeTabularExplainer(
    training_data=np.zeros((1, len(feature_names))),
    feature_names=feature_names,
    class_names=[f"Action {i}" for i in range(n_actions)],
    mode="classification"
)

def agent_fn(obs_batch):
    return np.array([agent.predict(obs) for obs in obs_batch])

# Parameters
num_episodes = 100
max_steps    = 1000
accumulated_vectors = np.zeros((n_actions, obs_dim))
counts = np.zeros(n_actions)

# Run episodes
for ep in tqdm(range(num_episodes), desc="Explaining episodes"):
    obs, _ = env.reset()
    for step in tqdm(range(max_steps), desc=f"Episode {ep + 1} steps"):
        exp = lime_explainer.explain_instance(
            data_row=obs,
            predict_fn=agent_fn,
            num_features=obs_dim,
            labels=list(range(n_actions)), 
            num_samples=100
        )
        
        for action in range(n_actions):
            explanation = exp.as_map()[action]
            vec = np.zeros(obs_dim)
            for idx, weight in explanation:
                vec[idx] = abs(weight)
            if vec.sum() > 0:
                vec /= vec.sum()
                accumulated_vectors[action] += vec
                counts[action] += 1
        
        action = np.argmax(agent.predict(obs))
        obs, reward, terminated, truncated, _ = env.step(action)
        if terminated or truncated:
            break

# Average over explanations
averaged_vectors = np.zeros_like(accumulated_vectors)
for a in range(n_actions):
    if counts[a] > 0:
        averaged_vectors[a] = accumulated_vectors[a] / counts[a]

print("\n--- Averaged LIME Explanation Vectors (one per action) ---")
for a in range(n_actions):
    print(f"Action {a}: {averaged_vectors[a]}")

if __name__ == "__main__":
    pass
