# PearlLab: Reinforcement Learning Explainability Framework

## Overview

PearlLab is a comprehensive framework for analyzing and explaining reinforcement learning (RL) agent behavior through various explainability methods. It provides a unified interface for evaluating RL agents using techniques like SHAP, LIME, stability analysis, and custom masking approaches. The framework supports both image-based (Atari games) and tabular (Lunar Lander) environments.

### Key Features

- **Multiple Explainability Methods**: SHAP, LIME, Stability Analysis, Linear Model U-Tree (LMUT)
- **Environment Support**: Atari games (Assault, Space Invaders) and tabular environments (Lunar Lander)
- **Visualization**: Real-time visualization of explanations with heatmaps, bar charts, and RGB overlays
- **Custom Masking**: Domain-specific action masking for improved explanation quality
- **Modular Architecture**: Extensible design for adding new environments, agents, and explainability methods
- **GUI Interface**: ImGui-based graphical interface for interactive exploration

## Table of Contents

1. [Installation](#installation)
2. [Quick Start](#quick-start)
3. [Architecture](#architecture)
4. [API Reference](#api-reference)
5. [Usage Examples](#usage-examples)
6. [Contributing](#contributing)
7. [License](#license)

## Installation

### Prerequisites

- **Operating System**: macOS 13.0+ (primary support), Linux (experimental)
- **Python**: 3.8+
- **C++ Compiler**: Clang/GCC with C++20 support
- **CMake**: 3.28+
- **CUDA**: Optional, for GPU acceleration

### Dependencies

#### System Dependencies
```bash
# macOS (using Homebrew)
brew install cmake boost glfw yaml-cpp

# Ubuntu/Debian
sudo apt-get install cmake libboost-all-dev libglfw3-dev libyaml-cpp-dev
```

#### Python Dependencies
The framework uses a virtual environment. Install dependencies via:
```bash
# Create virtual environment
python3 -m venv pearl_env
source pearl_env/bin/activate

# Install Python dependencies
cd py/pearl
pip install -r requirements.txt
```

### Building from Source

1. **Clone the repository**:
```bash
git clone <repository-url>
cd PearlLab
```

2. **Configure the build**:
```bash
mkdir build
cd build
cmake ..
```

3. **Build the project**:
```bash
make -j$(nproc)
```

4. **Update configuration**:
Edit `config.yaml` to set your virtual environment path:
```yaml
version: 1.0-beta
window:
  width: 1280
  height: 720
venv: /path/to/your/pearl_env
```

## Quick Start

### Running the GUI Application

```bash
# From the project root
./PearlLab
```

### Using the Build Script

For macOS users, you can use the provided build script which handles the complete build process and launches the application:

```bash
# Make the script executable (first time only)
chmod +x script.sh

# Run the build script
./script.sh
```

The script performs the following steps:
1. **Cleans** the build directory
2. **Configures** the project with macOS-specific settings
3. **Builds** the PearlLab application
4. **Launches** the application with the Space Invaders project

**Note**: The script is configured for macOS with Homebrew. For other platforms, you may need to modify the script or use the manual build process.

### Basic Python Usage

```python
import torch
from pearl.enviroments.GymRLEnv import GymRLEnv
from pearl.agents.SimpleDQN import DQNAgent
from pearl.methods.ShapExplainability import ShapExplainability
from pearl.provided.AssaultEnv import AssaultEnvShapMask

# Initialize environment
env = GymRLEnv("ALE/Assault-v5", stack_size=4, frame_skip=4)

# Load agent
agent = DQNAgent("models/dqn_assault_5m.pth")

# Setup explainability method
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
mask = AssaultEnvShapMask()
explainer = ShapExplainability(device, mask)

# Configure and run
explainer.set(env)
explainer.prepare(agent)

# Get explanation
obs = env.get_observations()
explanation = explainer.explain(obs)
value = explainer.value(obs)
```

## Architecture

### Core Components

#### 1. Environment Layer (`pearl/env.py`)
- **RLEnvironment**: Abstract base class for RL environments
- **GymRLEnv**: Gymnasium-based environment wrapper
- **ObservationWrapper**: Base class for observation preprocessing

#### 2. Agent Layer (`pearl/agent.py`)
- **RLAgent**: Abstract base class for RL agents
- **DQNAgent**: Simple DQN agent implementation
- **TorchDQN**: PyTorch-based DQN agent

#### 3. Explainability Layer (`pearl/method.py`)
- **ExplainabilityMethod**: Abstract base class for explainability methods
- **ShapExplainability**: SHAP-based explanations
- **LimeExplainability**: LIME-based explanations
- **StabilityExplainability**: Noise-based stability analysis
- **LMUTExplainability**: Linear Model U-Tree explanations

#### 4. Masking Layer (`pearl/mask.py`)
- **Mask**: Abstract base class for action masking
- **AssaultEnvShapMask**: Domain-specific mask for Atari Assault
- **LunarLanderTabularMask**: Tabular mask for Lunar Lander

#### 5. Visualization Layer (`pearl/visualizable.py`)
- **Visualizable**: Abstract base class for visualization support
- **VisualizationMethod**: Enumeration of supported visualization types

### Data Flow

```
Environment → Agent → Explainability Method → Mask → Visualization
     ↓           ↓           ↓                ↓         ↓
  Observations → Q-Values → Attributions → Scores → Visual Output
```

### C++ Backend Architecture

The C++ backend provides:
- **Python Integration**: pybind11-based Python-C++ bindings
- **GUI Framework**: ImGui-based user interface
- **OpenGL Rendering**: Hardware-accelerated visualization
- **Project Management**: YAML-based configuration and project persistence

## API Reference

### Core Classes

#### RLEnvironment

```python
class RLEnvironment(Visualizable):
    """Abstract base class for RL environments."""
    
    @abstractmethod
    def reset(self, seed: Optional[int] = None, 
              options: Optional[Dict[str, Any]] = None) -> Tuple[Any, Dict[str, Any]]:
        """Reset environment to initial state."""
        pass
    
    @abstractmethod
    def step(self, action: Any) -> Tuple[Any, Dict[str, np.ndarray], bool, bool, Dict[str, Any]]:
        """Take action and return next state, reward, termination flags, and info."""
        pass
    
    @abstractmethod
    def render(self, mode: str = "human") -> Optional[np.ndarray]:
        """Render the environment."""
        pass
    
    @abstractmethod
    def get_observations(self) -> Any:
        """Get current observations."""
        pass
```

**Parameters:**
- `seed`: Random seed for reproducibility
- `options`: Additional reset options
- `action`: Action to take in the environment
- `mode`: Render mode ("human", "rgb_array")

**Returns:**
- `observation`: Current environment observation
- `reward`: Reward dictionary with decomposed rewards
- `terminated`: Whether episode has terminated
- `truncated`: Whether episode was truncated
- `info`: Additional information dictionary

#### RLAgent

```python
class RLAgent(ABC):
    """Abstract base class for RL agents."""
    
    @abstractmethod
    def predict(self, observation: Any) -> np.ndarray:
        """Predict action probabilities given observation."""
        pass
    
    @abstractmethod
    def get_q_net(self) -> Any:
        """Get the Q-network for explainability analysis."""
        pass
    
    def close(self) -> None:
        """Clean up resources."""
        pass
```

**Parameters:**
- `observation`: Current environment observation

**Returns:**
- `action_probs`: Probability distribution over actions
- `q_net`: Q-network for explainability analysis

#### ExplainabilityMethod

```python
class ExplainabilityMethod(Visualizable):
    """Abstract base class for explainability methods."""
    
    @abstractmethod
    def set(self, env: RLEnvironment) -> None:
        """Set the environment for the method."""
        pass
    
    @abstractmethod
    def prepare(self, agent: RLAgent) -> None:
        """Prepare the method with an agent."""
        pass
    
    @abstractmethod
    def explain(self, obs: np.ndarray) -> np.ndarray | Any:
        """Generate explanation for given observation."""
        pass
    
    @abstractmethod
    def value(self, obs: np.ndarray) -> float:
        """Compute explanation quality score."""
        pass
```

**Parameters:**
- `env`: RL environment instance
- `agent`: RL agent instance
- `obs`: Observation array

**Returns:**
- `explanation`: Explanation data (varies by method)
- `value`: Quality score for the explanation

### Environment Implementations

#### GymRLEnv

```python
class GymRLEnv(RLEnvironment):
    def __init__(self, env_name: str, stack_size: int = 4, 
                 frame_skip: int = 1, render_mode: Optional[str] = None,
                 max_episode_steps: Optional[int] = None,
                 reward_clipping: Optional[Tuple[float, float]] = None,
                 reward_scaling: Optional[float] = None,
                 observation_preprocessing: Optional[Callable] = None,
                 action_repeat: Optional[int] = None,
                 seed: Optional[int] = None,
                 record_video: bool = False,
                 video_path: Optional[str] = None,
                 normalize_observations: bool = False,
                 num_envs: int = 1,
                 tabular: bool = False):
```

**Parameters:**
- `env_name`: Gymnasium environment name
- `stack_size`: Number of frames to stack
- `frame_skip`: Number of frames to skip
- `render_mode`: Rendering mode
- `max_episode_steps`: Maximum steps per episode
- `reward_clipping`: Tuple of (min, max) reward values
- `reward_scaling`: Reward scaling factor
- `observation_preprocessing`: Custom preprocessing function
- `action_repeat`: Number of times to repeat actions
- `seed`: Random seed
- `record_video`: Whether to record video
- `video_path`: Path for video recording
- `normalize_observations`: Whether to normalize observations
- `num_envs`: Number of parallel environments
- `tabular`: Whether environment is tabular

### Agent Implementations

#### DQNAgent

```python
class DQNAgent(RLAgent):
    def __init__(self, model_path: str):
        """Initialize DQN agent from saved model."""
```

**Parameters:**
- `model_path`: Path to saved PyTorch model

#### TorchDQN

```python
class TorchDQN(RLAgent):
    def __init__(self, model_path: str, model: nn.Module, device: torch.device):
        """Initialize PyTorch DQN agent."""
```

**Parameters:**
- `model_path`: Path to saved model weights
- `model`: PyTorch model architecture
- `device`: Computation device (CPU/GPU)

### Explainability Methods

#### ShapExplainability

```python
class ShapExplainability(ExplainabilityMethod):
    def __init__(self, device: torch.device, mask: Mask):
        """Initialize SHAP explainability method."""
```

**Parameters:**
- `device`: PyTorch device for computations
- `mask`: Mask object for computing attribution scores

#### LimeExplainability

```python
class LimeExplainability(ExplainabilityMethod):
    def __init__(self, device: torch.device, mask: Mask):
        """Initialize LIME explainability method."""
```

**Parameters:**
- `device`: PyTorch device for computations
- `mask`: Mask object for computing attribution scores

#### StabilityExplainability

```python
class StabilityExplainability(ExplainabilityMethod):
    def __init__(self, device: torch.device, 
                 noise_type: Optional[str] = 'gaussian',
                 noise_strength: Optional[float] = 0.01,
                 num_samples: Optional[int] = 20,
                 reward_weight: Optional[bool] = True,
                 visualize: Optional[bool] = True,
                 save_dir: Optional[str] = "./temp"):
        """Initialize stability explainability method."""
```

**Parameters:**
- `device`: PyTorch device for computations
- `noise_type`: Type of noise ('gaussian' or 'salt_pepper')
- `noise_strength`: Strength of noise
- `num_samples`: Number of noisy samples to test
- `reward_weight`: Whether to weight by reward
- `visualize`: Whether to save visualizations
- `save_dir`: Directory for saving visualizations

#### LMUTExplainability

```python
class LinearModelUTreeExplainability(ExplainabilityMethod):
    def __init__(self, device: torch.device, mask: Mask,
                 max_depth: int = 5, min_samples_leaf: int = 5,
                 num_training_samples: int = 1000):
        """Initialize LMUT explainability method."""
```

**Parameters:**
- `device`: PyTorch device for computations
- `mask`: Mask object for computing attribution scores
- `max_depth`: Maximum depth of decision trees
- `min_samples_leaf`: Minimum samples per leaf node
- `num_training_samples`: Number of training samples to collect

### Masking Implementations

#### AssaultEnvShapMask

```python
class AssaultEnvShapMask(Mask):
    def __init__(self):
        """Initialize Assault environment SHAP mask."""
```

#### LunarLanderTabularMask

```python
class LunarLanderTabularMask(Mask):
    def __init__(self):
        """Initialize Lunar Lander tabular mask."""
```

### Visualization Methods

```python
class VisualizationMethod(Enum):
    FEATURES = 0    # Feature map <string, float>
    RGB_ARRAY = 1   # Float 2D RGB image [0, 1.0]
    GRAY_SCAL = 2   # Float 2D Gray image [0, 1.0]
    HEAT_MAP = 3    # Float 2D image [-inf, inf]
    BAR_CHART = 4   # Bar chart <string, float>
```

## Usage Examples

### Basic Explainability Analysis

```python
import torch
from pearl.enviroments.GymRLEnv import GymRLEnv
from pearl.agents.SimpleDQN import DQNAgent
from pearl.methods.ShapExplainability import ShapExplainability
from pearl.provided.AssaultEnv import AssaultEnvShapMask

# Setup environment and agent
env = GymRLEnv("ALE/Assault-v5", stack_size=4, frame_skip=4)
agent = DQNAgent("models/dqn_assault_5m.pth")

# Initialize explainability method
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
mask = AssaultEnvShapMask()
explainer = ShapExplainability(device, mask)

# Configure explainer
explainer.set(env)
explainer.prepare(agent)

# Run analysis
obs = env.get_observations()
explanation = explainer.explain(obs)
quality_score = explainer.value(obs)

print(f"Explanation quality score: {quality_score:.4f}")
```

### Stability Analysis

```python
from pearl.methods.StabilityExplainability import StabilityExplainability

# Initialize stability analyzer
stability_explainer = StabilityExplainability(
    device=device,
    noise_type='gaussian',
    noise_strength=0.01,
    num_samples=20,
    visualize=True
)

# Configure and run
stability_explainer.set(env)
stability_explainer.prepare(agent)

stability_score = stability_explainer.value(obs)
print(f"Agent stability score: {stability_score:.4f}")
```

### Custom Environment Wrapper

```python
from pearl.enviroments.ObservationWrapper import ObservationWrapper
import cv2
import numpy as np

class CustomPreprocessingWrapper(ObservationWrapper):
    def __init__(self, env):
        super().__init__(env)
        
    def observation(self, obs):
        # Custom preprocessing logic
        gray = cv2.cvtColor(obs, cv2.COLOR_RGB2GRAY)
        resized = cv2.resize(gray, (84, 84))
        return np.expand_dims(resized, axis=-1)
    
    def get_observations(self):
        return self.observation(self.env.get_observations())
    
    def reset(self, **kwargs):
        obs, info = self.env.reset(**kwargs)
        return self.observation(obs), info
    
    def step(self, action):
        obs, reward_dict, terminated, truncated, info = self.env.step(action)
        return self.observation(obs), reward_dict, terminated, truncated, info

# Usage
base_env = GymRLEnv("ALE/Assault-v5")
custom_env = CustomPreprocessingWrapper(base_env)
```

### Training Custom Models

```python
# Example: Training DQN for Assault
from stable_baselines3 import DQN
from stable_baselines3.common.env_util import make_atari_env
from stable_baselines3.common.vec_env import VecFrameStack, VecTransposeImage

# Create environment
env = make_atari_env("ALE/Assault-v5", n_envs=4, seed=42)
env = VecFrameStack(env, n_stack=4)
env = VecTransposeImage(env)

# Train model
model = DQN("CnnPolicy", env, verbose=1)
model.learn(total_timesteps=1_000_000)
model.save("models/dqn_assault_custom")
```

### Visualization

```python
# Get visualization for SHAP explanations
visualization = explainer.getVisualization(
    VisualizationMethod.RGB_ARRAY,
    params=ShapVisualizationParams(action=0)
)

# Get bar chart for tabular explanations
tabular_viz = tabular_explainer.getVisualization(
    VisualizationMethod.BAR_CHART,
    params=TabularLimeVisualizationParams(action=0)
)
```

## Project Structure

```
PearlLab/
├── CMakeLists.txt              # Main build configuration
├── config.yaml                 # Application configuration
├── src/                        # C++ source code
│   ├── application.cpp         # Main application entry point
│   ├── backend/                # Python-C++ bindings
│   └── ui/                     # ImGui user interface
├── py/                         # Python framework
│   ├── pearl/                  # Main Python package
│   │   ├── agent.py            # Agent abstractions
│   │   ├── env.py              # Environment abstractions
│   │   ├── method.py           # Explainability method abstractions
│   │   ├── mask.py             # Action masking
│   │   ├── visualizable.py     # Visualization support
│   │   ├── agents/             # Agent implementations
│   │   ├── enviroments/        # Environment implementations
│   │   ├── methods/            # Explainability implementations
│   │   ├── provided/           # Provided environments and masks
│   │   └── custom_methods/     # Custom explainability implementations
│   ├── models/                 # Training and evaluation scripts
│   └── pearl_loader.py         # Module loader
├── scripts/                    # Utility scripts
└── projects/                   # Project configurations
```

## Contributing

### Development Setup

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/your-feature`
3. **Install development dependencies**:
   ```bash
   pip install -r requirements-dev.txt
   ```
4. **Run tests**:
   ```bash
   python -m pytest tests/
   ```
5. **Submit a pull request**

### Adding New Components

#### New Environment

1. Inherit from `RLEnvironment` or `ObservationWrapper`
2. Implement required abstract methods
3. Add to `pearl/enviroments/` directory
4. Update documentation

#### New Agent

1. Inherit from `RLAgent`
2. Implement `predict()` and `get_q_net()` methods
3. Add to `pearl/agents/` directory
4. Update documentation

#### New Explainability Method

1. Inherit from `ExplainabilityMethod`
2. Implement required abstract methods
3. Add visualization support if applicable
4. Add to `pearl/methods/` directory
5. Update documentation

#### New Mask

1. Inherit from `Mask`
2. Implement `update()` and `compute()` methods
3. Add to `pearl/provided/` or create new directory
4. Update documentation

### Code Style

- **Python**: Follow PEP 8 with 4-space indentation
- **C++**: Follow Google C++ Style Guide
- **Documentation**: Use docstrings for all public methods
- **Type Hints**: Use type hints for all function parameters and return values

### Testing

```bash
# Run all tests
python -m pytest

# Run specific test file
python -m pytest tests/test_agent.py

# Run with coverage
python -m pytest --cov=pearl tests/
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- **Gymnasium**: RL environment framework
- **Stable Baselines3**: RL algorithm implementations
- **SHAP**: SHAP explainability library
- **LIME**: LIME explainability library
- **ImGui**: Immediate mode GUI framework
- **PyTorch**: Deep learning framework

## Support

For questions, issues, or contributions:
- **Issues**: Use GitHub issues
- **Discussions**: Use GitHub discussions
- **Documentation**: Check the docs/ directory for detailed guides

## Version History

- **v1.0-beta**: Initial release with core explainability methods
  - SHAP, LIME, Stability, and LMUT explainability
  - Atari and Lunar Lander environment support
  - ImGui-based visualization interface
  - Modular architecture for extensibility 