# Switchback

An off-road 4x4 game on the SDL3CPlusPlus engine. Climb mountain tracks, crawl over rock, and stay out of the hazards. Written from scratch; no code or data from any existing 4x4 game is used.

## Status

In development. Milestone 1 is one playable level with a driving prototype.

## Levels

- **Spiral Pass** (milestone 1). A switchback track with checkpoints that winds up a mountain. The spec is in `assets/spiral_pass.json`.
- **Rock Garden** (planned). Rock-crawling terrain with rockfalls, ruts and water crossings.

## Engine use

- Physics: Bullet 3.25, the raycast vehicle with suspension and wheel friction.
- Rendering: the engine's SDL GPU layer. It runs on Vulkan on Windows and Linux and on Metal on macOS. Effects (PBR materials, cascaded shadows, SSAO, dust and spray, sky) are shaders in that layer.
- Game logic: JSON workflows under `workflows/`, following the other packages in `packages/`.

## Assets

Only free, libre assets are used, with the licence recorded next to each asset. Accepted sources include CC0 (Poly Haven, ambientCG) and other permissive licences. Anything that needs attribution is listed in this README when it is added.

## Track spec

`assets/spiral_pass.json` describes the track by parameters rather than by points. The game expands it into the road, checkpoints and terrain height at load time, so the spec stays small and easy to tune.
