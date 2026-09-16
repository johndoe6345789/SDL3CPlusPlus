# TODO — GTA V package

From a review of the engine's own recordings (2026-09-16): 11 frames
of a downtown walk, and the sound track's levels. Ordered by what a
player notices first. Each item says what was seen and what "done"
means.

Status: `[ ]` open, `[~]` in progress, `[x]` done.

## 1. Look

- [ ] **Colour pipeline.** Sunlit pavement is near white and the whole
  frame is flat. GTA's sRGB textures are uploaded as UNORM, the model
  shader never linearises them, and the composite applies 1/2.2 gamma
  on top. Done: sRGB formats upload as sRGB, one gamma at the end.
- [ ] **Exposure.** Once colour is right, set exposure so noon
  concrete sits near mid-grey rather than clipping.
- [ ] **Dirt and decal shaders.** Walls carry hard black drips, the
  paving dark blotches: GTA's dirt layers drawn as opaque paint. Done:
  they read as the faint grime they are in GTA.
- [ ] **Normal and specular maps.** Tiles and concrete are flat paint.
  GTA ships both maps per shader.
- [ ] **Ambient occlusion.** No contact shadow where walls meet the
  ground.
- [ ] **Sky and weather.** One flat blue, no clouds. Use GTA's
  timecycle and weather data for sky colour, haze and clouds by hour.

## 2. Feel

- [ ] **GTA movement speeds.** Footfalls every 0.2 s: the Quake 3 run
  (~10 m/s). GTA walks ~1.5 m/s, jogs ~3, sprints ~7. Done: W jogs,
  Shift sprints, a key walks, with GTA-like acceleration; the Quake
  collision stays.
- [ ] **Climb and vault.** A knee-high planter stops the player dead.
- [ ] **Camera collision.** The camera goes into walls; GTA's pulls in
  towards the player.
- [ ] **Over-the-shoulder camera.** The character sits dead centre
  under the reticle.

## 3. Weapons and HUD

- [ ] **Start unarmed.** The pistol is out while strolling.
- [ ] **Reticle only when aiming.** It sits on the character's head
  all the time.
- [ ] **Weapon readout only when it matters.** `PISTOL 6 190` never
  leaves the screen; GTA shows it when aiming, firing or choosing.
- [ ] **Drop the debug clock.** GTA has no clock on screen.
- [ ] **Minimap with health and armour.** A lone green bar today.

## 4. Life

- [ ] **City ambience.** Between footsteps the track is silent.
  audio_rel.rpf holds 960 ambient zones and 511 static emitters
  (fountains, AC units, bars); play them.
- [ ] **Pedestrians.** The plaza is empty. The ped loader already
  dresses the player; spawn ambient peds on GTA's paths.
- [ ] **Traffic density.** Only a few distant cars downtown.

## Done

(Moved here with the commit that did it.)
