"""A small headless driving sim on the engine's sense/action interface.

Cars follow a route of straights and bends in road (Frenet) coordinates
with a bicycle-model body, and meet a slower lead car, red lights and
kerbs. Fast enough to run hundreds of drivers at once for data
generation and PPO; the real world is the engine, so validate there.
"""
import numpy as np

from .features import IDX

WHEELBASE, DT = 2.7, 1.0 / 20.0
SEGMENTS, MAX_STEPS = 80, 600


class Sim:
    def __init__(self, n, seed=0):
        self.n, self.rng = n, np.random.default_rng(seed)
        z = lambda: np.zeros(n, np.float32)
        self.s, self.d, self.psi, self.v = z(), z(), z(), z()
        self.delta, self.steps = z(), np.zeros(n, np.int32)
        self.want, self.follow, self.nerve = z(), z(), z()
        self.gap, self.lead_v, self.lead_goal = z(), z(), z()
        self.light, self.clock = z(), z()
        self.bounds = np.zeros((n, SEGMENTS), np.float32)
        self.kappa = np.zeros((n, SEGMENTS), np.float32)
        self.turning = z()
        self.reset(np.ones(n, bool))

    def reset(self, mask):
        r, k = self.rng, int(mask.sum())
        if not k:
            return
        for a in (self.s, self.d, self.psi, self.delta, self.turning):
            a[mask] = 0
        self.v[mask] = r.uniform(0, 8, k)
        self.steps[mask] = 0
        self.want[mask] = r.uniform(6, 14, k)
        self.follow[mask] = r.uniform(3, 10, k)
        self.nerve[mask] = r.uniform(0.7, 1.6, k)
        length = r.uniform(30, 120, (k, SEGMENTS))
        bend = r.uniform(size=(k, SEGMENTS)) < 0.4
        radius = r.uniform(12, 60, (k, SEGMENTS)) * r.choice([-1, 1], (k, SEGMENTS))
        self.bounds[mask] = np.cumsum(length, axis=1)
        self.kappa[mask] = np.where(bend, 1.0 / radius, 0.0)
        self.gap[mask] = np.where(r.uniform(size=k) < 0.6,
                                  r.uniform(15, 80, k), 1e3)
        self.lead_v[mask] = r.uniform(0, 12, k)
        self.lead_goal[mask] = self.lead_v[mask]
        self.light[mask] = r.uniform(60, 300, k)
        self.clock[mask] = r.uniform(0, 20, k)

    def _kappa_here(self):
        idx = (self.s[:, None] >= self.bounds).sum(1).clip(0, SEGMENTS - 1)
        return self.kappa[np.arange(self.n), idx]

    def _red(self):
        return (self.clock % 20.0) < 9.0

    def sense(self):
        look = np.clip(self.v * 1.1, 7, 20)
        k = self._kappa_here()
        off = np.arctan2(k * look**2 / 2 - self.d, look) - self.psi
        off = (off + np.pi) % (2 * np.pi) - np.pi
        line = np.where(self._red(), self.light, 1e3)
        cols = [self.v, off, self.turning, self.gap, line, self.want,
                self.follow, self.nerve]
        return np.stack(cols, axis=1).astype(np.float32)

    def step(self, act):
        """act (N, 3) steer, throttle, brake -> reward, done, info."""
        r, dt = self.rng, DT
        steer, thr, brk = act[:, 0], act[:, 1], act[:, 2]
        lock = 0.8 / (1.0 + 0.03 * self.v)
        self.delta += np.clip(steer * lock - self.delta, -2 * dt, 2 * dt)
        acc = thr * 3.5 * (1 - self.v / 28.0) - brk * 9.0 - 0.05 * self.v
        self.v = np.maximum(0.0, self.v + acc * dt)
        self.turning = self.v * np.tan(self.delta) / WHEELBASE
        k = self._kappa_here()
        ds = self.v * np.cos(self.psi) / np.maximum(0.2, 1 - k * self.d) * dt
        self.d += self.v * np.sin(self.psi) * dt
        self.psi += (self.turning - k * ds / dt) * dt
        self.s += ds
        self._lead(ds, dt)
        self.clock += dt
        self.light -= ds
        passed = self.light < 0
        ran = passed & self._red()
        self.light = np.where(passed, r.uniform(120, 400, self.n), self.light)
        self.clock = np.where(passed, r.uniform(0, 20, self.n), self.clock)
        crash = self.gap < 0
        kerb = np.abs(self.d) > 2.0
        self.steps += 1
        done = crash | kerb | (self.steps >= MAX_STEPS) | \
            (self.s > self.bounds[:, -1] - 40)
        tail = np.maximum(0.0, self.follow - self.gap) / self.follow
        rew = (0.1 * ds + dt * (1 - np.abs(self.d) / 2)
               - dt * 0.2 * np.maximum(0.0, self.v - self.want)
               - dt * 2 * tail * (self.v > 0.5)
               - 10 * ran - 20 * crash - 5 * kerb)
        info = {"crash": crash, "kerb": kerb, "ran": ran}
        return rew.astype(np.float32), done, info

    def _lead(self, ds, dt):
        r = self.rng
        has = self.gap < 500
        pick = has & (r.uniform(size=self.n) < 0.01)
        goal = np.where(r.uniform(size=self.n) < 0.3, 0.0,
                        r.uniform(3, 12, self.n))
        self.lead_goal = np.where(pick, goal, self.lead_goal)
        self.lead_v += np.clip(self.lead_goal - self.lead_v, -3 * dt, 3 * dt)
        self.gap = np.where(has, self.gap + (self.lead_v * dt - ds), self.gap)
        gone = has & (self.gap > 150)
        born = ~has & (r.uniform(size=self.n) < 0.02)
        self.gap = np.where(gone, 1e3, np.where(born, r.uniform(20, 80, self.n),
                                                 self.gap))
        self.lead_v = np.where(born, r.uniform(0, 12, self.n), self.lead_v)
