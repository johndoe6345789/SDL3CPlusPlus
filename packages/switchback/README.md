# Switchback

An off-road 4x4 game on the SDL3CPlusPlus engine. Climb mountain tracks, crawl over rock, and stay out of the hazards.
## Status

In development. Milestone 1 is one playable level with a driving prototype.

## Levels

- **Spiral Pass** (milestone 1). A switchback track with checkpoints that winds up a mountain. The spec is in `assets/spiral_pass.json`.
- **Rock Garden** (planned). Rock-crawling terrain with rockfalls, ruts and water crossings.

Planned levels, in no particular order:

- Arctic Wasteland
- Arizona
- Autumn Leaves
- Baja Beach
- Bayou Flats
- Black Gold
- Castle Rock
- Costa Rica Rally
- Deadman's Gulch
- Egypt
- Final Destination
- Mediterranean
- Obstacle Park
- Quarry Lake
- River Side
- Silverton Pass
- Tibet Cliffside
- Tri Baja 250
- Vulture Canyon

## Engine use

- Physics: Bullet 3.25, the raycast vehicle with suspension and wheel friction.
- Rendering: the engine's SDL GPU layer. It runs on Vulkan on Windows and Linux and on Metal on macOS. Effects (PBR materials, cascaded shadows, SSAO, dust and spray, sky) are shaders in that layer.
- Game logic: JSON workflows under `workflows/`, following the other packages in `packages/`.

## Assets

Two sources are used.

- **Free and libre assets** (CC0 from Poly Haven and ambientCG, or other permissive licences). The licence is recorded next to each asset, and anything that needs attribution is listed in this README.
- **GTA V assets, read at runtime.** Vehicle models, textures and code are borrowed from the `gta5` package. The player must own GTA V. The game reads the player's own extracted files from `GTA5_DATA_DIR`, as `packages/gta5` does. Nothing from the game is committed, converted or redistributed with Switchback. The map is not used; Switchback has its own map.

Rule: no GTA V file ever goes into this repository or into a build output. Only code and the path to the player's extract are shared.

## Track spec

`assets/spiral_pass.json` describes the track by parameters rather than by points. The game expands it into the road, checkpoints and terrain height at load time, so the spec stays small and easy to tune.
