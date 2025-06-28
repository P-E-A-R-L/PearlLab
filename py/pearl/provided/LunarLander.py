import numpy as np
from pearl.mask import Mask

class LunarLanderTabularMask(Mask):
    """
    Dynamic heuristic mask for LunarLander with 8D state.
    Weights each feature by importance and current state value to emphasize relevant actions.
    Features: [x_pos, y_pos, x_vel, y_vel, angle, angular_vel, leg1_contact, leg2_contact]
    Actions: [Do nothing, Fire left, Fire main, Fire right]
    """
    # max abs values for normalization
    # feature space: Box([ -2.5 -2.5 -10. -10. -6.2831855 -10. 0. 0. ], [ 2.5 2.5 10. 10. 6.2831855 10. 1. 1. ], (8,), float32)
    _feature_scales = np.array([2.5, 2.5, 10.0, 10.0, 6.2831855, 10.0, 1.0, 1.0 ], dtype=np.float32)

    def __init__(self):
        super().__init__(4)
        self.action_space = 4
        self.weights = self._define_weights()
        self.last_obs = None

    def _define_weights(self) -> np.ndarray:
        # Base static weights shape: (features, actions)
        w = np.zeros((8, self.action_space), dtype=np.float32)
        # Do nothing: prefer stable (low velocity & angle)
        w[:, 0] = np.array([0.09308317, 0.09749602, 0.23993654, 0.33300868, 0.09240652, 0.11172584, 0.01716148, 0.01518175])
        # Fire left (rotate right): act when x_vel < 0 or angle < 0
        w[:, 1] = np.array([0.10035031, 0.0978269,  0.30228677, 0.22909508, 0.09797481, 0.15964805, 0.00675054, 0.00606754])
        # Fire main: when falling fast (y_vel negative) and far from pad (y_pos high)
        w[:, 2] = np.array([0.09868395, 0.09582568, 0.22472089, 0.35638494, 0.0984113,  0.10176454, 0.01516911, 0.00903959])
        # Fire right (rotate left): act when x_vel > 0 or angle > 0
        w[:, 3] = np.array([0.10791439, 0.10658454, 0.28125881, 0.21808978, 0.10598366, 0.16484477, 0.00851407, 0.00680997])
        return w * 10 

    def update(self, obs: np.ndarray):
        # obs shape: (1,8)
        self.last_obs = obs.reshape(-1)

    def compute(self, attr: np.ndarray) -> np.ndarray:
        # attr: (1, features, 1, 1, actions)
        _, C, _, _, A = attr.shape
        if self.last_obs is None:
            # fallback to static weighting if no state
            return np.sum(self.weights * np.sum(attr, axis=(0,2,3)), axis=0)

        # normalize obs
        scaled = np.abs(self.last_obs) / self._feature_scales
        # clip to [0,1] 
        scaled = np.clip(scaled, 0.0, 1.0)

        scores = np.zeros(A, dtype=np.float32)
        # dynamic weight = static weight * scaled state magnitude
        for a in range(A):
            for c in range(C):
                feature_attr = attr[0, c, 0, 0, a]
                # dyn_w = self.weights[c, a] * (1.0 + scaled[c])  # amplify weight by state
                scores[a] += feature_attr * self.weights[c, a] 
                
        # normalize scores to [0, 1]
        # scores = np.clip(scores, 0.0, None)
        # scores /= np.sum(scores) if np.sum(scores) > 0 else 1.0
        return scores
