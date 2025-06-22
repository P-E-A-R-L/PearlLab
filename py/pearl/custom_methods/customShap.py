import numpy as np
import torch
from sklearn.linear_model import Ridge, Lasso
from typing import Callable


class CustomShapTabularExplainer:
    def __init__(
        self,
        model: Callable,
        feature_dim: int,
        num_samples: int = 100,
        use_lasso: bool = False,
        normalize_inputs: bool = False,
        mask_k: int = None
    ):
        """
        :param model: PyTorch model returning Q-values from tabular features, shape (N, d) -> (N, A)
        :param feature_dim: number of features d
        :param num_samples: number of perturbation samples
        :param use_lasso: if True, use Lasso for regression; else Ridge
        :param normalize_inputs: whether to normalize inputs using trivial baseline stats (no real data) — not recommended
        :param mask_k: if set, use masks with exactly k features on
        """
        self.model = model.eval()
        self.device = next(model.parameters()).device
        self.num_samples = num_samples
        self.use_lasso = use_lasso
        self.normalize = normalize_inputs
        self.mask_k = mask_k
        self.feature_dim = feature_dim

        # Trivial "background": zeros
        # If normalize_inputs=True, mean=0, std=1 so normalized input = input itself
        self.background = torch.zeros((1, feature_dim), dtype=torch.float32, device=self.device)
        if normalize_inputs:
            # No real statistics, so mean=0, std=1: no change effectively
            self.mean = torch.zeros((1, feature_dim), device=self.device)
            self.std = torch.ones((1, feature_dim), device=self.device)
        else:
            self.mean = None
            self.std = None

    def _sample_masks(self, num_features: int) -> np.ndarray:
        if self.mask_k is not None:
            masks = np.zeros((self.num_samples, num_features), dtype=float)
            for i in range(self.num_samples):
                idxs = np.random.choice(num_features, size=self.mask_k, replace=False)
                masks[i, idxs] = 1.0
        else:
            masks = np.random.randint(0, 2, size=(self.num_samples, num_features)).astype(float)
        return masks

    def shap_values(self, input_tensor: torch.Tensor) -> list[np.ndarray]:
        """
        input_tensor: shape (1, d)
        Returns list of arrays, one per action, shape (d,) each.
        """
        input_tensor = input_tensor.to(self.device)
        input_np = input_tensor.squeeze(0).detach().cpu().numpy()  # shape (d,)

        # Optionally normalize (here trivial if mean=0,std=1)
        if self.normalize:
            input_norm = (input_tensor - self.mean) / self.std
            input_np = input_norm.squeeze(0).cpu().numpy()

        num_features = self.feature_dim

        # Backgrounds: zeros
        background_np = np.zeros((self.num_samples, num_features), dtype=float)

        # Sample masks
        masks = self._sample_masks(num_features)  # shape (num_samples, d)

        # Create masked inputs: mask * input + (1-mask) * background(=0) = mask * input
        masked_inputs = masks * input_np[None, :]

        # Evaluate model in batch
        batch_tensor = torch.tensor(masked_inputs, dtype=torch.float32).to(self.device)
        with torch.no_grad():
            q_values_batch = self.model(batch_tensor).cpu().numpy()  # shape (num_samples, A)

        num_actions = q_values_batch.shape[1]
        shap_vals = np.zeros((num_features, num_actions), dtype=float)

        for action in range(num_actions):
            y = q_values_batch[:, action]
            reg = Lasso(alpha=0.01) if self.use_lasso else Ridge(alpha=1.0)
            reg.fit(masks, y)
            shap_vals[:, action] = reg.coef_

        # Return list of arrays, one per action
        # shape choices: here return 1D arrays of length d for each action
        return [shap_vals[:, a] for a in range(num_actions)]
