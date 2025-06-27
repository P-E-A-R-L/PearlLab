# PearlLab Quick Reference Guide

## Table of Contents

1. [Core Concepts](#core-concepts)
2. [Quick Start](#quick-start)
3. [Common Patterns](#common-patterns)
4. [API Cheat Sheet](#api-cheat-sheet)
5. [Configuration](#configuration)
6. [Troubleshooting](#troubleshooting)

## Core Concepts

### Architecture Overview

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Environment   │───▶│     Agent       │───▶│  Explainability │
│                 │    │                 │    │     Method      │
│ • Observations  │    │ • Predictions   │    │ • Explanations  │
│ • Actions       │    │ • Q-Networks    │    │ • Quality Scores│
│ • Rewards       │    │ • Models        │    │ • Visualizations│
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│      Mask       │    │   Visualization │    │   GUI Display   │
│                 │    │                 │    │                 │
│ • Action Scores │    │ • Heatmaps      │    │ • Real-time     │
│ • Domain Logic  │    │ • Bar Charts    │    │ • Interactive   │
│ • Relevance     │    │ • RGB Overlays  │    │ • Project Mgmt  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Key Components

| Component | Purpose | Main Classes |
|-----------|---------|--------------|
| **Environment** | RL environment interface | `RLEnvironment`, `GymRLEnv` |
| **Agent** | RL agent with models | `RLAgent`, `DQNAgent`, `TorchDQN` |
| **Explainability** | Explanation methods | `ExplainabilityMethod`, `ShapExplainability`, `LimeExplainability` |
| **Mask** | Action relevance scoring | `Mask`, `AssaultEnvShapMask`, `LunarLanderTabularMask` |
| **Visualization** | Display methods | `Visualizable`, `VisualizationMethod` |

## Quick Start

### Minimal Example

```python
import torch
from pearl.enviroments.GymRLEnv import GymRLEnv
from pearl.agents.SimpleDQN import DQNAgent
from pearl.methods.ShapExplainability import ShapExplainability
from pearl.provided.AssaultEnv import AssaultEnvShapMask

# Setup
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
env = GymRLEnv("ALE/Assault-v5", stack_size=4, frame_skip=4)
agent = DQNAgent("models/dqn_assault_5m.pth")
mask = AssaultEnvShapMask()
explainer = ShapExplainability(device, mask)

# Configure
explainer.set(env)
explainer.prepare(agent)

# Analyze
obs = env.get_observations()
explanation = explainer.explain(obs)
score = explainer.value(obs)
print(f"Quality score: {score:.4f}")
```

### GUI Application

```bash
# Run the GUI application
./PearlLab

# Or with specific project
./PearlLab --project projects/my_project
```

## Common Patterns

### Environment Setup

```python
# Basic environment
env = GymRLEnv("ALE/Assault-v5", stack_size=4, frame_skip=4)

# With custom preprocessing
env = GymRLEnv(
    env_name="ALE/Assault-v5",
    stack_size=4,
    frame_skip=4,
    normalize_observations=True,
    reward_clipping=(-1, 1)
)

# Tabular environment
tabular_env = GymRLEnv("LunarLander-v2", tabular=True)
```

### Agent Loading

```python
# Stable Baselines3 agent
agent = DQNAgent("models/dqn_assault_5m.pth")

# Custom PyTorch agent
from pearl.agents.TourchDQN import TorchDQN
import torch.nn as nn

class CustomModel(nn.Module):
    def __init__(self, n_actions):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(8, 64), nn.ReLU(),
            nn.Linear(64, n_actions)
        )
    
    def forward(self, x):
        return self.net(x)

model = CustomModel(4)
custom_agent = TorchDQN("models/custom.pth", model, device)
```

### Explainability Methods

```python
# SHAP
shap_explainer = ShapExplainability(device, mask)

# LIME
lime_explainer = LimeExplainability(device, mask)

# Stability
stability_explainer = StabilityExplainability(
    device=device,
    noise_type='gaussian',
    noise_strength=0.01,
    num_samples=20
)

# LMUT
lmut_explainer = LinearModelUTreeExplainability(
    device=device,
    mask=mask,
    max_depth=5,
    num_training_samples=1000
)
```

### Visualization

```python
from visual import VisualizationMethod

# Get RGB visualization
rgb_viz = explainer.getVisualization(
    VisualizationMethod.RGB_ARRAY,
    params=ShapVisualizationParams(action=0)
)

# Get bar chart
bar_viz = explainer.getVisualization(
    VisualizationMethod.BAR_CHART,
    params=TabularLimeVisualizationParams(action=0)
)
```

## API Cheat Sheet

### Environment Interface

```python
class RLEnvironment:
    def reset(self, seed=None, options=None) -> Tuple[Any, Dict[str, Any]]:
        """Reset environment to initial state."""
    
    def step(self, action: Any) -> Tuple[Any, Dict[str, np.ndarray], bool, bool, Dict[str, Any]]:
        """Take action and return (obs, reward, terminated, truncated, info)."""
    
    def render(self, mode: str = "human") -> Optional[np.ndarray]:
        """Render the environment."""
    
    def get_observations(self) -> Any:
        """Get current observations."""
```

### Agent Interface

```python
class RLAgent:
    def predict(self, observation: Any) -> np.ndarray:
        """Predict action probabilities."""
    
    def get_q_net(self) -> Any:
        """Get Q-network for explainability."""
    
    def close(self) -> None:
        """Clean up resources."""
```

### Explainability Interface

```python
class ExplainabilityMethod:
    def set(self, env: RLEnvironment) -> None:
        """Set environment."""
    
    def prepare(self, agent: RLAgent) -> None:
        """Prepare with agent."""
    
    def onStep(self, action: Any) -> None:
        """Called before step."""
    
    def onStepAfter(self, action: Any, reward: Dict, done: bool, info: dict) -> None:
        """Called after step."""
    
    def explain(self, obs: np.ndarray) -> np.ndarray | Any:
        """Generate explanation."""
    
    def value(self, obs: np.ndarray) -> float:
        """Compute quality score."""
```

### Mask Interface

```python
class Mask:
    def update(self, observation: Any) -> None:
        """Update mask based on observation."""
    
    def compute(self, values: Any) -> np.ndarray:
        """Compute mask scores for values."""
```

### Visualization Interface

```python
class Visualizable:
    def supports(self, m: VisualizationMethod) -> bool:
        """Check if method is supported."""
    
    def getVisualizationParamsType(self, m: VisualizationMethod) -> type | None:
        """Get parameter type for method."""
    
    def getVisualization(self, m: VisualizationMethod, params: Any = None) -> np.ndarray | dict | None:
        """Get visualization."""
```

## Configuration

### YAML Configuration

```yaml
# config.yaml
version: 1.0-beta
window:
  width: 1280
  height: 720
venv: /path/to/pearl_env
```

### Project Configuration

```json
// projects/my_project/config.json
{
  "name": "my_project",
  "environments": {
    "assault": {
      "type": "ALE/Assault-v5",
      "stack_size": 4,
      "frame_skip": 4
    }
  },
  "agents": {
    "dqn": {
      "type": "DQNAgent",
      "model_path": "models/dqn_assault_5m.pth"
    }
  },
  "methods": {
    "shap": {
      "type": "ShapExplainability",
      "device": "auto"
    }
  }
}
```

### Environment Variables

```bash
# Python path
export PYTHONPATH="${PYTHONPATH}:/path/to/pearllab/py"

# Configuration
export PEARLLAB_CONFIG="/path/to/config.yaml"
export PEARLLAB_MODELS="/path/to/models"

# Debug
export PEARLLAB_DEBUG=1
export PEARLLAB_LOG_LEVEL=DEBUG
```

## Troubleshooting

### Common Issues

| Issue | Solution |
|-------|----------|
| **Import Error** | Check `PYTHONPATH` and virtual environment |
| **Model Not Found** | Verify model path and file exists |
| **CUDA Out of Memory** | Use CPU or reduce batch size |
| **GLFW Error** | Check display settings and graphics drivers |
| **CMake Error** | Install missing dependencies |

### Debug Commands

```bash
# Check Python path
echo $PYTHONPATH

# Test imports
python -c "import pearl; print('Import successful')"

# Check CUDA
python -c "import torch; print(torch.cuda.is_available())"

# Check system info
uname -a
python --version
gcc --version
```

### Performance Tips

```python
# Use GPU if available
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# Batch processing
def batch_explain(explainer, observations, batch_size=32):
    for i in range(0, len(observations), batch_size):
        batch = observations[i:i+batch_size]
        # Process batch

# Memory optimization
import gc
gc.collect()  # Force garbage collection
torch.cuda.empty_cache()  # Clear GPU cache
```

### Error Handling

```python
# Graceful error handling
try:
    explanation = explainer.explain(obs)
except Exception as e:
    print(f"Explanation failed: {e}")
    explanation = None

# Fallback values
score = explainer.value(obs) if explanation is not None else 0.0
```

## File Structure

```
PearlLab/
├── CMakeLists.txt              # Build configuration
├── config.yaml                 # Application config
├── src/                        # C++ source code
│   ├── application.cpp         # Main entry point
│   ├── backend/                # Python bindings
│   └── ui/                     # GUI components
├── py/                         # Python framework
│   ├── pearl/                  # Main package
│   │   ├── agent.py            # Agent abstractions
│   │   ├── env.py              # Environment abstractions
│   │   ├── method.py           # Explainability methods
│   │   ├── mask.py             # Action masking
│   │   ├── visualizable.py     # Visualization
│   │   ├── agents/             # Agent implementations
│   │   ├── enviroments/        # Environment implementations
│   │   ├── methods/            # Explainability implementations
│   │   └── provided/           # Provided components
│   ├── models/                 # Training scripts
│   └── pearl_loader.py         # Module loader
├── projects/                   # Project configurations
├── models/                     # Trained models
└── docs/                       # Documentation
```

## Command Line Interface

```bash
# Run GUI application
./PearlLab

# Run with specific project
./PearlLab --project projects/my_project

# Run with debug output
./PearlLab --debug

# Run with custom config
./PearlLab --config /path/to/config.yaml

# Help
./PearlLab --help
```

## Development Workflow

### 1. Setup Environment

```bash
# Create virtual environment
python3 -m venv pearl_env
source pearl_env/bin/activate

# Install dependencies
cd py/pearl
pip install -r requirements.txt
```

### 2. Build C++ Components

```bash
# Configure build
mkdir build && cd build
cmake ..

# Build
make -j$(nproc)
```

### 3. Test Installation

```python
# Test imports
import torch
import numpy as np
from pearl.enviroments.GymRLEnv import GymRLEnv
from pearl.agents.SimpleDQN import DQNAgent
print("✓ Installation successful")
```

### 4. Run Analysis

```python
# Basic analysis
env = GymRLEnv("ALE/Assault-v5")
agent = DQNAgent("models/dqn_assault_5m.pth")
explainer = ShapExplainability(device, mask)

explainer.set(env)
explainer.prepare(agent)

obs = env.get_observations()
explanation = explainer.explain(obs)
score = explainer.value(obs)
```

This quick reference guide provides essential information for getting started with PearlLab. For detailed documentation, see the main README.md and other documentation files. 