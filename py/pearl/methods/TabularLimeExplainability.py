from typing import Any, List
import numpy as np
import torch
from pearl.agent import RLAgent
from pearl.env import RLEnvironment
from pearl.mask import Mask
from pearl.method import ExplainabilityMethod
from pearl.custom_methods.customLime import CustomLimeTabularExplainer
from visual import VisualizationMethod
from annotations import Param


class TabularLimeVisualizationParams:
    action: Param(int) = 0


class TabularLimeExplainability(ExplainabilityMethod):
    def __init__(self, device: torch.device, mask: Mask, feature_names: List[str]):
        """
        :param device: torch device
        :param mask: Mask object for score computation
        :param feature_names: Names of each feature in the state vector
        """
        super().__init__()
        self.device = device
        self.mask = mask
        if isinstance(feature_names, str):
            self.feature_names = feature_names.split(",")
        else:
            self.feature_names = feature_names
        self.agent: RLAgent = None
       
        self.explainer = CustomLimeTabularExplainer(
            feature_names=self.feature_names,
        )
        self.last_explain = None

    def set(self, env: RLEnvironment):
        super().set(env)

    def prepare(self, agent: RLAgent):
        self.agent = agent

    def onStep(self, action: Any): pass
    def onStepAfter(self, action: Any, reward: dict, done: bool, info: dict): pass

    def explain(self, obs: np.ndarray) -> Any:
        if self.agent is None:
            raise ValueError("Call prepare() before explain().")

        obs_vec = obs.squeeze()
        if obs_vec.ndim == 2:
            obs_vec = obs_vec[0]

        model = self.agent.get_q_net().to(self.device).eval()

        # Predict function for LIME: mirror model preprocessing exactly
        def predict_fn(x: np.ndarray) -> np.ndarray:
            with torch.no_grad():
                x_tensor = torch.tensor(x, dtype=torch.float32).to(self.device)
                logits = model(x_tensor)
                probs = torch.softmax(logits, dim=1).cpu().numpy()
            return probs

        # Get predicted action for focusing LIME
        obs_tensor = torch.tensor(obs_vec.reshape(1, -1), dtype=torch.float32).to(self.device)
        with torch.no_grad():
            q_vals = model(obs_tensor)
        action = int(torch.argmax(q_vals))

        # Explain only the predicted action for stability
        exp = self.explainer.explain_instance(
            data_row=obs_vec,
            predict_fn=predict_fn,
            num_features=len(self.feature_names),
            top_labels=action + 1  # ensure explanations include this label
        )
        # Store both explanation object and action
        self.last_explain = (exp, action)
        return exp

    def value(self, obs: np.ndarray) -> float:
        exp = self.explain(obs)
        self.mask.update(obs)

        obs_tensor = torch.tensor(obs, dtype=torch.float32, device=self.device).squeeze()
        with torch.no_grad():
            q_vals = self.agent.get_q_net()(obs_tensor)
            
        action = int(torch.argmax(q_vals))

        # Build weights array for this action
        weights = np.zeros(len(self.feature_names), dtype=float)
        for fid, weight in exp.local_exp.get(action, []):
            weights[fid] = weight

        # Attribution tensor shape (1, features, 1,1, action_space)
        attribution = np.abs(weights).reshape(1, len(self.feature_names), 1, 1, 1)
        attribution = np.broadcast_to(
            attribution,
            (1, len(self.feature_names), 1, 1, self.mask.action_space)
        ).astype(np.float32)
        # Normalize
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
        return m == VisualizationMethod.BAR_CHART

    def getVisualizationParamsType(self, m: VisualizationMethod) -> type | None:
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)
        if m == VisualizationMethod.BAR_CHART:
            return TabularLimeVisualizationParams
        return None

    def getVisualization(self, m: VisualizationMethod, params: Any = None) -> dict | None:
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)
        if m == VisualizationMethod.BAR_CHART:
            # Return empty dict if no explanation exists
            if self.last_explain is None:
                return {name: 0.0 for name in self.feature_names}
            exp, action = self.last_explain
            idx = 0
            if params is not None and isinstance(params, TabularLimeVisualizationParams):
                idx = params.action
            idx = max(0, idx) % self.mask.action_space
            # Return a dict of feature name: importance
            return {self.feature_names[fid]: weight for fid, weight in exp.local_exp.get(action, [])}
        return None
