from typing import Any

import numpy as np
import shap
import torch

from pearl.agent import RLAgent
from pearl.env import RLEnvironment
from pearl.mask import Mask
from pearl.method import ExplainabilityMethod

# A little hack to fix the issue of shap not being able to handle Flatten layer
from shap.explainers._deep import deep_pytorch
deep_pytorch.op_handler['Flatten'] = deep_pytorch.passthrough

class ShapExplainability(ExplainabilityMethod):

    def __init__(self, device, mask: Mask):
        super().__init__()
        self.device = device
        self.explainer = None
        self.background = None
        self.mask = mask
        self.agents = None

    def set(self, env: RLEnvironment):
        super().set(env)
        self.background = torch.zeros(env.get_observations().shape).to(self.device)

    def prepare(self, agents: list[RLAgent]):
        self.explainer = []
        self.agents = agents
        for agent in agents:
            model = agent.get_q_net().to(self.device)
            self.explainer.append(shap.DeepExplainer(model, self.background))

    def onStep(self, action: Any):
        # nothing for shap
        pass

    def onStepAfter(self, action: Any):
        # nothing for shap
        pass

    def explain(self, obs) -> np.ndarray | Any:
        if self.explainer is None:
            raise ValueError("Explainer not set. Please call prepare() first.")

        obs_tensor = torch.as_tensor(obs, dtype=torch.float, device=self.device)
        result = []
        for explainer in self.explainer:
            result.append(explainer.shap_values(obs_tensor, check_additivity=False)) # Fixme: check_additivity should be true .. but I set it to false for now
        return result

    def value(self, obs) -> list[float]:
        explains = self.explain(obs)
        values = []
        obs_tensor = torch.as_tensor(obs, dtype=torch.float, device=self.device)
        for i, explain in enumerate(explains):
            agent = self.agents[i]
            self.mask.update(obs)
            scores = self.mask.compute(explain)
            action = agent.predict(obs_tensor)
            values.append(scores[action])
        return values