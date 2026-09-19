# Neural traffic drivers (GTA5)

The engine's traffic AI (`gta5_traffic_drive.cpp`) is a pure function of eight
numbers, the *sense vector* (`gta5_traffic_ai.hpp`): speed, heading error to
the aim point, yaw rate, gap ahead, distance to a red line, and three
personality values (`want` speed, `follow` gap, `nerve` in bends). The
hand-written driver, the logger and the network all read exactly that, so a
trained net is a drop-in replacement.

```
engine baseline --SDL3CPP_TRAFFIC_LOG--> rows of 8 sense + 3 controls
        |                                        |
        |                              train_bc.py (imitation + DAgger)
        |                                        |
        |                            <out>.tnn / <out>.onnx
        |                                        |
        |                                   ppo.py (RL in sim.py)
        v                                        v
 SDL3CPP_TRAFFIC_POLICY=<out>.tnn  ->  Gta5DriverNet (hand-rolled MLP)
```

## Steps

1. **Log the baseline** (headless, so the window stays out of the way):

   ```
   SDL3CPP_HEADLESS=1 SDL3CPP_TRAFFIC_LOG=build/traffic/game.bin \
     sdl3_app.exe --bootstrap bootstrap_windows --game gta5
   ```

   Rows are raw little-endian float32, 11 per row. When a policy is also
   loaded the *baseline's* answer is what gets logged, so the net's own
   mistakes get labelled (DAgger on real data).

2. **Imitate**: `python -m python.traffic_ai.train_bc --out build/traffic/bc
   --log build/traffic/game.bin`. It prints the max error of the Python
   baseline port against the logged controls (must be ~0), mixes game rows
   with sim rollouts, runs DAgger rounds, and writes `bc.tnn` + `bc.onnx`.
   Uses CUDA/ROCm if PyTorch sees one; a 8-64-64-3 MLP trains fine on CPU.

3. **Fine-tune with RL**: `python -m python.traffic_ai.ppo --init
   build/traffic/bc.tnn --out build/traffic/rl`. PPO in `sim.py`, reward per
   the pitch (progress, lane, speed/following gap; -10 red light, -20 crash,
   -5 kerb). Personality values are inputs, so one net gives many drivers.

4. **Run it in the engine**: set `SDL3CPP_TRAFFIC_POLICY=build/traffic/rl.tnn`.
   Unset, the baseline drives. Inference is `Gta5DriverNet` (a ~50 line MLP
   forward pass), so there is no ONNX Runtime dependency; the `.onnx` file is
   for inspection and other runtimes.

## Caveats

* `sim.py` is a kinematic sim, not the engine's Bullet vehicle. RL gains
  there can shrink or vanish in-game; check by running the engine with the
  policy and comparing against the baseline.
* No pedestrians or lane-change inputs yet; add them to the sense struct,
  `features.py` and the trainer together.
* The `.tnn` layout is checked by `tests/gta5_traffic_net_test.cpp`.
