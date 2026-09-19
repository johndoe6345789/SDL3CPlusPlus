"""Imitation learning: teach a small net to copy the baseline driver.

    python -m python.traffic_ai.train_bc --out build/traffic
    python -m python.traffic_ai.train_bc --log traffic.bin --out ...

Writes <out>.tnn (the engine loads it via SDL3CPP_TRAFFIC_POLICY) and
<out>.onnx. Uses CUDA/ROCm if PyTorch sees a GPU, otherwise the CPU:
for a net this small the CPU is not the bottleneck.
"""
import argparse

import numpy as np
import torch

from .data import load_log, rollout
from .evaluate import evaluate
from .features import baseline, inputs
from .net import DriverNet, controls, save_onnx, save_tnn
from .sim import Sim

WEIGHTS = torch.tensor([3.0, 1.0, 1.0])  # steering matters most


def fit(net, sense, label, epochs, device, lr=2e-3):
    x = torch.from_numpy(inputs(sense)).to(device)
    y = torch.from_numpy(label).to(device)
    opt = torch.optim.Adam(net.parameters(), lr=lr)
    sched = torch.optim.lr_scheduler.CosineAnnealingLR(opt, epochs)
    w = WEIGHTS.to(device)
    for epoch in range(epochs):
        order = torch.randperm(len(x), device=device)
        for i in range(0, len(x), 4096):
            b = order[i:i + 4096]
            loss = (((controls(net(x[b])) - y[b]) ** 2) * w).mean()
            opt.zero_grad()
            loss.backward()
            opt.step()
        sched.step()
    return float(loss)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--out", required=True)
    p.add_argument("--log", nargs="*", default=[])
    p.add_argument("--cars", type=int, default=256)
    p.add_argument("--steps", type=int, default=600)
    p.add_argument("--dagger", type=int, default=4)
    p.add_argument("--epochs", type=int, default=30)
    a = p.parse_args()
    device = "cuda" if torch.cuda.is_available() else "cpu"
    print("training on", device)
    sim = Sim(a.cars, seed=1)
    parts = [rollout(sim, a.steps, noise=0.15), rollout(sim, a.steps)]
    for path in a.log:
        s, y = load_log(path)
        print(f"{path}: {len(s)} game rows; baseline port max error",
              float(np.abs(baseline(s) - y).max()))
        parts.append((s, y))
    net = DriverNet().to(device)
    for it in range(a.dagger + 1):
        sense = np.concatenate([s for s, _ in parts])
        label = np.concatenate([y for _, y in parts])
        loss = fit(net, sense, label, a.epochs, device)
        net.cpu()
        print(f"round {it}: {len(sense)} rows, loss {loss:.5f}",
              evaluate(net, cars=128, steps=1000))
        if it < a.dagger:
            parts.append(rollout(sim, a.steps, net=net))
            net.to(device)
    print("baseline:", evaluate(None, cars=128, steps=1000))
    save_tnn(net, a.out + ".tnn")
    save_onnx(net, a.out + ".onnx")
    print("wrote", a.out + ".tnn", a.out + ".onnx")


if __name__ == "__main__":
    main()
