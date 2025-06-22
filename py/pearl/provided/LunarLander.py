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
        w[:, 0] = np.array([0.03501646, 0.02868263, 0.67910854, 0.19795921, 0.00638668, 0.05284648, 0. , 0.])
        # Fire left (rotate right): act when x_vel < 0 or angle < 0
        w[:, 1] = np.array([0.00426566, 0.00824412, 0.84698007, 0.02906354, 0.02114819, 0.09029842, 0. , 0.])
        # Fire main: when falling fast (y_vel negative) and far from pad (y_pos high)
        w[:, 2] = np.array([0.00634047, 0.00430323, 0.31383184, 0.53397896, 0.06132264, 0.08022287, 0. , 0.])
        # Fire right (rotate left): act when x_vel > 0 or angle > 0
        w[:, 3] = np.array([0.00606135, 0.00799748, 0.77693051, 0.07101095, 0.03231421, 0.10568552, 0. , 0.])
        return w

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
                dyn_w = self.weights[c, a] * (1.0 + scaled[c])  # amplify weight by state
                scores[a] += dyn_w * feature_attr
                
        # normalize scores to [0, 1]
        # scores = np.clip(scores, 0.0, None)
        # scores /= np.sum(scores) if np.sum(scores) > 0 else 1.0
        return scores
