"""The sense vector, its scaling, and the hand-written baseline driver.

Mirrors Gta5DriverInputs and Gta5BaselineDriver in
gta5_traffic_baseline.cpp line for line.
"""
import numpy as np

SENSE = ["speed", "off", "turning", "gap", "line", "want", "follow",
         "nerve"]
N_IN, N_OUT = len(SENSE), 3
IDX = {name: i for i, name in enumerate(SENSE)}


def inputs(sense):
    """Raw sense rows (N, 8) -> the scaled rows the network reads."""
    s = np.asarray(sense, dtype=np.float32)
    out = np.empty_like(s)
    out[:, 0] = s[:, 0] / 20.0
    out[:, 1] = s[:, 1] / np.pi
    out[:, 2] = s[:, 2] / 2.0
    out[:, 3] = np.minimum(s[:, 3], 40.0) / 40.0
    out[:, 4] = np.minimum(s[:, 4], 40.0) / 40.0
    out[:, 5] = s[:, 5] / 20.0
    out[:, 6] = s[:, 6] / 10.0
    out[:, 7] = s[:, 7]
    return out


def baseline(sense):
    """Raw sense rows (N, 8) -> (steer, throttle, brake) rows (N, 3)."""
    s = np.asarray(sense, dtype=np.float32)
    speed, off, turning, gap, line, want, follow, nerve = s.T
    want = np.where(gap < follow + 6.0,
                    np.minimum(want, np.maximum(0.0, gap - follow) * 1.6),
                    want)
    want = np.where(line < 22.0,
                    np.minimum(want, np.maximum(0.0, line - 7.0) * 1.1),
                    want)
    want = want * np.maximum(0.4, 1.0 - np.abs(off) * 0.6 / nerve)
    gain = 1.6 / (1.0 + speed * 0.12)
    steer = np.clip(off * gain - turning * 0.25, -1.0, 1.0)
    pedal = np.clip((want - speed) * 0.5, -1.0, 1.0)
    brake = np.where((want < 0.5) & (speed > 0.6), 0.8,
                     np.where(pedal < -0.4, 0.35, 0.0))
    return np.stack([steer, np.maximum(0.0, pedal), brake],
                    axis=1).astype(np.float32)
