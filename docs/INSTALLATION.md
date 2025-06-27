# PearlLab Installation Guide

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Prerequisites](#prerequisites)
3. [Installation Methods](#installation-methods)
4. [Platform-Specific Instructions](#platform-specific-instructions)
5. [Dependency Management](#dependency-management)
6. [Configuration](#configuration)
7. [Verification](#verification)
8. [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements

- **Operating System**: macOS 13.0+, Ubuntu 20.04+, Windows 10+ (experimental)
- **Python**: 3.8 or higher
- **C++ Compiler**: Clang 12+ (macOS), GCC 9+ (Linux), MSVC 2019+ (Windows)
- **CMake**: 3.28 or higher
- **Memory**: 8 GB RAM minimum, 16 GB recommended
- **Storage**: 5 GB free space
- **GPU**: Optional, CUDA 11.0+ for GPU acceleration

### Recommended Requirements

- **Operating System**: macOS 14.0+, Ubuntu 22.04+
- **Python**: 3.9 or higher
- **C++ Compiler**: Clang 15+ (macOS), GCC 11+ (Linux)
- **CMake**: 3.30 or higher
- **Memory**: 32 GB RAM
- **Storage**: 20 GB free space
- **GPU**: NVIDIA GPU with 8+ GB VRAM, CUDA 12.0+

## Prerequisites

### System Dependencies

#### macOS

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install system dependencies
brew install cmake boost glfw yaml-cpp python@3.9

# Install Xcode Command Line Tools (if not already installed)
xcode-select --install
```

#### Ubuntu/Debian

```bash
# Update package list
sudo apt update

# Install system dependencies
sudo apt install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libglfw3-dev \
    libyaml-cpp-dev \
    python3 \
    python3-pip \
    python3-venv \
    git \
    wget \
    curl

# Install additional dependencies for development
sudo apt install -y \
    pkg-config \
    libssl-dev \
    libffi-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev
```

#### CentOS/RHEL/Fedora

```bash
# For CentOS/RHEL 8+
sudo dnf install -y \
    gcc-c++ \
    cmake \
    boost-devel \
    glfw-devel \
    yaml-cpp-devel \
    python3 \
    python3-pip \
    python3-devel \
    git

# For Fedora
sudo dnf install -y \
    gcc-c++ \
    cmake \
    boost-devel \
    glfw-devel \
    yaml-cpp-devel \
    python3 \
    python3-pip \
    python3-devel \
    git
```

#### Windows (Experimental)

```bash
# Install Visual Studio 2019 or 2022 with C++ workload
# Download from: https://visualstudio.microsoft.com/downloads/

# Install vcpkg for package management
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg install boost glfw3 yaml-cpp

# Install Python from python.org
# Download Python 3.9+ from: https://www.python.org/downloads/
```

### Python Environment

#### Create Virtual Environment

```bash
# Create virtual environment
python3 -m venv pearl_env

# Activate virtual environment
# On macOS/Linux:
source pearl_env/bin/activate

# On Windows:
pearl_env\Scripts\activate
```

#### Install Python Dependencies

```bash
# Upgrade pip
pip install --upgrade pip

# Install Python dependencies
cd py/pearl
pip install -r requirements.txt
```

## Installation Methods

### Method 1: Build from Source (Recommended)

#### Step 1: Clone Repository

```bash
# Clone the repository
git clone https://github.com/your-org/pearllab.git
cd PearlLab

# Or download and extract if you have the source code
```

#### Step 2: Configure Build

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_EXECUTABLE=$(which python3) \
    -DCMAKE_PREFIX_PATH=/opt/homebrew  # macOS with Homebrew
```

#### Step 3: Build Project

```bash
# Build the project
make -j$(nproc)  # Linux/macOS
# or
make -j%NUMBER_OF_PROCESSORS%  # Windows
```

#### Step 4: Install Python Package

```bash
# Install PearlLab Python package
cd py/pearl
pip install -e .
```

### Method 2: Docker Installation

#### Create Dockerfile

```dockerfile
# Dockerfile for PearlLab
FROM ubuntu:22.04

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libglfw3-dev \
    libyaml-cpp-dev \
    python3 \
    python3-pip \
    python3-venv \
    git \
    wget \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /pearllab

# Copy source code
COPY . .

# Create virtual environment
RUN python3 -m venv pearl_env
ENV PATH="/pearllab/pearl_env/bin:$PATH"

# Install Python dependencies
RUN pip install --upgrade pip
RUN cd py/pearl && pip install -r requirements.txt

# Build C++ components
RUN mkdir build && cd build \
    && cmake .. \
    && make -j$(nproc)

# Set environment variables
ENV PYTHONPATH="/pearllab/py:$PYTHONPATH"

# Expose port for GUI (if needed)
EXPOSE 8080

# Default command
CMD ["./build/PearlLab"]
```

#### Build and Run Docker Container

```bash
# Build Docker image
docker build -t pearllab .

# Run container
docker run -it --rm \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    pearllab
```

### Method 3: Conda Installation

#### Create Conda Environment

```bash
# Create conda environment
conda create -n pearllab python=3.9
conda activate pearllab

# Install conda packages
conda install -c conda-forge \
    cmake \
    boost \
    glfw \
    yaml-cpp \
    pytorch \
    torchvision \
    torchaudio \
    numpy \
    matplotlib \
    scikit-learn

# Install remaining Python packages
pip install -r py/pearl/requirements.txt
```

## Platform-Specific Instructions

### macOS

#### Xcode Setup

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Verify installation
xcode-select -p
# Should output: /Applications/Xcode.app/Contents/Developer

# Set SDK path in CMakeLists.txt
set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
```

#### Homebrew Dependencies

```bash
# Install all dependencies via Homebrew
brew install \
    cmake \
    boost \
    glfw \
    yaml-cpp \
    python@3.9 \
    llvm

# Set compiler to use LLVM
export CC=/opt/homebrew/opt/llvm/bin/clang
export CXX=/opt/homebrew/opt/llvm/bin/clang++
```

#### Build Configuration

```bash
# Configure with macOS-specific settings
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
    -DCMAKE_PREFIX_PATH=/opt/homebrew \
    -DPYTHON_EXECUTABLE=/opt/homebrew/bin/python3
```

### Linux

#### Ubuntu/Debian

```bash
# Install all dependencies
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libglfw3-dev \
    libyaml-cpp-dev \
    python3 \
    python3-pip \
    python3-venv \
    python3-dev \
    git \
    wget \
    curl \
    pkg-config \
    libssl-dev \
    libffi-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev

# Build configuration
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_EXECUTABLE=$(which python3)
```

#### CentOS/RHEL

```bash
# Enable EPEL repository
sudo yum install -y epel-release

# Install dependencies
sudo yum groupinstall -y "Development Tools"
sudo yum install -y \
    cmake3 \
    boost-devel \
    glfw-devel \
    yaml-cpp-devel \
    python3 \
    python3-pip \
    python3-devel \
    git

# Use cmake3 instead of cmake
cmake3 .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DPYTHON_EXECUTABLE=$(which python3)
```

### Windows (Experimental)

#### Visual Studio Setup

1. **Install Visual Studio 2019 or 2022**
   - Download from: https://visualstudio.microsoft.com/downloads/
   - Install with "Desktop development with C++" workload
   - Include Windows 10/11 SDK

2. **Install vcpkg**
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   bootstrap-vcpkg.bat
   vcpkg integrate install
   ```

3. **Install Dependencies**
   ```cmd
   vcpkg install boost glfw3 yaml-cpp
   ```

4. **Build Configuration**
   ```cmd
   cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```

## Dependency Management

### Python Dependencies

#### Core Dependencies

```bash
# Install core ML dependencies
pip install torch torchvision torchaudio
pip install stable-baselines3
pip install gymnasium[atari]
pip install ale-py

# Install explainability libraries
pip install shap
pip install lime
pip install scikit-learn

# Install visualization libraries
pip install matplotlib
pip install opencv-python
pip install pillow

# Install utility libraries
pip install numpy
pip install pandas
pip install tqdm
pip install rich
```

#### Development Dependencies

```bash
# Install development tools
pip install pytest
pip install pytest-cov
pip install black
pip install flake8
pip install mypy
pip install pre-commit
```

### C++ Dependencies

#### Required Libraries

- **Boost**: C++ utility libraries
- **GLFW**: OpenGL framework
- **yaml-cpp**: YAML parsing
- **pybind11**: Python-C++ bindings
- **OpenGL**: Graphics rendering

#### Version Compatibility

```bash
# Check library versions
pkg-config --modversion glfw3
pkg-config --modversion yaml-cpp

# Boost version check
grep "BOOST_VERSION" /usr/include/boost/version.hpp
```

## Configuration

### Environment Configuration

#### Virtual Environment Path

```bash
# Set virtual environment path in config.yaml
cat > config.yaml << EOF
version: 1.0-beta
window:
  width: 1280
  height: 720
venv: $(pwd)/pearl_env
EOF
```

#### Environment Variables

```bash
# Set environment variables
export PYTHONPATH="${PYTHONPATH}:$(pwd)/py"
export PEARLLAB_CONFIG="$(pwd)/config.yaml"
export PEARLLAB_MODELS="$(pwd)/models"
```

#### Python Path Configuration

```bash
# Add PearlLab to Python path
echo "export PYTHONPATH=\"\${PYTHONPATH}:$(pwd)/py\"" >> ~/.bashrc
source ~/.bashrc
```

### Project Configuration

#### Create Project Structure

```bash
# Create project directories
mkdir -p projects/my_project
mkdir -p models
mkdir -p data
mkdir -p logs

# Create project configuration
cat > projects/my_project/config.json << EOF
{
  "name": "my_project",
  "version": "1.0.0",
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
EOF
```

## Verification

### Basic Functionality Test

```python
# Test basic imports
import torch
import numpy as np
from pearl.enviroments.GymRLEnv import GymRLEnv
from pearl.agents.SimpleDQN import DQNAgent
from pearl.methods.ShapExplainability import ShapExplainability

print("✓ All imports successful")

# Test environment creation
env = GymRLEnv("ALE/Assault-v5", stack_size=4, frame_skip=4)
obs, info = env.reset()
print(f"✓ Environment created, observation shape: {obs.shape}")

# Test agent loading (if model exists)
try:
    agent = DQNAgent("models/dqn_assault_5m.pth")
    print("✓ Agent loaded successfully")
except FileNotFoundError:
    print("⚠ Model file not found, skipping agent test")

# Test explainability method
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"✓ Using device: {device}")
```

### GUI Application Test

```bash
# Test GUI application
./PearlLab

# Check for any error messages
# The application should open with the main interface
```

### Performance Test

```python
# Performance benchmark
import time

def benchmark_explanation(explainer, obs, num_runs=100):
    start_time = time.time()
    for _ in range(num_runs):
        explanation = explainer.explain(obs)
    end_time = time.time()
    
    avg_time = (end_time - start_time) / num_runs
    print(f"Average explanation time: {avg_time:.4f} seconds")
    return avg_time

# Run benchmark
if 'explainer' in locals() and 'obs' in locals():
    benchmark_explanation(explainer, obs)
```

## Troubleshooting

### Common Issues

#### 1. CMake Configuration Errors

```bash
# Error: Could not find Boost
# Solution: Install Boost development libraries
sudo apt install libboost-all-dev  # Ubuntu/Debian
brew install boost  # macOS

# Error: Could not find Python
# Solution: Specify Python executable
cmake .. -DPYTHON_EXECUTABLE=$(which python3)

# Error: Could not find OpenGL
# Solution: Install OpenGL development libraries
sudo apt install libgl1-mesa-dev  # Ubuntu/Debian
# macOS: OpenGL is included with Xcode
```

#### 2. Build Errors

```bash
# Error: Compiler not found
# Solution: Install C++ compiler
sudo apt install build-essential  # Ubuntu/Debian
xcode-select --install  # macOS

# Error: C++20 not supported
# Solution: Update compiler or use older C++ standard
cmake .. -DCMAKE_CXX_STANDARD=17

# Error: Memory exhausted during compilation
# Solution: Reduce parallel jobs
make -j2  # Instead of make -j$(nproc)
```

#### 3. Python Import Errors

```bash
# Error: Module not found
# Solution: Check Python path
echo $PYTHONPATH
export PYTHONPATH="${PYTHONPATH}:$(pwd)/py"

# Error: Version conflicts
# Solution: Use virtual environment
python3 -m venv pearl_env
source pearl_env/bin/activate
pip install -r requirements.txt

# Error: CUDA not available
# Solution: Install CUDA or use CPU
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cpu
```

#### 4. Runtime Errors

```bash
# Error: GLFW window creation failed
# Solution: Check display settings
export DISPLAY=:0  # Linux
# macOS: Usually works automatically

# Error: OpenGL context creation failed
# Solution: Update graphics drivers
# Linux: sudo apt install mesa-utils
# macOS: Update to latest macOS version

# Error: Model file not found
# Solution: Download or train models
wget https://example.com/models/dqn_assault_5m.pth -O models/dqn_assault_5m.pth
```

### Debug Mode

#### Enable Debug Logging

```bash
# Set debug environment variables
export PEARLLAB_DEBUG=1
export PEARLLAB_LOG_LEVEL=DEBUG

# Run with debug output
./PearlLab --debug
```

#### Verbose Build

```bash
# Build with verbose output
make VERBOSE=1

# CMake with debug info
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Performance Issues

#### Memory Optimization

```bash
# Monitor memory usage
htop  # Linux/macOS
# or
top

# Reduce memory usage
export OMP_NUM_THREADS=4  # Limit OpenMP threads
export MKL_NUM_THREADS=4  # Limit MKL threads
```

#### GPU Issues

```bash
# Check CUDA installation
nvidia-smi
nvcc --version

# Test PyTorch CUDA
python -c "import torch; print(torch.cuda.is_available())"

# Force CPU usage if GPU issues persist
export CUDA_VISIBLE_DEVICES=""
```

### Getting Help

#### Log Files

```bash
# Check log files
ls -la logs/
cat logs/pearllab.log

# Enable detailed logging
export PEARLLAB_LOG_FILE=logs/detailed.log
```

#### Community Support

- **GitHub Issues**: Report bugs and request features
- **Documentation**: Check this guide and API documentation
- **Discussions**: Use GitHub discussions for questions
- **Email**: Contact maintainers for critical issues

#### System Information

```bash
# Collect system information for bug reports
echo "=== System Information ==="
uname -a
python3 --version
gcc --version
cmake --version
nvidia-smi  # If available
echo "=== Python Packages ==="
pip list
echo "=== Environment Variables ==="
env | grep -E "(PYTHON|PEARL|CUDA)"
```

This comprehensive installation guide should help you successfully install and configure PearlLab on your system. If you encounter any issues not covered here, please refer to the troubleshooting section or contact the maintainers. 