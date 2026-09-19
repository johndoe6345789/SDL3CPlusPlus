"""RL fine-tuning: PPO on top of the imitation net, in the sim.

    python -m python.traffic_ai.ppo --init build/traffic/bc.tnn \
        --out build/traffic/rl --iters 300

Reward (see sim.py): progress and staying in lane, a little for holding
the personality's speed and following gap; -10 red light, -20 crash,
-5 kerb. Personality values are network inputs, so one net drives them
all. Keeps the best net by evaluation reward and exports it.
"""
import argparse

import numpy as np
import torch
from torch import nn

from .evaluate import evaluate
from .features import inputs
from .net import DriverNet, controls, load_tnn, save_onnx, save_tnn
from .sim import Sim


def critic():
    return nn.Sequential(nn.Linear(8, 64), nn.Tanh(),
                         nn.Linear(64, 64), nn.Tanh(), nn.Linear(64, 1))


def collect(sim, net, log_std, value, horizon):
    obs, pre, logp, val, rew, done = [], [], [], [], [], []
    for _ in range(horizon):
        x = torch.from_numpy(inputs(sim.sense()))
        with torch.no_grad():
            mean = net(x)
            z = mean + log_std.exp() * torch.randn_like(mean)
            lp = torch.distributions.Normal(mean, log_std.exp()) \
                .log_prob(z).sum(-1)
            v = value(x).squeeze(-1)
        r, d, _ = sim.step(controls(z).numpy())
        obs.append(x); pre.append(z); logp.append(lp); val.append(v)
        rew.append(torch.from_numpy(r)); done.append(torch.from_numpy(d))
        sim.reset(d)
    with torch.no_grad():
        last = value(torch.from_numpy(inputs(sim.sense()))).squeeze(-1)
    return [torch.stack(t) for t in (obs, pre, logp, val, rew, done)], last


def advantages(val, rew, done, last, gamma=0.99, lam=0.95):
    adv, gae, nxt = torch.zeros_like(rew), 0.0, last
    for t in reversed(range(len(rew))):
        live = 1.0 - done[t].float()
        delta = rew[t] + gamma * nxt * live - val[t]
        gae = delta + gamma * lam * live * gae
        adv[t], nxt = gae, val[t]
    return adv, adv + val


def update(net, log_std, value, opt, batch, clip=0.2, epochs=4):
    obs, pre, old, adv, ret = batch
    adv = (adv - adv.mean()) / (adv.std() + 1e-6)
    for _ in range(epochs):
        for b in torch.randperm(len(obs)).split(4096):
            mean = net(obs[b])
            dist = torch.distributions.Normal(mean, log_std.exp())
            ratio = (dist.log_prob(pre[b]).sum(-1) - old[b]).exp()
            pol = -torch.min(ratio * adv[b],
                             ratio.clamp(1 - clip, 1 + clip) * adv[b]).mean()
            loss = pol + 0.5 * ((value(obs[b]).squeeze(-1) - ret[b]) ** 2) \
                .mean() - 0.001 * dist.entropy().sum(-1).mean()
            opt.zero_grad()
            loss.backward()
            nn.utils.clip_grad_norm_(list(net.parameters())
                                     + list(value.parameters()), 1.0)
            opt.step()


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--init", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--iters", type=int, default=300)
    p.add_argument("--cars", type=int, default=256)
    p.add_argument("--horizon", type=int, default=64)
    p.add_argument("--warmup", type=int, default=5)
    a = p.parse_args()
    net, value, sim = load_tnn(a.init), critic(), Sim(a.cars, seed=7)
    log_std = nn.Parameter(torch.full((3,), -1.5))
    opt = torch.optim.Adam([{"params": net.parameters(), "lr": 1e-4},
                            {"params": [log_std], "lr": 1e-3},
                            {"params": value.parameters(), "lr": 1e-3}])
    best = evaluate(net, cars=128, steps=1000)
    print("start", best)
    best_r = best["reward_per_step"]
    for it in range(a.iters):
        (obs, pre, logp, val, rew, done), last = collect(
            sim, net, log_std, value, a.horizon)
        adv, ret = advantages(val, rew, done, last)
        if it < a.warmup:  # let the critic catch up before moving the policy
            for g in opt.param_groups[:2]:
                g["lr"] = 0.0
        elif it == a.warmup:
            opt.param_groups[0]["lr"], opt.param_groups[1]["lr"] = 1e-4, 1e-3
        flat = lambda t: t.reshape(-1, *t.shape[2:])
        update(net, log_std, value, opt,
               (flat(obs), flat(pre), flat(logp), flat(adv), flat(ret)))
        if (it + 1) % 20 == 0:
            res = evaluate(net, cars=128, steps=1000)
            print(f"iter {it + 1} std {log_std.exp().mean():.3f}", res)
            if res["reward_per_step"] > best_r:
                best_r = res["reward_per_step"]
                save_tnn(net, a.out + ".tnn")
                save_onnx(net, a.out + ".onnx")
    print("best eval reward/step", best_r)


if __name__ == "__main__":
    main()
