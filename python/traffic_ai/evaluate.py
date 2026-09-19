"""How a driver does in the sim: crashes, kerbs, red lights, progress."""
import numpy as np

from .data import net_actions
from .features import baseline
from .sim import Sim


def evaluate(net=None, cars=256, steps=3000, seed=123):
    sim, total, ev = Sim(cars, seed), 0.0, {"crash": 0, "kerb": 0, "ran": 0}
    metres = 0.0
    for _ in range(steps):
        sense = sim.sense()
        act = baseline(sense) if net is None else net_actions(net, sense)
        before = sim.s.copy()
        rew, done, info = sim.step(act)
        total += float(rew.mean())
        metres += float(np.maximum(sim.s - before, 0).sum())
        for k in ev:
            ev[k] += int(info[k].sum())
        sim.reset(done)
    per_km = {k: round(1000 * v / max(metres, 1), 3) for k, v in ev.items()}
    return {"reward_per_step": round(total / steps, 4),
            "km": round(metres / 1000, 1), "per_km": per_km}
