from abc import abstractmethod
from typing import Optional, Dict, Any

from pearl.env import RLEnvironment


class ObservationWrapper(RLEnvironment):
    """
    Abstract wrapper that forwards all calls to an underlying env,
    except reset() and get_observations(), which subclasses must implement.
    """
    def __init__(self, env):
        self.env = env
        self.observation_space = env.observation_space
        self.action_space = env.action_space
        self.metadata = getattr(env, 'metadata', {})

    @abstractmethod
    def reset(self, seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None):
        pass

    @abstractmethod
    def step(self, action: Any):
        pass

    def render(self, mode: str = "human"):  # type: ignore
        return self.env.render(mode=mode)

    def close(self):
        return self.env.close()

    def seed(self, seed: Optional[int] = None):
        return self.env.seed(seed)

    def get_available_actions(self):
        return super().get_available_actions()

    @abstractmethod
    def get_observations(self):
        pass