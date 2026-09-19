"""The driver network, and its export to the engine's TNN1 file and ONNX."""
import struct

import numpy as np
import torch
from torch import nn

from .features import N_IN, N_OUT

MAGIC = 0x314E4E54  # "TNN1", read little-endian by the engine


class DriverNet(nn.Module):
    """ReLU MLP. Outputs are pre-activation: tanh steer, sigmoid the
    other two, applied by `controls` (and by the engine)."""

    def __init__(self, hidden=(64, 64)):
        super().__init__()
        sizes = [N_IN, *hidden, N_OUT]
        self.layers = nn.ModuleList(
            nn.Linear(a, b) for a, b in zip(sizes[:-1], sizes[1:]))

    def forward(self, x):
        for layer in self.layers[:-1]:
            x = torch.relu(layer(x))
        return self.layers[-1](x)


def controls(pre):
    """Pre-activation outputs -> (steer, throttle, brake)."""
    return torch.cat([torch.tanh(pre[..., :1]),
                      torch.sigmoid(pre[..., 1:])], dim=-1)


def save_tnn(net, path):
    """Write the weights as the engine's Gta5DriverNet::Load reads them."""
    with open(path, "wb") as f:
        f.write(struct.pack("<II", MAGIC, len(net.layers)))
        for layer in net.layers:
            out, inn = layer.weight.shape
            f.write(struct.pack("<ii", inn, out))
            f.write(layer.weight.detach().cpu().numpy()
                    .astype("<f4").tobytes())
            f.write(layer.bias.detach().cpu().numpy()
                    .astype("<f4").tobytes())


class _Exported(nn.Module):
    def __init__(self, net):
        super().__init__()
        self.net = net

    def forward(self, x):
        return controls(self.net(x))


def save_onnx(net, path):
    net = net.cpu().eval()
    torch.onnx.export(_Exported(net), torch.zeros(1, N_IN), path,
                      input_names=["sense"], output_names=["controls"],
                      dynamic_axes={"sense": {0: "batch"}},
                      opset_version=17, dynamo=False)


def load_tnn(path):
    """Read a TNN1 file back into a DriverNet (for RL warm starts)."""
    raw = open(path, "rb").read()
    magic, count = struct.unpack_from("<II", raw, 0)
    assert magic == MAGIC
    at, shapes, tensors = 8, [], []
    for _ in range(count):
        inn, out = struct.unpack_from("<ii", raw, at)
        at += 8
        w = np.frombuffer(raw, "<f4", inn * out, at).reshape(out, inn)
        at += 4 * inn * out
        b = np.frombuffer(raw, "<f4", out, at)
        at += 4 * out
        shapes.append(out)
        tensors.append((w.copy(), b.copy()))
    net = DriverNet(tuple(shapes[:-1]))
    for layer, (w, b) in zip(net.layers, tensors):
        layer.weight.data = torch.from_numpy(w)
        layer.bias.data = torch.from_numpy(b)
    return net
