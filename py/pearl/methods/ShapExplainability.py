from typing import Any, Dict

import numpy as np
import shap
import torch

from pearl.agent import RLAgent
from pearl.env import RLEnvironment
from pearl.mask import Mask
from pearl.method import ExplainabilityMethod

from annotations import Param

# A little hack to fix the issue of shap not being able to handle Flatten layer
from shap.explainers._deep import deep_pytorch

from visual import VisualizationMethod

deep_pytorch.op_handler['Flatten'] = deep_pytorch.passthrough

class ShapVisualizationParams:
    action: Param(int) = 0

class ShapExplainability(ExplainabilityMethod):

    def __init__(self, device, mask: Mask):
        super().__init__()
        self.device = device
        self.explainer = None
        self.background = None
        self.mask = mask
        self.agent = None
        self.last_explain = None

    def set(self, env: RLEnvironment):
        super().set(env)
        self.background = torch.zeros(env.get_observations().shape).to(self.device)

    def prepare(self, agent: RLAgent):
        model = agent.get_q_net().to(self.device)
        self.explainer = shap.DeepExplainer(model, self.background)
        self.agent = agent

    def onStep(self, action: Any):
        # nothing for shap
        pass

    def onStepAfter(self, action: Any, reward: Dict[str, np.ndarray], done: bool, info: dict):
        # nothing for shap
        pass

    def explain(self, obs) -> np.ndarray | Any:
        if self.explainer is None:
            raise ValueError("Explainer not set. Please call prepare() first.")

        obs_tensor = torch.as_tensor(obs, dtype=torch.float, device=self.device)
        return self.explainer.shap_values(obs_tensor, check_additivity=False) # Fixme: check_additivity should be true .. but I set it to false for now

    def value(self, obs) -> float:
        explain = self.explain(obs)
        obs_tensor = torch.as_tensor(obs, dtype=torch.float, device=self.device)
        self.mask.update(obs)
        scores = self.mask.compute(explain)
        action = np.argmax(self.agent.predict(obs_tensor))
        self.last_explain = explain
        return scores[action]

    def supports(self, m: VisualizationMethod) -> bool:
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)

        return m == VisualizationMethod.HEAT_MAP

    def getVisualizationParamsType(self, m: VisualizationMethod) -> type | None:
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)

        if m == VisualizationMethod.HEAT_MAP:
            return ShapVisualizationParams
        return None

    def getVisualization(self, m: VisualizationMethod, params: Any = None) -> np.ndarray | dict | None:
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)

        if m == VisualizationMethod.HEAT_MAP:
            idx = 0
            if params is not None and isinstance(params, ShapVisualizationParams):
                idx = params.action
            if idx < 0: idx = 0
            idx = idx % len(self.last_explain)
            return self.last_explain[..., idx].mean(axis=(0, 1))
        return None