# PearlLab Architecture Documentation

## Overview

PearlLab is designed as a hybrid C++/Python framework that combines the performance of C++ for the GUI and visualization components with the flexibility of Python for machine learning and explainability algorithms. The architecture follows a modular design pattern that allows for easy extension and customization.

## System Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    PearlLab Application                     │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │   C++ Backend   │  │  Python Engine  │  │  GUI Layer   │ │
│  │                 │  │                 │  │              │ │
│  │ • OpenGL        │  │ • RL Agents     │  │ • ImGui      │ │
│  │ • ImGui         │  │ • Environments  │  │ • Windows    │ │
│  │ • pybind11      │  │ • Explainability│  │ • Rendering  │ │
│  │ • Project Mgmt  │  │ • Visualization │  │ • Layout     │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Component Interaction Flow

```
1. GUI Event → 2. C++ Handler → 3. Python Call → 4. ML Processing → 5. Results → 6. Visualization
```

## Core Components

### 1. C++ Backend (`src/`)

The C++ backend provides the foundation for the application, handling GUI rendering, project management, and Python integration.

#### Key Components:

##### Application Layer (`src/application.cpp`)
- **Purpose**: Main application entry point and lifecycle management
- **Responsibilities**:
  - Initialize GLFW window and OpenGL context
  - Load configuration from YAML
  - Initialize Python interpreter
  - Manage application lifecycle (init, loop, destroy)

```cpp
int application_init(const std::string& project_path);
int application_loop();
int application_destroy();
```

##### Python Integration (`src/backend/`)
- **py_scope.cpp/hpp**: Python interpreter scope management
- **py_agent.cpp/hpp**: Agent interface bindings
- **py_env.cpp/hpp**: Environment interface bindings
- **py_method.cpp/hpp**: Explainability method bindings
- **py_visualizable.cpp/hpp**: Visualization interface bindings
- **py_object.cpp/hpp**: Generic Python object handling
- **py_param.cpp/hpp**: Parameter system for GUI configuration
- **py_safe_wrapper.cpp/hpp**: Thread-safe Python operations

##### GUI Framework (`src/ui/`)
- **lab_layout.cpp/hpp**: Main application layout management
- **project_manager.cpp/hpp**: Project loading and saving
- **modules/**: Individual GUI modules
  - **pipeline.cpp/hpp**: Data processing pipeline
  - **inspector.cpp/hpp**: Object inspection and editing
  - **logger.cpp/hpp**: Logging and debugging
  - **preview.cpp/hpp**: Real-time preview rendering
  - **py_module_window.cpp/hpp**: Python module interface
- **widgets/**: Reusable UI components
  - **image_viewer.cpp/hpp**: Image display and manipulation
  - **scrollable_plot.cpp/hpp**: Data plotting and visualization
  - **splitter.cpp/hpp**: Resizable panel management
- **utility/**: Helper utilities
  - **drawing.cpp/h**: OpenGL drawing primitives
  - **gl_texture.cpp/hpp**: OpenGL texture management
  - **image_store.cpp/hpp**: Image caching and storage
  - **layout.cpp/hpp**: Layout management algorithms

### 2. Python Engine (`py/pearl/`)

The Python engine provides the machine learning and explainability functionality through a modular, extensible architecture.

#### Core Abstractions:

##### Environment Layer (`pearl/env.py`)
```python
class RLEnvironment(Visualizable):
    """Abstract base class for RL environments."""
    
    @abstractmethod
    def reset(self, seed=None, options=None) -> Tuple[Any, Dict[str, Any]]:
        """Reset environment to initial state."""
    
    @abstractmethod
    def step(self, action: Any) -> Tuple[Any, Dict[str, np.ndarray], bool, bool, Dict[str, Any]]:
        """Take action and return next state, reward, termination flags, and info."""
    
    @abstractmethod
    def render(self, mode: str = "human") -> Optional[np.ndarray]:
        """Render the environment."""
    
    @abstractmethod
    def get_observations(self) -> Any:
        """Get current observations."""
```

**Design Principles:**
- **Unified Interface**: All environments follow the same interface regardless of underlying implementation
- **Composability**: Environments can be wrapped and combined using the `ObservationWrapper` pattern
- **Visualization Support**: Built-in support for multiple visualization methods
- **Type Safety**: Strong typing with type hints for better development experience

##### Agent Layer (`pearl/agent.py`)
```python
class RLAgent(ABC):
    """Abstract base class for RL agents."""
    
    @abstractmethod
    def predict(self, observation: Any) -> np.ndarray:
        """Predict action probabilities given observation."""
    
    @abstractmethod
    def get_q_net(self) -> Any:
        """Get the Q-network for explainability analysis."""
```

**Design Principles:**
- **Model Agnostic**: Supports any PyTorch-based model architecture
- **Explainability Ready**: Provides access to internal networks for attribution analysis
- **Flexible Prediction**: Supports both deterministic and probabilistic action selection

##### Explainability Layer (`pearl/method.py`)
```python
class ExplainabilityMethod(Visualizable):
    """Abstract base class for explainability methods."""
    
    @abstractmethod
    def set(self, env: RLEnvironment) -> None:
        """Set the environment for the method."""
    
    @abstractmethod
    def prepare(self, agent: RLAgent) -> None:
        """Prepare the method with an agent."""
    
    @abstractmethod
    def explain(self, obs: np.ndarray) -> np.ndarray | Any:
        """Generate explanation for given observation."""
    
    @abstractmethod
    def value(self, obs: np.ndarray) -> float:
        """Compute explanation quality score."""
```

**Design Principles:**
- **Lifecycle Management**: Clear setup and teardown phases
- **Step-by-Step Analysis**: Support for both pre-step and post-step analysis
- **Quality Metrics**: Built-in quality assessment for explanations
- **Visualization Integration**: Native support for multiple visualization types

##### Masking Layer (`pearl/mask.py`)
```python
class Mask(ABC):
    """Abstract base class for action masking."""
    
    @abstractmethod
    def update(self, observation: Any) -> None:
        """Update the mask based on the current observation."""
    
    @abstractmethod
    def compute(self, values: Any) -> np.ndarray:
        """Compute mask scores for the given values."""
```

**Design Principles:**
- **Domain Specific**: Masks can be tailored to specific environments and tasks
- **Dynamic Updates**: Masks can adapt based on current observations
- **Score Computation**: Provides quantitative assessment of action relevance

### 3. Data Flow Architecture

#### Primary Data Flow

```
Environment → Agent → Explainability Method → Mask → Visualization
     ↓           ↓           ↓                ↓         ↓
  Observations → Q-Values → Attributions → Scores → Visual Output
```

#### Detailed Flow Breakdown

1. **Environment Step**:
   ```python
   obs, reward_dict, terminated, truncated, info = env.step(action)
   ```

2. **Agent Prediction**:
   ```python
   action_probs = agent.predict(obs)
   q_values = agent.get_q_net()(obs)
   ```

3. **Explainability Analysis**:
   ```python
   # Pre-step analysis
   explainer.onStep(action)
   
   # Post-step analysis
   explainer.onStepAfter(action, reward_dict, terminated, info)
   
   # Generate explanation
   explanation = explainer.explain(obs)
   quality_score = explainer.value(obs)
   ```

4. **Masking and Scoring**:
   ```python
   mask.update(obs)
   scores = mask.compute(explanation)
   ```

5. **Visualization**:
   ```python
   visualization = explainer.getVisualization(method, params)
   ```

#### Threading Model

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Main Thread   │    │  Python Thread  │    │  Render Thread  │
│                 │    │                 │    │                 │
│ • GUI Events    │◄──►│ • ML Processing │◄──►│ • OpenGL Render │
│ • User Input    │    │ • Explainability│    │ • Visualization │
│ • Project Mgmt  │    │ • Data Analysis │    │ • UI Updates    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

**Thread Safety:**
- **GIL Management**: Python operations are properly synchronized using GIL
- **Safe Wrappers**: `py_safe_wrapper` provides thread-safe Python operations
- **Async Operations**: Long-running ML operations are performed asynchronously
- **UI Responsiveness**: GUI remains responsive during computation

### 4. Memory Management

#### C++ Memory Management
- **RAII**: Resource Acquisition Is Initialization for automatic cleanup
- **Smart Pointers**: Use of `std::unique_ptr` and `std::shared_ptr` for automatic memory management
- **OpenGL Resources**: Proper cleanup of textures, buffers, and shaders
- **Python Objects**: Careful management of Python object lifetimes

#### Python Memory Management
- **Reference Counting**: Automatic garbage collection for Python objects
- **NumPy Arrays**: Efficient memory layout for numerical computations
- **PyTorch Tensors**: GPU memory management for deep learning operations
- **Context Managers**: Proper resource cleanup using `with` statements

### 5. Configuration System

#### YAML Configuration (`config.yaml`)
```yaml
version: 1.0-beta
window:
  width: 1280
  height: 720
venv: /path/to/python/environment
```

#### Project Configuration (`projects/`)
- **Graph Configuration**: JSON-based pipeline configuration
- **Module Settings**: Per-module parameter storage
- **Visualization Settings**: Display and rendering preferences

#### Parameter System (`py/annotations.py`)
```python
class Param:
    def __init__(self, typ, editable=True, range_start=None, range_end=None,
                 isFilePath=None, choices=None, default=None, disc=None):
```

**Features:**
- **Type Safety**: Strong typing for all parameters
- **GUI Integration**: Automatic UI generation from parameter definitions
- **Validation**: Range checking and choice validation
- **Documentation**: Built-in parameter descriptions

### 6. Extension Points

#### Adding New Environments

1. **Inherit from Base Class**:
   ```python
   class CustomEnvironment(RLEnvironment):
       def __init__(self):
           super().__init__()
           # Initialize environment-specific components
   ```

2. **Implement Required Methods**:
   ```python
   def reset(self, seed=None, options=None):
       # Reset logic
       return observation, info
   
   def step(self, action):
       # Step logic
       return observation, reward_dict, terminated, truncated, info
   ```

3. **Add Visualization Support**:
   ```python
   def supports(self, m: VisualizationMethod) -> bool:
       return m == VisualizationMethod.RGB_ARRAY
   
   def getVisualization(self, m: VisualizationMethod, params=None):
       # Return appropriate visualization
   ```

#### Adding New Agents

1. **Inherit from Base Class**:
   ```python
   class CustomAgent(RLAgent):
       def __init__(self, model_path: str):
           super().__init__()
           # Load model
   ```

2. **Implement Required Methods**:
   ```python
   def predict(self, observation):
       # Prediction logic
       return action_probs
   
   def get_q_net(self):
       # Return Q-network for explainability
       return self.q_network
   ```

#### Adding New Explainability Methods

1. **Inherit from Base Class**:
   ```python
   class CustomExplainability(ExplainabilityMethod):
       def __init__(self, device: torch.device, mask: Mask):
           super().__init__()
           # Initialize method-specific components
   ```

2. **Implement Required Methods**:
   ```python
   def set(self, env: RLEnvironment):
       # Setup environment-specific components
   
   def prepare(self, agent: RLAgent):
       # Setup agent-specific components
   
   def explain(self, obs):
       # Generate explanations
       return explanation
   
   def value(self, obs):
       # Compute quality score
       return score
   ```

3. **Add Visualization Support**:
   ```python
   def supports(self, m: VisualizationMethod) -> bool:
       return m in [VisualizationMethod.RGB_ARRAY, VisualizationMethod.HEAT_MAP]
   
   def getVisualization(self, m: VisualizationMethod, params=None):
       # Return appropriate visualization
   ```

### 7. Performance Considerations

#### Optimization Strategies

1. **Caching**:
   - **Explanation Caching**: Store computed explanations to avoid recomputation
   - **Visualization Caching**: Cache rendered visualizations
   - **Model Caching**: Keep models in memory for faster inference

2. **Batch Processing**:
   - **Multiple Observations**: Process multiple observations simultaneously
   - **Parallel Agents**: Evaluate multiple agents in parallel
   - **Vectorized Operations**: Use NumPy/PyTorch vectorization

3. **Memory Optimization**:
   - **Lazy Loading**: Load models and data on demand
   - **Memory Pools**: Reuse memory buffers for similar operations
   - **Garbage Collection**: Proper cleanup of temporary objects

4. **GPU Utilization**:
   - **CUDA Operations**: Leverage GPU for deep learning operations
   - **Memory Transfers**: Minimize CPU-GPU memory transfers
   - **Batch Processing**: Process multiple samples on GPU simultaneously

### 8. Error Handling

#### Error Handling Strategy

1. **Graceful Degradation**:
   - **Fallback Methods**: Provide alternative implementations when primary methods fail
   - **Default Values**: Use sensible defaults when parameters are invalid
   - **Error Recovery**: Attempt to recover from errors when possible

2. **Error Reporting**:
   - **Structured Logging**: Use structured logging for better error tracking
   - **User Feedback**: Provide clear error messages to users
   - **Debug Information**: Include debug information for developers

3. **Validation**:
   - **Input Validation**: Validate all inputs before processing
   - **State Validation**: Ensure system state is consistent
   - **Resource Validation**: Verify resources are available and valid

### 9. Security Considerations

#### Security Measures

1. **Input Validation**:
   - **File Path Validation**: Validate all file paths to prevent path traversal
   - **Parameter Validation**: Validate all parameters to prevent injection attacks
   - **Type Checking**: Ensure all inputs are of expected types

2. **Resource Management**:
   - **Memory Limits**: Set limits on memory usage to prevent DoS attacks
   - **File Size Limits**: Limit file sizes to prevent resource exhaustion
   - **Execution Time Limits**: Set timeouts for long-running operations

3. **Isolation**:
   - **Process Isolation**: Run untrusted code in separate processes
   - **Environment Isolation**: Use virtual environments for Python dependencies
   - **Network Isolation**: Restrict network access when not needed

## Conclusion

The PearlLab architecture provides a robust, extensible foundation for reinforcement learning explainability research. The modular design allows for easy addition of new environments, agents, and explainability methods, while the hybrid C++/Python approach provides both performance and flexibility. The comprehensive error handling, security measures, and performance optimizations ensure the framework is suitable for both research and production use. 