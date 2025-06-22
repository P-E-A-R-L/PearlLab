from typing import Any, List
import numpy as np
import torch

from pearl.agent import RLAgent
from pearl.env import RLEnvironment
from pearl.mask import Mask
from pearl.method import ExplainabilityMethod
from pearl.custom_methods.customShap import CustomShapTabularExplainer
from visual import VisualizationMethod
from annotations import Param


class TabularShapVisualizationParams:
    action: Param(int) = 0


class TabularShapExplainability(ExplainabilityMethod):
    def __init__(self, device: torch.device, mask: Mask, feature_names: List[str]):
        super().__init__()
        self.device = device
        self.mask = mask
        if isinstance(feature_names, str):
            self.feature_names = feature_names.split(",")
        else:
            self.feature_names = feature_names
        self.agent: RLAgent = None
        self.explainer = None
        self.last_explain = None

    def set(self, env: RLEnvironment):
        super().set(env)

    def prepare(self, agent: RLAgent):
        self.agent = agent
        model = agent.get_q_net().to(self.device).eval()
        feature_dim = len(self.feature_names)
        self.explainer = CustomShapTabularExplainer(
            model,
            feature_dim,
            num_samples=100,
            use_lasso=False,
            normalize_inputs=False,
            mask_k=None
        )

    def onStep(self, action: Any): pass
    def onStepAfter(self, action: Any, reward: dict, done: bool, info: dict): pass

    def explain(self, obs: np.ndarray) -> Any:
        if self.explainer is None:
            raise ValueError("Explainer not initialized. Call prepare() first.")

        obs_vec = obs.squeeze()
        if obs_vec.ndim == 2:
            obs_vec = obs_vec[0]

        obs_tensor = torch.tensor(obs_vec.reshape(1, -1), dtype=torch.float32).to(self.device)
        shap_values = self.explainer.shap_values(obs_tensor)

        explanation = {
            'shap_values': shap_values,  # List/array per action
            'feature_names': self.feature_names,
            'data': obs_vec
        }
        self.last_explain = explanation
        return explanation

    def value(self, obs: np.ndarray) -> float:
        exp = self.explain(obs)
        self.mask.update(obs)

        obs_tensor = torch.tensor(obs, dtype=torch.float32, device=self.device).squeeze()
        with torch.no_grad():
            q_vals = self.agent.get_q_net()(obs_tensor)

        action = int(torch.argmax(q_vals))
        shap_values = exp['shap_values']
        action_shap = shap_values[action]

        weights = np.abs(action_shap).reshape(1, len(self.feature_names), 1, 1, 1)
        attribution = np.broadcast_to(
            weights,
            (1, len(self.feature_names), 1, 1, self.mask.action_space)
        ).astype(np.float32)

        total = np.sum(attribution, axis=1, keepdims=True)
        if total.any():
            attribution /= total

        score = float(self.mask.compute(attribution)[action])
        action_q = q_vals[action].item()
        max_q = torch.max(q_vals).item()
        confidence = action_q / max_q if max_q != 0 else 1.0

        return score * confidence

    def supports(self, m: VisualizationMethod) -> bool:
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)
        return m == VisualizationMethod.FEATURES

    def getVisualizationParamsType(self, m: VisualizationMethod) -> type | None:
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)
        if m == VisualizationMethod.FEATURES:
            return TabularShapVisualizationParams
        return None

    def getVisualization(self, m: VisualizationMethod, params: Any = None) -> dict | None:
        if self.last_explain is None:
            return {}
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)
        if m == VisualizationMethod.FEATURES:
            idx = 0
            if params is not None and isinstance(params, TabularShapVisualizationParams):
                idx = params.action
            idx = max(0, idx) % self.mask.action_space

            shap_vals = self.last_explain['shap_values'][idx]
            return {self.feature_names[i]: float(shap_vals[i]) for i in range(len(self.feature_names))}
        return None
