# PearlLab API Documentation

## Table of Contents

1. [Core Abstractions](#core-abstractions)
2. [Environment Implementations](#environment-implementations)
3. [Agent Implementations](#agent-implementations)
4. [Explainability Methods](#explainability-methods)
5. [Masking Implementations](#masking-implementations)
6. [Visualization](#visualization)
7. [Utility Classes](#utility-classes)

## Core Abstractions

### RLEnvironment

**File**: `pearl/env.py`

Abstract base class for all reinforcement learning environments in the PearlLab framework.

```python
class RLEnvironment(Visualizable):
    """Abstract base class for Reinforcement Learning environments."""
```

#### Abstract Methods

##### `reset(seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None) -> Tuple[Any, Dict[str, Any]]`

Reset the environment to an initial state.

**Parameters:**
- `seed` (Optional[int]): Random seed for reproducibility
- `options` (Optional[Dict[str, Any]]): Additional options for resetting

**Returns:**
- `Tuple[Any, Dict[str, Any]]`: Initial observation and info dictionary

##### `step(action: Any) -> Tuple[Any, Dict[str, np.ndarray], bool, bool, Dict[str, Any]]`

Take a step in the environment with the given action.

**Parameters:**
- `action` (Any): The action to take

**Returns:**
- `Tuple[Any, Dict[str, np.ndarray], bool, bool, Dict[str, Any]]`: 
  - Observation after taking the action
  - Reward dictionary with decomposed rewards
  - Whether the episode has terminated
  - Whether the episode was truncated
  - Additional information dictionary

##### `render(mode: str = "human") -> Optional[np.ndarray]`

Render the environment.

**Parameters:**
- `mode` (str): Render mode ("human", "rgb_array", etc.)

**Returns:**
- `Optional[np.ndarray]`: Rendered image or None

##### `get_observations() -> Any`

Get the current observations from the environment.

**Returns:**
- `Any`: Current observations (typically numpy array)

#### Concrete Methods

##### `close() -> None`

Clean up resources used by the environment.

##### `seed(seed: Optional[int] = None) -> List[int]`

Set the seed for the environment's random number generator.

**Parameters:**
- `seed` (Optional[int]): The seed value

**Returns:**
- `List[int]`: List containing the seed

##### `get_available_actions() -> List[Any]`

Get the list of available actions in the current state.

**Returns:**
- `List[Any]`: List of valid actions

### RLAgent

**File**: `pearl/agent.py`

Abstract base class for all reinforcement learning agents.

```python
class RLAgent(ABC):
    """Simple abstract base class for Reinforcement Learning agents."""
```

#### Abstract Methods

##### `__init__(observation_space=None, action_space=None)`

Initialize the agent.

**Parameters:**
- `observation_space`: The observation space of the environment
- `action_space`: The action space of the environment

##### `predict(observation: Any) -> np.ndarray`

Returns the probability for each action given an observation.

**Parameters:**
- `observation` (Any): The current observation from the environment

**Returns:**
- `np.ndarray`: Probability distribution over actions

##### `get_q_net() -> Any`

Returns the Q-network of the agent.

**Returns:**
- `Any`: The Q-network of the agent

#### Concrete Methods

##### `close() -> None`

Clean up resources used by the agent.

### ExplainabilityMethod

**File**: `pearl/method.py`

Abstract base class for all explainability methods.

```python
class ExplainabilityMethod(Visualizable):
    """Abstract base class for explainability methods."""
```

#### Abstract Methods

##### `set(env: RLEnvironment) -> None`

Called once when the method is attached to an environment and agent.

**Parameters:**
- `env` (RLEnvironment): The environment to attach to

##### `prepare(agents: RLAgent) -> None`

Called once when the method is attached to an agent.

**Parameters:**
- `agents` (RLAgent): The agent to attach to

##### `onStep(action: Any) -> None`

Called once when a step is taken into the environment (before it's taken).

**Parameters:**
- `action` (Any): The action being taken

##### `onStepAfter(action: Any, reward: Dict[str, np.ndarray], done: bool, info: dict) -> None`

Called once when a step is taken into the environment (after it's taken).

**Parameters:**
- `action` (Any): The action that was taken
- `reward` (Dict[str, np.ndarray]): The reward received
- `done` (bool): Whether the episode is done
- `info` (dict): Additional information

##### `explain(obs) -> np.ndarray | Any`

Using the current state of the environment and agent, return the explainability for this method.

**Parameters:**
- `obs`: Current observation

**Returns:**
- `np.ndarray | Any`: Explanation data

##### `value(obs) -> float`

Should return one value for the agent that indicates how well the agent performed according to this method.

**Parameters:**
- `obs`: Current observation

**Returns:**
- `float`: Performance score

### Mask

**File**: `pearl/mask.py`

Abstract base class for masking actions in reinforcement learning.

```python
class Mask(ABC):
    """Simple abstract base class for masking actions in reinforcement learning."""
```

#### Abstract Methods

##### `update(observation: Any) -> None`

Update the mask based on the current observation.

**Parameters:**
- `observation` (Any): The current observation from the environment (N, H, W, C)

##### `compute(values: Any) -> np.ndarray`

Returns the value of the mask for the given values.

**Parameters:**
- `values` (Any): The current values to multiply by the mask (N, H, W, C, Action)

**Returns:**
- `np.ndarray`: Tensor of shape [actions]

### Visualizable

**File**: `pearl/visualizable.py`

Abstract base class for visualizable methods.

```python
class Visualizable(ABC):
    """Abstract base class for visualizable methods."""
```

#### Abstract Methods

##### `supports(m: VisualizationMethod) -> bool`

Should return if this instance supports this type of visualization.

**Parameters:**
- `m` (VisualizationMethod): The visualization method to query

**Returns:**
- `bool`: Whether this method is supported

##### `getVisualizationParamsType(m: VisualizationMethod) -> type | None`

If the visualization method requires some parameters, then this function should return the type of the dataclass of the parameters the method expects.

**Parameters:**
- `m` (VisualizationMethod): The visualization method to query

**Returns:**
- `type | None`: None if the method is not supported/requires no params, else the type of the dataclass

##### `getVisualization(m: VisualizationMethod, params: Any = None) -> np.ndarray | dict | None`

Should return the visualization as np.array (Heatmap, RGB, Gray) or dict (Features <str, float>).

**Parameters:**
- `m` (VisualizationMethod): The visualization method to query
- `params` (Any): If this method requires parameters, then this should be the dataclass of the parameters

**Returns:**
- `np.ndarray | dict | None`: Visualization as np.array (Heatmap, RGB, Gray) or dict (Features <str, float>)

## Environment Implementations

### GymRLEnv

**File**: `pearl/enviroments/GymRLEnv.py`

Gymnasium-based environment wrapper that provides a unified interface for Gymnasium environments.

```python
class GymRLEnv(RLEnvironment):
    """Gymnasium-based RL environment wrapper."""
```

#### Constructor

```python
def __init__(self, env_name: str, stack_size: int = 4, frame_skip: int = 1,
             render_mode: Optional[str] = None, max_episode_steps: Optional[int] = None,
             reward_clipping: Optional[Tuple[float, float]] = None,
             reward_scaling: Optional[float] = None,
             observation_preprocessing: Optional[Callable[[np.ndarray], np.ndarray]] = None,
             action_repeat: Optional[int] = None, seed: Optional[int] = None,
             record_video: bool = False, video_path: Optional[str] = None,
             normalize_observations: bool = False, num_envs: int = 1, tabular: bool = False):
```

**Parameters:**
- `env_name` (str): Gymnasium environment name
- `stack_size` (int): Number of frames to stack (default: 4)
- `frame_skip` (int): Number of frames to skip (default: 1)
- `render_mode` (Optional[str]): Rendering mode (default: None)
- `max_episode_steps` (Optional[int]): Maximum steps per episode (default: None)
- `reward_clipping` (Optional[Tuple[float, float]]): Tuple of (min, max) reward values (default: None)
- `reward_scaling` (Optional[float]): Reward scaling factor (default: None)
- `observation_preprocessing` (Optional[Callable]): Custom preprocessing function (default: None)
- `action_repeat` (Optional[int]): Number of times to repeat actions (default: None)
- `seed` (Optional[int]): Random seed (default: None)
- `record_video` (bool): Whether to record video (default: False)
- `video_path` (Optional[str]): Path for video recording (default: None)
- `normalize_observations` (bool): Whether to normalize observations (default: False)
- `num_envs` (int): Number of parallel environments (default: 1)
- `tabular` (bool): Whether environment is tabular (default: False)

#### Methods

##### `reset(seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None) -> Tuple[np.ndarray, Dict[str, Any]]`

Reset the environment to initial state.

**Parameters:**
- `seed` (Optional[int]): Random seed for reproducibility
- `options` (Optional[Dict[str, Any]]): Additional reset options

**Returns:**
- `Tuple[np.ndarray, Dict[str, Any]]`: Initial observation and info

##### `step(action: Union[int, np.ndarray]) -> Tuple[np.ndarray, Dict[str, np.ndarray], bool, bool, Dict[str, Any]]`

Take action and return next state, reward, termination flags, and info.

**Parameters:**
- `action` (Union[int, np.ndarray]): Action to take

**Returns:**
- `Tuple[np.ndarray, Dict[str, np.ndarray], bool, bool, Dict[str, Any]]`: 
  - Next observation
  - Reward dictionary
  - Termination flag
  - Truncation flag
  - Info dictionary

##### `render(mode: str = "human") -> Optional[np.ndarray]`

Render the environment.

**Parameters:**
- `mode` (str): Render mode

**Returns:**
- `Optional[np.ndarray]`: Rendered image or None

##### `get_observations() -> np.ndarray`

Get current observations.

**Returns:**
- `np.ndarray`: Current observations

### ObservationWrapper

**File**: `pearl/enviroments/ObservationWrapper.py`

Abstract wrapper that forwards all calls to an underlying environment, except reset() and get_observations(), which subclasses must implement.

```python
class ObservationWrapper(RLEnvironment):
    """Abstract wrapper for observation preprocessing."""
```

#### Constructor

```python
def __init__(self, env):
```

**Parameters:**
- `env`: The underlying environment to wrap

#### Abstract Methods

##### `reset(seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None)`

Reset the wrapped environment with custom observation processing.

##### `step(action: Any)`

Take a step in the wrapped environment with custom observation processing.

##### `get_observations()`

Get observations from the wrapped environment with custom processing.

## Agent Implementations

### DQNAgent

**File**: `pearl/agents/SimpleDQN.py`

Simple DQN agent implementation using Stable Baselines3.

```python
class DQNAgent(RLAgent):
    """Simple DQN agent."""
```

#### Constructor

```python
def __init__(self, model_path: str):
```

**Parameters:**
- `model_path` (str): Path to saved Stable Baselines3 model

#### Methods

##### `predict(observation) -> np.ndarray`

Predict action probabilities given observation.

**Parameters:**
- `observation`: Current observation

**Returns:**
- `np.ndarray`: Probability distribution over actions

##### `get_q_net()`

Get the Q-network for explainability analysis.

**Returns:**
- Q-network model

##### `close()`

Clean up resources.

### TorchDQN

**File**: `pearl/agents/TourchDQN.py`

PyTorch-based DQN agent implementation.

```python
class TorchDQN(RLAgent):
    """PyTorch-based DQN agent."""
```

#### Constructor

```python
def __init__(self, model_path: str, model: nn.Module, device: torch.device):
```

**Parameters:**
- `model_path` (str): Path to saved model weights
- `model` (nn.Module): PyTorch model architecture
- `device` (torch.device): Computation device (CPU/GPU)

#### Methods

##### `predict(observation) -> np.ndarray`

Predict action probabilities given observation.

**Parameters:**
- `observation`: Current observation

**Returns:**
- `np.ndarray`: Probability distribution over actions

##### `get_q_net()`

Get the Q-network for explainability analysis.

**Returns:**
- Q-network model

## Explainability Methods

### ShapExplainability

**File**: `pearl/methods/ShapExplainability.py`

SHAP-based explainability method for deep learning models.

```python
class ShapExplainability(ExplainabilityMethod):
    """SHAP explainability method."""
```

#### Constructor

```python
def __init__(self, device: torch.device, mask: Mask):
```

**Parameters:**
- `device` (torch.device): PyTorch device for computations
- `mask` (Mask): Mask object for computing attribution scores

#### Methods

##### `set(env: RLEnvironment) -> None`

Set the environment and initialize background.

**Parameters:**
- `env` (RLEnvironment): The environment to attach to

##### `prepare(agent: RLAgent) -> None`

Prepare the SHAP explainer with the agent's model.

**Parameters:**
- `agent` (RLAgent): The agent to attach to

##### `explain(obs) -> np.ndarray | Any`

Generate SHAP explanations for the given observation.

**Parameters:**
- `obs`: Current observation

**Returns:**
- `np.ndarray | Any`: SHAP values

##### `value(obs) -> float`

Compute explanation quality score using the mask.

**Parameters:**
- `obs`: Current observation

**Returns:**
- `float`: Quality score

### LimeExplainability

**File**: `pearl/methods/LimeExplainability.py`

LIME-based explainability method for image-based models.

```python
class LimeExplainability(ExplainabilityMethod):
    """LIME explainability method."""
```

#### Constructor

```python
def __init__(self, device: torch.device, mask: Mask):
```

**Parameters:**
- `device` (torch.device): PyTorch device for computations
- `mask` (Mask): Mask object for computing attribution scores

#### Methods

##### `explain(obs: np.ndarray) -> Any`

Generate LIME explanations for the given observation.

**Parameters:**
- `obs` (np.ndarray): Current observation

**Returns:**
- `Any`: LIME explanation object

##### `value(obs: np.ndarray) -> float`

Compute explanation quality score using the mask.

**Parameters:**
- `obs` (np.ndarray): Current observation

**Returns:**
- `float`: Quality score

### StabilityExplainability

**File**: `pearl/methods/StabilityExplainability.py`

Stability-based explainability method that measures agent consistency under input noise.

```python
class StabilityExplainability(ExplainabilityMethod):
    """Stability explainability method."""
```

#### Constructor

```python
def __init__(self, device: torch.device, noise_type: Optional[str] = 'gaussian',
             noise_strength: Optional[float] = 0.01, num_samples: Optional[int] = 20,
             reward_weight: Optional[bool] = True, visualize: Optional[bool] = True,
             save_dir: Optional[str] = "./temp"):
```

**Parameters:**
- `device` (torch.device): PyTorch device for computations
- `noise_type` (Optional[str]): Type of noise ('gaussian' or 'salt_pepper')
- `noise_strength` (Optional[float]): Strength of noise
- `num_samples` (Optional[int]): Number of noisy samples to test
- `reward_weight` (Optional[bool]): Whether to weight by reward
- `visualize` (Optional[bool]): Whether to save visualizations
- `save_dir` (Optional[str]): Directory for saving visualizations

#### Methods

##### `explain(obs: np.ndarray) -> np.ndarray`

Generate stability explanations by measuring consistency under noise.

**Parameters:**
- `obs` (np.ndarray): Current observation

**Returns:**
- `np.ndarray`: Stability scores

##### `value(obs) -> float`

Compute stability score for the current observation.

**Parameters:**
- `obs`: Current observation

**Returns:**
- `float`: Stability score

### LMUTExplainability

**File**: `pearl/methods/LMUTExplainability.py`

Linear Model U-Tree explainability method that combines decision trees with linear models.

```python
class LinearModelUTreeExplainability(ExplainabilityMethod):
    """Linear Model U-Tree explainability method."""
```

#### Constructor

```python
def __init__(self, device: torch.device, mask: Mask, max_depth: int = 5,
             min_samples_leaf: int = 5, num_training_samples: int = 1000):
```

**Parameters:**
- `device` (torch.device): PyTorch device for computations
- `mask` (Mask): Mask object for computing attribution scores
- `max_depth` (int): Maximum depth of decision trees
- `min_samples_leaf` (int): Minimum samples per leaf node
- `num_training_samples` (int): Number of training samples to collect

#### Methods

##### `collect_training_data(num_samples: Optional[int] = None)`

Collect training data by running the environment and gathering Q-values.

**Parameters:**
- `num_samples` (Optional[int]): Number of samples to collect

##### `fit_models()`

Fit the decision trees and linear models with collected data.

##### `explain(obs: np.ndarray) -> List[np.ndarray]`

Generate explanations using the fitted LMUT models.

**Parameters:**
- `obs` (np.ndarray): Current observation

**Returns:**
- `List[np.ndarray]`: Explanations for each agent

### TabularLimeExplainability

**File**: `pearl/methods/TabularLimeExplainability.py`

LIME-based explainability method for tabular data.

```python
class TabularLimeExplainability(ExplainabilityMethod):
    """Tabular LIME explainability method."""
```

#### Constructor

```python
def __init__(self, device: torch.device, mask: Mask, feature_names: List[str]):
```

**Parameters:**
- `device` (torch.device): PyTorch device for computations
- `mask` (Mask): Mask object for computing attribution scores
- `feature_names` (List[str]): Names of features in the state vector

#### Methods

##### `explain(obs: np.ndarray) -> Any`

Generate LIME explanations for tabular data.

**Parameters:**
- `obs` (np.ndarray): Current observation

**Returns:**
- `Any`: LIME explanation object

### TabularShapExplainability

**File**: `pearl/methods/TabularShapExplainability.py`

SHAP-based explainability method for tabular data.

```python
class TabularShapExplainability(ExplainabilityMethod):
    """Tabular SHAP explainability method."""
```

#### Constructor

```python
def __init__(self, device: torch.device, mask: Mask, feature_names: List[str]):
```

**Parameters:**
- `device` (torch.device): PyTorch device for computations
- `mask` (Mask): Mask object for computing attribution scores
- `feature_names` (List[str]): Names of features in the state vector

#### Methods

##### `explain(obs: np.ndarray) -> Any`

Generate SHAP explanations for tabular data.

**Parameters:**
- `obs` (np.ndarray): Current observation

**Returns:**
- `Any`: SHAP explanation object

## Masking Implementations

### AssaultEnvShapMask

**File**: `pearl/provided/AssaultEnv.py`

Domain-specific mask for Atari Assault environment that focuses on foreground objects.

```python
class AssaultEnvShapMask(Mask):
    """Simplified SHAP mask for the ALE/Assault environment."""
```

#### Constructor

```python
def __init__(self):
```

#### Methods

##### `update(frame: np.ndarray)`

Update the mask based on the current frame.

**Parameters:**
- `frame` (np.ndarray): Current frame of shape (C, H, W) or (1, C, H, W)

##### `compute(values: Any) -> np.ndarray`

Compute mask scores for the given values.

**Parameters:**
- `values` (Any): Values of shape (1, T, H, W, A)

**Returns:**
- `np.ndarray`: Score per action

### LunarLanderTabularMask

**File**: `pearl/provided/LunarLander.py`

Dynamic heuristic mask for Lunar Lander with 8D state.

```python
class LunarLanderTabularMask(Mask):
    """Dynamic heuristic mask for LunarLander with 8D state."""
```

#### Constructor

```python
def __init__(self):
```

#### Methods

##### `update(obs: np.ndarray)`

Update the mask based on the current observation.

**Parameters:**
- `obs` (np.ndarray): Current observation of shape (1, 8)

##### `compute(attr: np.ndarray) -> np.ndarray`

Compute mask scores for the given attributions.

**Parameters:**
- `attr` (np.ndarray): Attributions of shape (1, features, 1, 1, actions)

**Returns:**
- `np.ndarray`: Score per action

## Visualization

### VisualizationMethod

**File**: `py/visual.py`

Enumeration of supported visualization methods.

```python
class VisualizationMethod(Enum):
    FEATURES = 0    # Feature map <string, float>
    RGB_ARRAY = 1   # Float 2D RGB image [0, 1.0]
    GRAY_SCAL = 2   # Float 2D Gray image [0, 1.0]
    HEAT_MAP = 3    # Float 2D image [-inf, inf]
    BAR_CHART = 4   # Bar chart <string, float>
```

### Visualization Parameters

#### ShapVisualizationParams

```python
class ShapVisualizationParams:
    action: Param(int) = 0
```

#### LimeVisualizationParams

```python
class LimeVisualizationParams:
    action: Param(int) = 0
```

#### StabilityVisualizationParams

```python
class StabilityVisualizationParams:
    action: Param(int) = 0
```

#### LMUTVisualizationParams

```python
class LMUTVisualizationParams:
    action: Param(int) = 0
    agent_idx: Param(int) = 0
```

#### TabularLimeVisualizationParams

```python
class TabularLimeVisualizationParams:
    action: Param(int) = 0
```

#### TabularShapVisualizationParams

```python
class TabularShapVisualizationParams:
    action: Param(int) = 0
```

## Utility Classes

### Param

**File**: `py/annotations.py`

Parameter annotation class for GUI configuration.

```python
class Param:
    def __init__(self, typ, editable: bool = True, range_start=None, range_end=None,
                 isFilePath: bool | None = None, choices=None, default=None,
                 disc: str | None = None):
```

**Parameters:**
- `typ`: Parameter type
- `editable` (bool): Whether parameter is editable
- `range_start`: Start of value range
- `range_end`: End of value range
- `isFilePath` (bool | None): Whether parameter is a file path
- `choices`: Available choices for the parameter
- `default`: Default value
- `disc` (str | None): Parameter description

### NoiseGenerator

**File**: `pearl/methods/StabilityExplainability.py`

Utility class for generating different types of noise.

```python
class NoiseGenerator:
    """Handles different types of noise generation for tensors."""
```

#### Static Methods

##### `add_gaussian_noise(tensor: torch.Tensor, std: float = 0.01) -> torch.Tensor`

Add Gaussian noise to tensor and clamp to [0, 1].

**Parameters:**
- `tensor` (torch.Tensor): Input tensor
- `std` (float): Standard deviation of noise

**Returns:**
- `torch.Tensor`: Noisy tensor

##### `add_salt_pepper_noise(tensor: torch.Tensor, amount: float = 0.01) -> torch.Tensor`

Add salt and pepper noise to tensor.

**Parameters:**
- `tensor` (torch.Tensor): Input tensor
- `amount` (float): Amount of noise to add

**Returns:**
- `torch.Tensor`: Noisy tensor

### Visualizer

**File**: `pearl/methods/StabilityExplainability.py`

Utility class for visualizing observations and noisy observations.

```python
class Visualizer:
    """Handles visualization of observations and noisy observations."""
```

#### Constructor

```python
def __init__(self, save_dir: str = "./temp"):
```

**Parameters:**
- `save_dir` (str): Directory to save visualizations

#### Methods

##### `save_comparison(original: np.ndarray, noisy: np.ndarray, title: str = "Original vs Noisy") -> None`

Save side-by-side comparison of original and noisy observations.

**Parameters:**
- `original` (np.ndarray): Original observation
- `noisy` (np.ndarray): Noisy observation
- `title` (str): Plot title 