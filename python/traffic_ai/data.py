"""Rollouts and logs -> (sense, baseline action) training rows."""
import numpy as np
import torch

from .features import N_IN, N_OUT, baseline, inputs
from .net import controls


def load_log(path):
    """Rows the engine appended under SDL3CPP_TRAFFIC_LOG."""
    rows = np.fromfile(path, dtype="<f4").reshape(-1, N_IN + N_OUT)
    return rows[:, :N_IN], rows[:, N_IN:]


def net_actions(net, sense):
    with torch.no_grad():
        x = torch.from_numpy(inputs(sense))
        return controls(net(x)).numpy()


def rollout(sim, steps, net=None, noise=0.0):
    """Drive `sim` and label every state with the baseline.

    With a net it drives (DAgger: its own mistakes get labelled);
    otherwise the baseline drives, with `noise` on the controls so the
    data covers more than the baseline's own tidy line.
    """
    rng, senses, labels = np.random.default_rng(), [], []
    for _ in range(steps):
        sense = sim.sense()
        label = baseline(sense)
        act = label if net is None else net_actions(net, sense)
        if noise:
            act = act + rng.normal(0, noise, act.shape).astype(np.float32)
            act[:, 0] = np.clip(act[:, 0], -1, 1)
            act[:, 1:] = np.clip(act[:, 1:], 0, 1)
        senses.append(sense)
        labels.append(label)
        _, done, _ = sim.step(act)
        sim.reset(done)
    return np.concatenate(senses), np.concatenate(labels)
