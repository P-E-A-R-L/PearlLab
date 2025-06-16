from typing import Any

import numpy as np

from pearl.mask import Mask

def split_vertical_lines(img: np.ndarray):
    """
    Given a 2D grayscale (or binary) image `img` of shape (H, W),
    returns two uint8 images (H, W):
      - vertical: all pixels that belong to 1‑pixel‑wide vertical runs
      - rest:      the original image with those vertical pixels zeroed out

    A pixel at (y,x) is classified as "vertical" if:
       img[y,x] != 0
       AND img[y, x-1] == 0  (or x == 0)
       AND img[y, x+1] == 0  (or x == W-1)

    This picks out any vertical stroke of width exactly one.
    """
    # flatten shape
    if img.ndim != 2:
        raise ValueError("Input must be a 2D array")
    H, W = img.shape

    # foreground mask (non-zero)
    fg = img != 0

    # shift provided left/right (out‑of‑bounds treated as False)
    left  = np.zeros_like(fg)
    right = np.zeros_like(fg)
    left[:, 1:]  = fg[:, :-1]
    right[:, :-1] = fg[:, 1:]

    # a pixel is a 1‑px vertical line if it's on and both neighbors are off
    vertical_mask = fg & ~left & ~right

    # build output images
    vertical = np.zeros_like(img, dtype=np.uint8)
    rest     = img.copy().astype(np.uint8)

    vertical[vertical_mask] = img[vertical_mask]
    rest[vertical_mask]     = 0

    return vertical, rest


def segment_assault_by_yranges(frame: np.ndarray):
    """
    Purely spatial segmentation of a grayscale Assault frame into 7 provided:
      1) mothership band
      2) bullets band
      3) enemies band
      4) player band
      5) all UI (top + bottom)
      6) lives UI only (bottom UI, left slice)
      7) cannon UI only

    Input:
      frame: H×W or H×W×1 uint8 array
    Returns:
      moth, bullets, enemies, player, ui_all, ui_lives, ui_cannon
      each an H×W uint8 mask (0 or original pixel)
    """
    # flatten to H×W
    if frame.ndim == 3 and frame.shape[2] == 1:
        img = frame[:, :, 0]
    else:
        img = frame
    H, W = img.shape

    # define breakpoints (fractions of H)
    top_ui_frac     = 0.08   # top 8% = scoreboard
    moth_frac       = 0.15
    enemy_frac      = 0.64
    # bottom UI fraction ~8%
    bottom_ui_frac  = 0.12
    bottom_ui_bar_frac = 0.04

    # compute pixel rows
    y0_top_ui    = 0
    y1_top_ui    = int(H * top_ui_frac)
    y0_moth      = y1_top_ui
    y1_moth      = y0_moth + int(H * moth_frac)
    y0_enemy     = y1_moth
    y1_enemy     = y0_enemy + int(H * enemy_frac)
    y0_player    = y1_enemy
    y1_player    = H - int(H * bottom_ui_frac)
    y0_bottom_ui = y1_player
    y1_bottom_ui = H

    # 1) Mothership mask: keep only rows [y0_moth:y1_moth]
    moth = np.zeros_like(img)
    moth[y0_moth:y1_moth, :] = img[y0_moth:y1_moth, :]

    # 2) Enemies mask: rows [y0_enemy:y1_enemy]
    enemies = np.zeros_like(img)
    enemies[y0_enemy:y1_enemy, :] = img[y0_enemy:y1_enemy, :]

    # 3) Bullets mask: rows [y0_bullets:y1_bullets]
    #    (this is just the playfield excluding moth/enemy bands)
    player = np.zeros_like(img)
    player[y0_player:y1_player, :] = img[y0_player:y1_player, :]

    # 4) All UI: top UI + bottom UI
    ui_all = np.zeros_like(img)
    ui_all[y0_top_ui:y1_top_ui, :]       = img[y0_top_ui:y1_top_ui, :]
    ui_all[y0_bottom_ui:y1_bottom_ui, :] = img[y0_bottom_ui:y1_bottom_ui, :]

    # 5) Lives-only UI: bottom UI, leftmost 25% of width
    lives_w = int(W * 0.5)
    ui_lives = np.zeros_like(img)
    ui_lives[y0_bottom_ui + int(H * bottom_ui_bar_frac):y1_bottom_ui, 0:lives_w] = img[y0_bottom_ui + int(H * bottom_ui_bar_frac):y1_bottom_ui, 0:lives_w]

    lives_w = int(W * 0.5)
    ui_cannon = np.zeros_like(img)
    ui_cannon[y0_bottom_ui + int(H * bottom_ui_bar_frac):y1_bottom_ui, lives_w:W] = img[y0_bottom_ui + int(H * bottom_ui_bar_frac):y1_bottom_ui, lives_w:W]

    return moth, *split_vertical_lines(enemies), player, ui_all, ui_lives, ui_cannon


def weighted_centroid(img: np.ndarray):
    """
    Compute the intensity-weighted centroid of a 2D grayscale image.

    Args:
      img: 2D numpy array of shape (H, W), dtype float or uint8.

    Returns:
      (x_centroid, y_centroid): floats giving the weighted average
        of the column and row indices, respectively.
    """
    # ensure float
    I = img.astype(np.float64)

    # coordinates
    H, W = I.shape
    ys = np.arange(H)[:, None]  # shape (H,1)
    xs = np.arange(W)[None, :]  # shape (1,W)

    total_intensity = I.sum()
    if total_intensity == 0:
        # avoid division by zero; return center of image
        return (W - 1) / 2.0, (H - 1) / 2.0

    x_center = (I * xs).sum() / total_intensity
    y_center = (I * ys).sum() / total_intensity

    return x_center, y_center


class AssaultEnvShapMask(Mask):
    """
    Mask for the ALE/Assault environment.
    """

    def __init__(self):
        super().__init__(7) # Gym assault game has 7 actions
        self.moth = []
        self.enemies = []
        self.player = []
        self.ui_all = []
        self.ui_lives = []
        self.bullets = []
        self.ui_cannon = []

    def update(self, frame: np.ndarray):
        self.moth = []
        self.enemies = []
        self.bullets = []
        self.player = []
        self.ui_all = []
        self.ui_lives = []
        self.ui_cannon = []

        N = frame.shape[1]
        for i in range(N):
            img = frame[0, i, :, :]
            moth, bullets, enemies, player, ui_all, ui_lives, ui_cannon = segment_assault_by_yranges(img)
            self.bullets.append(bullets)
            self.ui_cannon.append(ui_cannon)
            self.moth.append(moth)
            self.enemies.append(enemies)
            self.player.append(player)
            self.ui_all.append(ui_all)
            self.ui_lives.append(ui_lives)

    def _noop_score(self, x, y, img, moth, bullets, enemies, player, ui_all, ui_lives, ui_canon):
        """
        When the agent is taking a noop, the noop should be affected positively by the location of
        enemies, player, and bullets
        """
        mask = np.zeros_like(img)
        mask += enemies
        mask += player
        mask += moth
        mask = mask
        return np.sum(mask * img) / (np.sum(np.maximum(mask, img)) + 1) # +1 to prevent errors

    def _fire_score(self, x, y, img, moth, bullets, enemies, player, ui_all, ui_lives, ui_canon):
        """
        Agent decision to fire should be affectedly positivly by the location of enemies above the player
        """
        h, w = img.shape
        mask = np.zeros_like(img)
        _start = int(max(0, x - 10))
        _end   = int(min(w, x + 10))
        mask[:, _start:_end] = enemies[:, _start:_end] + moth[:, _start:_end]
        mask = mask
        return np.sum(mask * img) / (np.sum(np.maximum(mask, img)) + 1)


    def _right_score(self, x, y, img, moth, bullets, enemies, player, ui_all, ui_lives, ui_canon):
        """
        Agent decision to move right, should try to follow enemies, but avoid bullets
        """
        h, w = img.shape
        mask = np.zeros_like(img)
        x = int(x)
        mask[:, x:w] += enemies[:, x:w] + moth[:, x:w] - bullets[:, x:w]
        mask[:, 0:x] -= enemies[:, 0:x] + moth[:, 0:x] - bullets[:, 0:x]
        mask = mask
        return np.sum(mask * img) / (np.sum(np.maximum(mask, img)) + 1)

    def _left_score(self, x, y, img, moth, bullets, enemies, player, ui_all, ui_lives, ui_canon):
        """
        Agent decision to move right, should try to follow enemies, but avoid bullets
        """
        h, w = img.shape
        mask = np.zeros_like(img)
        x = int(x)
        mask[:, x:w] -= enemies[:, x:w] + moth[:, x:w] - bullets[:, x:w]
        mask[:, 0:x] += enemies[:, 0:x] + moth[:, 0:x] - bullets[:, 0:x]
        mask = mask
        return np.sum(mask * img) / (np.sum(np.maximum(mask, img)) + 1)



    def compute(self, values: Any) -> np.ndarray:
        N = values.shape[1]
        result = np.zeros(7)
        for i in range(N):
            img = values[0, i, :, :]
            x, y = weighted_centroid(self.player[i])
            result[0] += self._noop_score(x, y, img[:,:, 0], self.moth[i], self.bullets[i], self.enemies[i], self.player[i], self.ui_all[i], self.ui_lives[i], self.ui_cannon[i])
            result[1] += self._fire_score(x, y, img[:,:, 1], self.moth[i], self.bullets[i], self.enemies[i], self.player[i],
                                      self.ui_all[i], self.ui_lives[i], self.ui_cannon[i])
            result[2] += 0 # up / unhandled
            result[3] += self._right_score(x, y, img[:,:, 3], self.moth[i], self.bullets[i], self.enemies[i], self.player[i],
                                    self.ui_all[i], self.ui_lives[i], self.ui_cannon[i])
            result[4] += self._left_score(x, y, img[:,:, 4], self.moth[i], self.bullets[i], self.enemies[i], self.player[i],
                                    self.ui_all[i], self.ui_lives[i], self.ui_cannon[i])

            result[5] += 0 # fire-left / unhandled
            result[6] += 0 # fire-right / unhandled

        return result