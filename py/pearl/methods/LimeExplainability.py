from typing import Any
import numpy as np
import torch
from skimage.color import gray2rgb
from skimage.transform import resize
from pearl.agent import RLAgent
from pearl.env import RLEnvironment
from pearl.mask import Mask
from pearl.method import ExplainabilityMethod
from visual import VisualizationMethod
from annotations import Param
from pearl.custom_methods.customLimeImage import CustomLimeImageExplainer

class LimeVisualizationParams:
    action: Param(int) = 0


class LimeExplainability(ExplainabilityMethod):
    """
    LIME explainability aligned with the new ShapExplainability interface.
    """
    def __init__(self, device: torch.device, mask: Mask):
        super().__init__()
        self.device = device
        self.mask = mask
        self.explainer = CustomLimeImageExplainer(num_samples=100, num_segments=1000)
        self.agent: RLAgent = None
        self.last_explain = None
        self.obs = None

    def set(self, env: RLEnvironment):
        super().set(env)

    def prepare(self, agent: RLAgent):
        self.agent = agent

    def onStep(self, action: Any): pass
    def onStepAfter(self, action: Any, reward: dict, done: bool, info: dict): pass

    def explain(self, obs: np.ndarray) -> Any:
        if self.agent is None:
            raise ValueError("Call prepare() before explain().")

        self.obs = obs
        frame = obs.squeeze()  # (C, H, W)
        img = np.transpose(frame, (1, 2, 0))
        if img.shape[2] != 3:
            img = img[:, :, :3] if img.shape[2] > 3 else gray2rgb(img[:, :, 0])

        def batch_predict(images: np.ndarray) -> np.ndarray:
            gr = np.mean(images, axis=3, keepdims=True).astype(np.float32) / 255.0
            stacked = np.repeat(gr, frame.shape[0], axis=3)
            tensor = torch.tensor(np.transpose(stacked, (0, 3, 1, 2)), dtype=torch.float32).to(self.device)
            with torch.no_grad():
                out = self.agent.get_q_net()(tensor)
            return out.cpu().numpy()

        exp = self.explainer.explain_instance(
            image=img.astype(np.double),
            classifier_fn=batch_predict,
            top_labels=self.mask.action_space,
            hide_color=0,
            num_samples=100,
        )

        self.last_explain = exp
        return exp

    def value(self, obs: np.ndarray) -> float:
        exp = self.explain(obs)
        self.mask.update(obs)
        obs_t = torch.as_tensor(obs, dtype=torch.float32, device=self.device)
        with torch.no_grad():
            q = self.agent.get_q_net()(obs_t)
        action = int(torch.argmax(q))

        segs = exp.segments
        A = self.mask.action_space
        C, H, W = obs.shape[1:]

        maps = np.zeros((A, H, W), dtype=float)
        for lbl, pairs in exp.local_exp.items():
            for seg_id, wt in pairs:
                maps[lbl, segs == seg_id] = wt

        if segs.shape != (H, W):
            maps = np.stack([resize(m, (H, W), preserve_range=True, anti_aliasing=True) for m in maps], axis=0)

        maps_hw_a = np.transpose(maps, (1, 2, 0))
        attributions = np.broadcast_to(maps_hw_a[None, None, :, :, :], (1, C, H, W, A)).astype(np.float32)
        attributions = np.abs(attributions)
        attributions = attributions / np.sum(attributions, axis=(1, 2, 3), keepdims=True)

        return float(self.mask.compute(attributions)[action])

    def supports(self, m: VisualizationMethod) -> bool:
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)
        return m == VisualizationMethod.HEAT_MAP

    def getVisualizationParamsType(self, m: VisualizationMethod) -> type | None:
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)
        if m == VisualizationMethod.HEAT_MAP:
            return LimeVisualizationParams
        return None

    def getVisualization(self, m: VisualizationMethod, params: Any = None) -> np.ndarray | dict | None:
        if self.last_explain is None:
            return np.zeros((336, 336), dtype=np.float32)
        if not isinstance(m, VisualizationMethod):
            m = VisualizationMethod(m)
        if m == VisualizationMethod.HEAT_MAP:
            idx = 0
            if params is not None and isinstance(params, LimeVisualizationParams):
                idx = params.action
            if idx < 0: idx = 0
            idx = idx % self.mask.action_space
            

            segs = self.last_explain.segments
            heatmap = np.zeros(segs.shape, dtype=np.float32)
            for segment, importance in self.last_explain.local_exp[idx]:
                heatmap[segs == segment] = importance
                
            resized_heatmap = resize(heatmap, (336, 336), preserve_range=True, anti_aliasing=True)
            return resized_heatmap
        return None