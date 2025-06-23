from abc import ABC, abstractmethod
import numpy as np
from typing import Any

from pearl.agent import RLAgent
from pearl.env import RLEnvironment


class ExplainabilityMethod(ABC):
    def __init__(self):
        """
        Initialize any variables needed for the method
        """
        self.env = None

    @abstractmethod
    def set(self, env: RLEnvironment):
        """
        Called once, when the method is attached to an environment and agent
        """
        self.env = env

    @abstractmethod
    def prepare(self, agents: list[RLAgent]):
        """
        Called once, when the method is attached to an agent
        """
        pass
    
    @abstractmethod
    def onStep(self, action: Any):
        """
        Called once when a step is taken into the environment
        (before it's taken)
        """
        pass

    @abstractmethod
    def onStepAfter(self, action: Any):
        """
        Called once when a step is taken into the environment
        (after it's taken)
        """
        pass

    @abstractmethod
    def explain(self, obs) -> np.ndarray | Any:
        """
        using the current state of the environment and agent, return the explainability for this
        method
        """
        pass

    @abstractmethod
    def value(self, obs) -> list[float]:
        """
        Should return one value for each agent, that indicates how well the agent preformed according to this method
        """
        pass