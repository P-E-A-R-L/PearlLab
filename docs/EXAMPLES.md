# PearlLab Usage Examples

## Table of Contents

1. [Basic Setup](#basic-setup)
2. [Environment Examples](#environment-examples)
3. [Agent Examples](#agent-examples)
4. [Explainability Examples](#explainability-examples)
5. [Visualization Examples](#visualization-examples)
6. [Custom Components](#custom-components)
7. [Advanced Usage](#advanced-usage)
8. [Troubleshooting](#troubleshooting)

## Basic Setup

### Environment Setup

```python
import torch
import numpy as np
from pearl.enviroments.GymRLEnv import GymRLEnv
from pearl.agents.SimpleDQN import DQNAgent
from pearl.methods.ShapExplainability import ShapExplainability
from pearl.provided.AssaultEnv import AssaultEnvShapMask

# Set device
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"Using device: {device}")
```

### Basic Pipeline

```python
# 1. Create environment
env = GymRLEnv(
    env_name="ALE/Assault-v5",
    stack_size=4,
    frame_skip=4,
    render_mode="rgb_array"
)

# 2. Load agent
agent = DQNAgent("models/dqn_assault_5m.pth")

# 3. Setup explainability method
mask = AssaultEnvShapMask()
explainer = ShapExplainability(device, mask)

# 4. Configure explainer
explainer.set(env)
explainer.prepare(agent)

# 5. Run analysis
obs = env.get_observations()
explanation = explainer.explain(obs)
quality_score = explainer.value(obs)

print(f"Explanation quality score: {quality_score:.4f}")
```

## Environment Examples

### Atari Environment with Custom Preprocessing

```python
import cv2
from pearl.enviroments.ObservationWrapper import ObservationWrapper

class CustomAtariWrapper(ObservationWrapper):
    def __init__(self, env):
        super().__init__(env)
        
    def observation(self, obs):
        # Convert to grayscale and resize
        if obs.ndim == 3:
            gray = cv2.cvtColor(obs, cv2.COLOR_RGB2GRAY)
        else:
            gray = obs
            
        resized = cv2.resize(gray, (84, 84))
        normalized = resized.astype(np.float32) / 255.0
        return np.expand_dims(normalized, axis=-1)
    
    def get_observations(self):
        return self.observation(self.env.get_observations())
    
    def reset(self, **kwargs):
        obs, info = self.env.reset(**kwargs)
        return self.observation(obs), info
    
    def step(self, action):
        obs, reward_dict, terminated, truncated, info = self.env.step(action)
        return self.observation(obs), reward_dict, terminated, truncated, info

# Usage
base_env = GymRLEnv("ALE/Assault-v5", stack_size=4, frame_skip=4)
custom_env = CustomAtariWrapper(base_env)
```

### Tabular Environment (Lunar Lander)

```python
# Lunar Lander environment
lunar_env = GymRLEnv(
    env_name="LunarLander-v2",
    tabular=True,
    normalize_observations=True
)

# Reset and get initial observation
obs, info = lunar_env.reset()
print(f"Observation shape: {obs.shape}")
print(f"Action space: {lunar_env.action_space}")
```

### Multi-Environment Setup

```python
# Create multiple environments for comparison
envs = {
    "assault": GymRLEnv("ALE/Assault-v5", stack_size=4, frame_skip=4),
    "space_invaders": GymRLEnv("ALE/SpaceInvaders-v5", stack_size=4, frame_skip=4),
    "lunar_lander": GymRLEnv("LunarLander-v2", tabular=True)
}

# Test each environment
for name, env in envs.items():
    obs, info = env.reset()
    print(f"{name}: observation shape = {obs.shape}")
```

## Agent Examples

### Loading Different Agent Types

```python
# Simple DQN Agent (Stable Baselines3)
dqn_agent = DQNAgent("models/dqn_assault_5m.pth")

# Custom PyTorch DQN Agent
from pearl.agents.TourchDQN import TorchDQN
import torch.nn as nn

class CustomDQN(nn.Module):
    def __init__(self, n_actions):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(8, 64),
            nn.ReLU(),
            nn.Linear(64, 64),
            nn.ReLU(),
            nn.Linear(64, n_actions)
        )
    
    def forward(self, x):
        return self.net(x)

# Load custom agent
model = CustomDQN(4)  # 4 actions for Lunar Lander
custom_agent = TorchDQN("models/custom_lunar_lander.pth", model, device)
```

### Agent Evaluation

```python
def evaluate_agent(agent, env, num_episodes=10):
    """Evaluate agent performance over multiple episodes."""
    total_rewards = []
    
    for episode in range(num_episodes):
        obs, info = env.reset()
        episode_reward = 0
        done = False
        
        while not done:
            action_probs = agent.predict(obs)
            action = np.argmax(action_probs)
            
            obs, reward_dict, terminated, truncated, info = env.step(action)
            done = terminated or truncated
            episode_reward += float(reward_dict['reward'])
        
        total_rewards.append(episode_reward)
        print(f"Episode {episode + 1}: Reward = {episode_reward:.2f}")
    
    avg_reward = np.mean(total_rewards)
    std_reward = np.std(total_rewards)
    print(f"Average reward: {avg_reward:.2f} ± {std_reward:.2f}")
    
    return total_rewards

# Evaluate agent
rewards = evaluate_agent(agent, env)
```

## Explainability Examples

### SHAP Analysis

```python
from pearl.methods.ShapExplainability import ShapExplainability
from pearl.provided.AssaultEnv import AssaultEnvShapMask

# Setup SHAP explainer
mask = AssaultEnvShapMask()
shap_explainer = ShapExplainability(device, mask)

# Configure explainer
shap_explainer.set(env)
shap_explainer.prepare(agent)

# Generate explanations
obs = env.get_observations()
shap_values = shap_explainer.explain(obs)
shap_score = shap_explainer.value(obs)

print(f"SHAP explanation shape: {shap_values.shape}")
print(f"SHAP quality score: {shap_score:.4f}")
```

### LIME Analysis

```python
from pearl.methods.LimeExplainability import LimeExplainability

# Setup LIME explainer
lime_explainer = LimeExplainability(device, mask)

# Configure explainer
lime_explainer.set(env)
lime_explainer.prepare(agent)

# Generate explanations
lime_explanation = lime_explainer.explain(obs)
lime_score = lime_explainer.value(obs)

print(f"LIME quality score: {lime_score:.4f}")
```

### Stability Analysis

```python
from pearl.methods.StabilityExplainability import StabilityExplainability

# Setup stability analyzer with different noise types
stability_gaussian = StabilityExplainability(
    device=device,
    noise_type='gaussian',
    noise_strength=0.01,
    num_samples=20,
    visualize=True
)

stability_salt_pepper = StabilityExplainability(
    device=device,
    noise_type='salt_pepper',
    noise_strength=0.01,
    num_samples=20,
    visualize=True
)

# Configure analyzers
stability_gaussian.set(env)
stability_gaussian.prepare(agent)

stability_salt_pepper.set(env)
stability_salt_pepper.prepare(agent)

# Run analysis
gaussian_score = stability_gaussian.value(obs)
salt_pepper_score = stability_salt_pepper.value(obs)

print(f"Gaussian noise stability: {gaussian_score:.4f}")
print(f"Salt & pepper noise stability: {salt_pepper_score:.4f}")
```

### LMUT Analysis

```python
from pearl.methods.LMUTExplainability import LinearModelUTreeExplainability

# Setup LMUT explainer
lmut_explainer = LinearModelUTreeExplainability(
    device=device,
    mask=mask,
    max_depth=5,
    min_samples_leaf=5,
    num_training_samples=1000
)

# Configure explainer
lmut_explainer.set(env)
lmut_explainer.prepare(agent)

# Collect training data and fit models
lmut_explainer.collect_training_data()
lmut_explainer.fit_models()

# Generate explanations
lmut_explanations = lmut_explainer.explain(obs)
lmut_scores = lmut_explainer.value(obs)

print(f"LMUT explanations: {len(lmut_explanations)}")
print(f"LMUT scores: {lmut_scores}")
```

### Tabular Explainability

```python
from pearl.methods.TabularLimeExplainability import TabularLimeExplainability
from pearl.methods.TabularShapExplainability import TabularShapExplainability
from pearl.provided.LunarLander import LunarLanderTabularMask

# Setup tabular environment and agent
tabular_env = GymRLEnv("LunarLander-v2", tabular=True)
tabular_agent = DQNAgent("models/lunar_lander_dqn.pth")

# Setup masks and explainers
tabular_mask = LunarLanderTabularMask()
feature_names = ["x_pos", "y_pos", "x_vel", "y_vel", "angle", "angular_vel", "leg1_contact", "leg2_contact"]

tabular_lime = TabularLimeExplainability(device, tabular_mask, feature_names)
tabular_shap = TabularShapExplainability(device, tabular_mask, feature_names)

# Configure explainers
tabular_lime.set(tabular_env)
tabular_lime.prepare(tabular_agent)

tabular_shap.set(tabular_env)
tabular_shap.prepare(tabular_agent)

# Generate explanations
tabular_obs = tabular_env.get_observations()
lime_explanation = tabular_lime.explain(tabular_obs)
shap_explanation = tabular_shap.explain(tabular_obs)

lime_score = tabular_lime.value(tabular_obs)
shap_score = tabular_shap.value(tabular_obs)

print(f"Tabular LIME score: {lime_score:.4f}")
print(f"Tabular SHAP score: {shap_score:.4f}")
```

## Visualization Examples

### Basic Visualization

```python
from visual import VisualizationMethod

# Get RGB visualization
rgb_viz = explainer.getVisualization(
    VisualizationMethod.RGB_ARRAY,
    params=ShapVisualizationParams(action=0)
)

# Get bar chart for tabular data
bar_viz = tabular_explainer.getVisualization(
    VisualizationMethod.BAR_CHART,
    params=TabularLimeVisualizationParams(action=0)
)

print(f"RGB visualization shape: {rgb_viz.shape}")
print(f"Bar chart data: {bar_viz}")
```

### Multiple Visualization Types

```python
def get_all_visualizations(explainer, obs, action=0):
    """Get all available visualizations for an explainer."""
    visualizations = {}
    
    for method in VisualizationMethod:
        if explainer.supports(method):
            try:
                # Create appropriate params object
                if method == VisualizationMethod.RGB_ARRAY:
                    params = ShapVisualizationParams(action=action)
                elif method == VisualizationMethod.BAR_CHART:
                    params = TabularLimeVisualizationParams(action=action)
                else:
                    params = None
                
                viz = explainer.getVisualization(method, params)
                visualizations[method] = viz
                
            except Exception as e:
                print(f"Failed to get {method} visualization: {e}")
    
    return visualizations

# Get all visualizations
all_viz = get_all_visualizations(explainer, obs)
for method, viz in all_viz.items():
    print(f"{method}: {type(viz)}")
```

### Custom Visualization

```python
import matplotlib.pyplot as plt

def plot_explanation_comparison(obs, explainers, action=0):
    """Compare explanations from different methods."""
    fig, axes = plt.subplots(1, len(explainers), figsize=(5*len(explainers), 5))
    
    if len(explainers) == 1:
        axes = [axes]
    
    for i, (name, explainer) in enumerate(explainers.items()):
        if explainer.supports(VisualizationMethod.RGB_ARRAY):
            viz = explainer.getVisualization(
                VisualizationMethod.RGB_ARRAY,
                params=ShapVisualizationParams(action=action)
            )
            
            axes[i].imshow(viz)
            axes[i].set_title(f"{name} Explanation")
            axes[i].axis('off')
    
    plt.tight_layout()
    plt.show()

# Compare different explainability methods
explainers = {
    "SHAP": shap_explainer,
    "LIME": lime_explainer
}

plot_explanation_comparison(obs, explainers)
```

## Custom Components

### Custom Environment

```python
import gymnasium as gym
from pearl.env import RLEnvironment

class CustomEnvironment(RLEnvironment):
    def __init__(self, env_name: str):
        super().__init__()
        self.env = gym.make(env_name)
        self.observation_space = self.env.observation_space
        self.action_space = self.env.action_space
        self.metadata = self.env.metadata
    
    def reset(self, seed=None, options=None):
        obs, info = self.env.reset(seed=seed, options=options)
        return obs, info
    
    def step(self, action):
        obs, reward, terminated, truncated, info = self.env.step(action)
        reward_dict = {"reward": np.array(reward, dtype=np.float32)}
        return obs, reward_dict, terminated, truncated, info
    
    def render(self, mode="human"):
        return self.env.render(mode=mode)
    
    def get_observations(self):
        return self.env.get_observations()
    
    def close(self):
        return self.env.close()

# Usage
custom_env = CustomEnvironment("CartPole-v1")
```

### Custom Agent

```python
import torch.nn as nn
from pearl.agent import RLAgent

class CustomAgent(RLAgent):
    def __init__(self, model_path: str, device: torch.device):
        super().__init__()
        self.device = device
        
        # Define model architecture
        self.q_net = nn.Sequential(
            nn.Linear(4, 64),
            nn.ReLU(),
            nn.Linear(64, 64),
            nn.ReLU(),
            nn.Linear(64, 2)
        ).to(device)
        
        # Load weights
        self.q_net.load_state_dict(torch.load(model_path, map_location=device))
        self.q_net.eval()
    
    def predict(self, observation):
        with torch.no_grad():
            obs_tensor = torch.tensor(observation, dtype=torch.float32, device=self.device)
            q_values = self.q_net(obs_tensor)
            probs = torch.softmax(q_values, dim=-1)
            return probs.cpu().numpy()
    
    def get_q_net(self):
        return self.q_net
    
    def close(self):
        del self.q_net

# Usage
custom_agent = CustomAgent("models/custom_model.pth", device)
```

### Custom Explainability Method

```python
from pearl.method import ExplainabilityMethod
from visual import VisualizationMethod

class CustomExplainability(ExplainabilityMethod):
    def __init__(self, device: torch.device, mask):
        super().__init__()
        self.device = device
        self.mask = mask
        self.env = None
        self.agent = None
    
    def set(self, env):
        self.env = env
    
    def prepare(self, agent):
        self.agent = agent
    
    def onStep(self, action):
        # Store action for later use
        self.last_action = action
    
    def onStepAfter(self, action, reward, done, info):
        # Store reward for analysis
        self.last_reward = reward
    
    def explain(self, obs):
        # Custom explanation logic
        # This is a simple example - replace with your method
        obs_tensor = torch.tensor(obs, dtype=torch.float32, device=self.device)
        with torch.no_grad():
            q_values = self.agent.get_q_net()(obs_tensor)
        
        # Create simple attribution based on Q-value gradients
        q_values.backward(torch.ones_like(q_values))
        
        # Get gradients from first layer
        gradients = obs_tensor.grad
        return gradients.cpu().numpy()
    
    def value(self, obs):
        # Custom quality metric
        explanation = self.explain(obs)
        self.mask.update(obs)
        scores = self.mask.compute(explanation)
        return float(scores[self.last_action])
    
    def supports(self, m: VisualizationMethod) -> bool:
        return m == VisualizationMethod.HEAT_MAP
    
    def getVisualizationParamsType(self, m: VisualizationMethod) -> type | None:
        return None
    
    def getVisualization(self, m: VisualizationMethod, params=None):
        if m == VisualizationMethod.HEAT_MAP:
            explanation = self.explain(self.env.get_observations())
            return np.mean(explanation, axis=0)  # Average across channels
        return None

# Usage
custom_explainer = CustomExplainability(device, mask)
custom_explainer.set(env)
custom_explainer.prepare(agent)
```

### Custom Mask

```python
from pearl.mask import Mask

class CustomMask(Mask):
    def __init__(self, action_space_size: int):
        super().__init__(action_space_size)
        self.last_obs = None
    
    def update(self, observation):
        # Store observation for later use
        self.last_obs = observation.copy()
    
    def compute(self, values):
        # Custom masking logic
        # This example creates a simple attention mask
        if self.last_obs is None:
            return np.ones(self.action_space, dtype=np.float32)
        
        # Create mask based on observation intensity
        obs_intensity = np.mean(np.abs(self.last_obs))
        mask = np.ones(self.action_space, dtype=np.float32) * obs_intensity
        
        # Normalize
        mask = mask / np.sum(mask) if np.sum(mask) > 0 else mask
        
        return mask

# Usage
custom_mask = CustomMask(7)  # 7 actions for Assault
```

## Advanced Usage

### Batch Processing

```python
def batch_explain(explainer, observations, batch_size=32):
    """Process multiple observations in batches."""
    explanations = []
    
    for i in range(0, len(observations), batch_size):
        batch = observations[i:i+batch_size]
        batch_explanations = []
        
        for obs in batch:
            explanation = explainer.explain(obs)
            batch_explanations.append(explanation)
        
        explanations.extend(batch_explanations)
    
    return explanations

# Collect multiple observations
observations = []
for _ in range(10):
    obs = env.get_observations()
    observations.append(obs)
    env.step(env.action_space.sample())

# Batch process explanations
batch_explanations = batch_explain(explainer, observations)
```

### Multi-Agent Analysis

```python
def compare_agents(agents, env, explainer, num_episodes=5):
    """Compare multiple agents using explainability methods."""
    results = {}
    
    for agent_name, agent in agents.items():
        print(f"Analyzing {agent_name}...")
        
        # Configure explainer for this agent
        explainer.prepare(agent)
        
        episode_scores = []
        for episode in range(num_episodes):
            obs, info = env.reset()
            episode_score = 0
            
            for step in range(100):  # Max 100 steps per episode
                # Get explanation
                explanation = explainer.explain(obs)
                score = explainer.value(obs)
                episode_score += score
                
                # Take action
                action_probs = agent.predict(obs)
                action = np.argmax(action_probs)
                obs, reward_dict, terminated, truncated, info = env.step(action)
                
                if terminated or truncated:
                    break
            
            episode_scores.append(episode_score)
        
        results[agent_name] = {
            'mean_score': np.mean(episode_scores),
            'std_score': np.std(episode_scores),
            'scores': episode_scores
        }
    
    return results

# Compare multiple agents
agents = {
    "DQN_1M": DQNAgent("models/dqn_assault_1m.pth"),
    "DQN_5M": DQNAgent("models/dqn_assault_5m.pth"),
    "Custom": custom_agent
}

comparison_results = compare_agents(agents, env, explainer)
for name, result in comparison_results.items():
    print(f"{name}: {result['mean_score']:.4f} ± {result['std_score']:.4f}")
```

### Real-time Analysis

```python
import time
import threading
from queue import Queue

class RealTimeAnalyzer:
    def __init__(self, env, agent, explainer, update_interval=0.1):
        self.env = env
        self.agent = agent
        self.explainer = explainer
        self.update_interval = update_interval
        self.running = False
        self.results_queue = Queue()
    
    def start(self):
        self.running = True
        self.analysis_thread = threading.Thread(target=self._analysis_loop)
        self.analysis_thread.start()
    
    def stop(self):
        self.running = False
        if hasattr(self, 'analysis_thread'):
            self.analysis_thread.join()
    
    def _analysis_loop(self):
        while self.running:
            try:
                obs = self.env.get_observations()
                explanation = self.explainer.explain(obs)
                score = self.explainer.value(obs)
                
                self.results_queue.put({
                    'explanation': explanation,
                    'score': score,
                    'timestamp': time.time()
                })
                
                time.sleep(self.update_interval)
                
            except Exception as e:
                print(f"Analysis error: {e}")
    
    def get_latest_results(self):
        if not self.results_queue.empty():
            return self.results_queue.get()
        return None

# Usage
analyzer = RealTimeAnalyzer(env, agent, explainer)
analyzer.start()

# In main loop
for _ in range(100):
    results = analyzer.get_latest_results()
    if results:
        print(f"Score: {results['score']:.4f}")
    time.sleep(0.05)

analyzer.stop()
```

## Troubleshooting

### Common Issues

#### 1. CUDA Out of Memory

```python
# Solution: Use CPU or reduce batch size
device = torch.device("cpu")  # Force CPU usage

# Or reduce batch size in explainability methods
explainer = ShapExplainability(device, mask)
# Some explainers have batch_size parameters
```

#### 2. Model Loading Errors

```python
# Check model path and format
import os
model_path = "models/dqn_assault_5m.pth"
if not os.path.exists(model_path):
    print(f"Model not found: {model_path}")
    # Download or train model
```

#### 3. Environment Compatibility

```python
# Check environment compatibility
try:
    env = GymRLEnv("ALE/Assault-v5")
    obs, info = env.reset()
    print(f"Environment works: {obs.shape}")
except Exception as e:
    print(f"Environment error: {e}")
    # Try different environment or version
```

#### 4. Visualization Issues

```python
# Check visualization support
for method in VisualizationMethod:
    if explainer.supports(method):
        print(f"Supports {method}")
    else:
        print(f"Does not support {method}")

# Try different visualization methods
viz_methods = [VisualizationMethod.RGB_ARRAY, VisualizationMethod.HEAT_MAP]
for method in viz_methods:
    if explainer.supports(method):
        viz = explainer.getVisualization(method)
        print(f"Got {method} visualization")
```

### Debug Mode

```python
import logging

# Enable debug logging
logging.basicConfig(level=logging.DEBUG)

# Add debug prints to custom components
class DebugExplainability(ExplainabilityMethod):
    def explain(self, obs):
        print(f"Debug: Explaining observation of shape {obs.shape}")
        result = super().explain(obs)
        print(f"Debug: Explanation shape {result.shape}")
        return result
```

### Performance Profiling

```python
import cProfile
import pstats

def profile_explanation(explainer, obs, num_runs=100):
    """Profile explanation performance."""
    profiler = cProfile.Profile()
    profiler.enable()
    
    for _ in range(num_runs):
        explanation = explainer.explain(obs)
    
    profiler.disable()
    stats = pstats.Stats(profiler)
    stats.sort_stats('cumulative')
    stats.print_stats(10)  # Top 10 functions

# Profile explanation method
profile_explanation(explainer, obs)
```

This comprehensive examples guide should help you get started with PearlLab and explore its various capabilities. Each example can be modified and extended to suit your specific research needs. 