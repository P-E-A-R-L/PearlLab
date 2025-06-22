import numpy as np
from sklearn.linear_model import Ridge
from typing import Callable


class CustomExplanation:
    def __init__(self, local_exp: dict):
        self.local_exp = local_exp


class CustomLimeTabularExplainer:
    def __init__(
        self,
        feature_names: list[str],
        noise_scale: float = 0.1
    ):
        """
        A minimal LIME-like explainer without a background dataset.
        Perturbs each instance with Gaussian noise of given scale.
        :param feature_names: list of feature names (for bookkeeping, not used internally)
        :param noise_scale: relative scale of Gaussian noise for perturbations
        """
        self.feature_names = feature_names
        self.noise_scale = noise_scale

    def _generate_perturbations(self, instance: np.ndarray, num_samples: int) -> np.ndarray:
        # Gaussian noise around the instance
        noise = np.random.normal(0, 1, size=(num_samples, instance.shape[0]))
        perturbed = instance + noise * self.noise_scale
        return perturbed

    def _compute_distances(self, instance: np.ndarray, samples: np.ndarray) -> np.ndarray:
        distances = np.linalg.norm(samples - instance, axis=1)
        kernel_width = np.sqrt(samples.shape[1]) * 0.75
        weights = np.exp(-(distances ** 2) / (kernel_width ** 2))
        return weights

    def explain_instance(
        self,
        data_row: np.ndarray,
        predict_fn: Callable[[np.ndarray], np.ndarray],
        num_features: int = 10,
        top_labels: int = 1,
        num_samples: int = 5000
    ) -> CustomExplanation:
        """
        data_row: 1D array shape (d,)
        predict_fn: function mapping array (N, d) -> probabilities array (N, num_classes)
        """
        print(f"Explaining instance with {num_samples} samples...")
        # Generate perturbations
        perturbed = self._generate_perturbations(data_row, num_samples=num_samples)
        preds = predict_fn(perturbed)  # shape (num_samples, num_classes)
        weights = self._compute_distances(data_row, perturbed)

        explanations: dict[int, list[tuple[int, float]]] = {}
        num_classes = preds.shape[1]

        # For stability, only fit linear model for labels < top_labels
        for label in range(min(top_labels, num_classes)):
            y = preds[:, label]
            model = Ridge(alpha=1.0, fit_intercept=True)
            model.fit(perturbed, y, sample_weight=weights)
            importances = list(enumerate(model.coef_))
            importances.sort(key=lambda x: abs(x[1]), reverse=True)
            explanations[label] = importances[:num_features]

        return CustomExplanation(local_exp=explanations)
