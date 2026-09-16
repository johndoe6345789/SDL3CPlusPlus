# TODO — GTA V package

From a review of the engine's own recordings (2026-09-16): 11 frames
of a downtown walk, and the sound track's levels. Ordered by what a
player notices first. Each item says what was seen and what "done"
means.

Status: `[ ]` open, `[~]` in progress, `[x]` done.

## 1. Look

- [ ] **Normal and specular maps.** Tiles and concrete are flat paint.
  GTA ships both maps per shader.
- [ ] **Ambient occlusion.** No contact shadow where walls meet the
  ground.
- [ ] **Sky and weather.** One flat blue, no clouds. Use GTA's
  timecycle and weather data for sky colour, haze and clouds by hour.

## 2. Feel


## 3. Weapons and HUD

- [ ] **Minimap with health and armour.** A lone green bar today.

## 4. Life

- [ ] **City ambience.** Between footsteps the track is silent.
  audio_rel.rpf holds 960 ambient zones and 511 static emitters
  (fountains, AC units, bars); play them.
- [ ] **Pedestrians.** The plaza is empty. The ped loader already
  dresses the player; spawn ambient peds on GTA's paths.
- [ ] **Traffic density.** Only a few distant cars downtown.

## Done

- [x] **Colour pipeline.** No texture downtown is tagged sRGB (1,493
  looked at: BC1, BC3, a few BC5/BC2), so the formats were not the
  fault: the shaders lit GTA's display-encoded colour maps as linear,
  and the composite encoded them again. The model, terrain, emissive and
  effects shaders now decode them (`include/gta5_srgb.glsl`).
- [x] **Sun and shade.** The wrapped sun lit a face turned away at half
  strength; plain Lambert and a sky term that is weaker on faces turned
  down now separate them. Exposure roughly doubled (noon 0.42) to put
  concrete back near where it was, and water's own colour halved to
  match. The three shaders share their shadow, water and haze code
  from `include/` instead of three copies.
- [x] **Dirt and decals.** The black drips and blotches were ordinary
  decals (`decal`, `normal_spec_decal`, `decal_tnt`; no `decal_dirt`
  downtown) drawn at their texture's full alpha. GTA fades each by its
  vertex colour 0 alpha: across 8,088 decal meshes that alpha has a
  median of 147/255, and only 2,221 are fully opaque. Colour 0's alpha
  now rides in every vertex, and blended surfaces multiply it in.
- [x] **Stride, not speed.** The review blamed Quake's 10 m/s, but the
  package already jogs at 3.4 and sprints at 6. The fast footfalls were
  a fixed 1.4 m stride in the sound, and a run stride of 2.3 m in the
  animation (5.2 steps a second at a sprint). The run stride is 2.8 m,
  the footsteps follow the drawn walk cycle (0.2 s apart at a jog, now
  0.28), and Left Alt walks at 1.5 m/s.
- [x] **Camera.** It sits over the right shoulder (`shoulder`, 0.45 m),
  and pulls in with a 0.45 m sphere instead of a ray: a ray that
  slipped past a wall's edge let the near plane, 0.8 m to its corners,
  cut a third of the screen out of it.
- [x] **Weapons and HUD.** The game starts holstered (the pistol is
  still owned, a Q away); the reticle shows only while aiming; the
  weapon readout shows while aiming, choosing, or for 3 s after the
  weapon or its clip changes; and the clock is gone.
- [x] **Climb and vault.** Space facing a wall with a top 0.4-2 m up
  climbs onto it in about half a second (`gta5.player.climb`); anywhere
  else it is the jump. Tested on boxes (`gta5_climb_test`) and seen in a
  recording, up a 0.89 m planter in Legion Square.
