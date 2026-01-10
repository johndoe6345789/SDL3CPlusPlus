# ROADMAP

See docs/PROMPT.md

## North Star
Treat JSON config as a declarative control plane that compiles into scene, resource, and render graphs with strict validation, budget enforcement, and crash-resistant policies.

## Guiding Principles
- Fail fast with clear JSON-path error reporting.
- Keep APIs explicit, predictable, and easy to reason about.
- Prefer refactoring that reduces boilerplate and hardcoded state.
- Treat JSON workflows as the primary control plane, keeping each definition micro‑stepped (templates/refs preferred over huge monoliths) and wiring only the plugin/service paths required by the active workflow.
- Keep source files focused (ideally <100 LOC unless justified) with a single responsibility, leverage DI/plugin/service/controller systems, and prefer loops and declarative packages over repeated boilerplate.
- Push static data (lists, hash maps) into JSON configs and template packages, then loop over those data sets so the C++ service code stays lean and declarative.

## Status Legend
- [x] live now
- [~] partial / limited
- [ ] planned

## Current Snapshot (Codebase Audit)
- Config intake: version gating + schema validation + layered merges (`extends`, `@delete`) with JSON Pointer diagnostics.
- Config compiler builds Scene/Resource/RenderGraph IR, resolves asset/material/render-pass refs, and schedules a render pass DAG; render graph pass order now configures view clears/touches but attachments and draw submission are still pending.
- Schema now covers assets/materials/shaders, `shader_systems`, and render-pass `view_id` + `clear` metadata.
- Runtime rendering is still Lua-driven, with MaterialX shader generation, pipeline validation, sampler caps, and texture/GUI cache budget enforcement.
- Diagnostics include ProbeService reports plus CrashRecoveryService heartbeats/GPU hang detection; runtime probe hooks (draw/present/frame) are still missing.
- Package workflows still include stub placeholders in `packages/*/workflows/stubs`, so `scripts/package_lint.py` currently fails.

## Launch Packages (Cheesy Edition)

### Starter Plan: "Bootstrap Hosting"
- [x] Config version gating (`schema_version` / `configVersion` checks)
- [x] JSON Schema validation (external schema + validator)
- [~] JSON-path diagnostics (schema validator pointers + compiler-generated paths; not full JSON Pointer coverage)
- [~] Layered config merges (supports `extends` + `@delete`; no profile/local/CLI yet)
- [x] Trace logging around config load, validation, and merge steps
- [~] Migration hooks for future versions (notes + hook implementation)

### Pro Plan: "Graph Builder"
- [~] Typed IRs: `SceneIR`, `ResourceIR`, `RenderGraphIR` (compiled; not yet consumed by runtime)
- [~] Symbol tables + reference resolution with clear diagnostics (assets/materials/render passes only)
- [x] Render graph DAG compile with cycle detection
- [x] "Use before produce" validation for render pass inputs
- [~] Explicit pass scheduling and backend submission planning (schedule only; no backend plan)

### Ultra Plan: "Probe Fortress"
- [~] Probe hooks (config/render graph/graphics reports wired; `OnDraw`/`OnPresent`/`OnFrameEnd`/`OnLoadScene` emit trace-gated runtime probes)
- [x] Pipeline compatibility checks (mesh layout vs shader inputs) via shader pipeline validator
- [x] Sampler limits enforced from bgfx caps
- [ ] Shader uniform compatibility enforcement
- [~] Resource budget enforcement (texture memory + max texture dim + GUI caches; no buffer budgets)
- [x] Crash recovery service (heartbeats, GPU hang detection, memory monitoring)
- [ ] Probe severity mapped to crash recovery policies

### Enterprise Plan: "Demo Deluxe"
- [ ] Service refactors to reduce boilerplate and hardcoded state
- [ ] JSON-driven component tree generation (entities, materials, passes)
- [ ] Cube demo rebuilt on config-first scene + render pipeline
- [ ] Hot-reload with diff logging and rollback on validation failure
- [ ] Configurable feature flags to isolate subsystems quickly

## Near-Term Focus
- Wire config compiler IR into resource loading + scene setup (reduce Lua-only paths).
- Execute render graph schedule in the renderer (attachments, lifetimes, view ordering).
- Add runtime probe hooks and map probe severity to crash recovery policies.
- Enforce shader uniform compatibility using reflection + material metadata.
- Validate soundboard workflow steps (catalog/GUI/audio) in runtime and retire the Lua soundboard UI path.
- Port Lua demos (seed/gui/soundboard/quake3) into JSON-first packages and workflows.
- Replace all workflow stubs in `packages/*/workflows/stubs` with fully implemented n8n JSON graphs derived from the existing Lua scripts + JSON configs (no placeholders).
- Add tests for schema/merge rules, render graph validation, and budget enforcement.
- Start service refactor program for large modules (approaching 2K LOC).
- Remove the Lua-first execution path once config-first workflows (boot, frame, gameplay) can fully describe the engine; workflows should be able to teleport the camera to key spots, render expected visuals, and fail fast when actual output deviates from the JSON-specified state (no Lua required unless explicitly chosen).

## Config-First Program Plan (Verbose)

### Decisions Locked In
- Config-first is the default runtime path. Lua becomes optional or secondary.
- Users can persist a default runtime config (via `--set-default-json`).
- Schema extensions are allowed.
- The default config pipeline (`--json-file-in` → `--set-default-json`/stored default → seed) is the preferred boot path, and Lua implications must only run when a workflow explicitly routes to it.
- Shader systems should be pluggable (MaterialX now, others later).

### Scope And Assumptions
- Scope: move config-first IR into runtime execution, add render graph execution, add runtime probes, and close shader uniform compatibility.
- Assume Lua scene/scripts remain as an explicit opt-in fallback while the IR path is built.
- Assume schema changes remain within `runtime_config_v2.schema.json` (no v3 bump yet).

### Phase 0: Baseline And Config-First Default (1-2 days)
- Ensure JSON config always compiles into IR before Lua services run.
- Confirm default config precedence: `--json-file-in` → `--set-default-json` path → stored default config → seed config.
- Introduce a runtime switch to enable Lua-only scene loading; default is config-first.
- Deliverable: app boot always compiles config and prefers IR-derived data.
- Acceptance: running with only a JSON config triggers IR compilation, and Lua scene load only happens if explicitly enabled.

- Introduce an npm-like dependency manifest per package and rely on the `packages/` directory (resolved through `scripts/package_lint.py`) so packages can declare dependencies (e.g., `materialx`) before their workflows run; this keeps each workflow self-contained and enforces the “only register what the workflow references” rule.
- Current catalog entries: `packages/seed` (spinning cube), `packages/gui` (GUI panels), `packages/soundboard`, `packages/quake3`, `packages/engine_tester`, and `packages/materialx`; each package lists its dependency graph and workflow entry point so a loader can resolve them in dependency order.

### Phase 1: Schema Extensions For Config-First Runtime (2-4 days)
- Extend schema to fully cover `assets`, `materials`, and `render.passes` (inputs/outputs, pass types).
- Add schema for render pass clear state, attachment format, and view metadata.
- Add a `shader_systems` section and allow per-shader system selection.
- Status: assets/materials/shaders + `shader_systems` + render-pass `view_id`/`clear` metadata are now in schema.
- Deliverable: schema guarantees all data needed for IR compilation and render execution.
- Acceptance: invalid configs fail with JSON Pointer diagnostics from schema validation.

### Phase 2: Pluggable Shader System Architecture (3-6 days)
- Define an `IShaderSystem` interface with explicit methods: `GetId()`, `BuildShader`, `GetReflection`, `GetDefaultTextures`.
- Add a shader system registry for discovery and selection.
- Implement `MaterialXShaderSystem` using existing MaterialX generator logic.
- Update shader loading to use the selected shader system to build `ShaderPaths`.
- Deliverable: shader generation/compilation becomes a plugin choice, not hardcoded.
- Status: `IShaderSystem` + registry wired into shader loading, with `materialx` and `glsl` systems registered; config compiler validates shader system declarations; registry exposes reflection + default textures (reflection uses shader texture bindings where available).
- Acceptance: MaterialX stays working, and a second system (e.g., `glsl`) can be registered without touching `IGraphicsService`.

### Phase 3: Resource IR → Runtime Resource Registry (3-6 days)
- Create a resource registry service to own `TextureIR`, `MeshIR`, `ShaderIR`, `MaterialIR` lifecycles.
- Integrate registry with `IShaderSystemRegistry` for shader assets.
- Add probe reports for missing resources and unresolved references (JSON paths included).
- Deliverable: resources can be created without Lua script involvement.
- Acceptance: a config with `assets` + `materials` loads textures/shaders and provides handles to rendering.

### Phase 4: Scene IR → Runtime Scene Setup (4-7 days)
- Expand `SceneIR` to include minimal component payloads (Transform + Renderable).
- Add schema for component payloads in scene entities.
- Implement scene builder service to map IR into `IEcsService`.
- Keep Lua scene path as an explicit fallback.
- Deliverable: scene is constructed from JSON without Lua.
- Acceptance: a single-mesh scene renders from config only.

### Phase 5: Render Graph Execution (5-9 days)
- Extend `RenderPassIR` to include clear flags, view IDs, and attachment definitions.
- Implement a render-graph executor that consumes `RenderGraphBuildResult::passOrder`.
- Map pass outputs to framebuffers and attachments, with swapchain as a valid output.
- Track attachment lifetimes and simple transient usage in the executor.
- Status: render graph pass order now drives view configuration (clear + touch); attachments/framebuffers still pending.
- Deliverable: render graph scheduling is executed, not just computed.
- Acceptance: a two-pass graph (offscreen + swapchain) renders correctly.

### Phase 6: Runtime Probe Hooks And Recovery Policy (3-6 days)
- Add runtime probe hooks (`OnDraw`, `OnPresent`, `OnFrameEnd`, `OnLoadScene`) in render coordinator + graphics backend/scene service.
- Map probe severity to crash recovery policies.
- Add probes for invalid handles and pass output misuse.
- Deliverable: runtime diagnostics that are structured and actionable.
- Acceptance: injected faults generate probe reports and prevent crashes.

### Phase 7: Shader Uniform Compatibility Enforcement (3-5 days)
- Choose uniform reflection sources per shader system (MaterialX vs bgfx shader binary).
- Validate material uniform mappings at config compile time.
- Emit JSON-path diagnostics for mismatches.
- Deliverable: uniform mismatches fail fast before rendering.
- Acceptance: invalid uniform mappings fail validation with clear JSON-path errors.

### Phase 8: Tests And Docs (2-5 days, overlaps phases)
- Add unit tests for config merge rules (`extends`, `@delete`).
- Add render graph validation tests for cycles and invalid outputs.
- Add shader system registry tests for multi-system support. (done)
- Update docs with a "Config First Pipeline" guide and known limitations.
- Deliverable: regression protection for the new pipeline.
- Acceptance: new tests pass alongside existing integration tests.

## Service Refactor Program (2K LOC Risk Plan)
### Goals
- Reduce single-file service sizes to improve readability, reviewability, and test coverage.
- Isolate responsibilities: parsing vs validation vs runtime state vs external I/O.
- Make failure modes explicit and easier to diagnose with trace probes.
- Align runtime services with JSON workflows so only referenced services run; keep each service file focused (<100 LOC ideally) and gate them through DI/plugin choreography rather than global singletons.

### Target Services (Top Of List)
- JsonConfigService (~1800 LOC): split into loader/merger/validator/parser modules.
- ScriptEngineService (~1650 LOC): split Lua binding registry, library setup, and script loading.
- BgfxGraphicsBackend (~1400 LOC): split pipeline/buffer/texture/screenshot/validation submodules.
- BgfxGuiService (~1100 LOC): split font cache, SVG cache, command encoding, and layout.
- MaterialXShaderGenerator (~1100 LOC): split MaterialX graph prep, shader emit, validation.

### Phase A: Mechanical Extraction (1-3 days)
- [~] JsonConfigService: extracted config document load/merge helpers into `src/services/impl/config/json_config_document_loader.cpp` plus small parser/extend/merge services to keep files <100 LOC.
- [~] JsonConfigService: split schema validation/versioning/migration into `json_config_*` services (`schema_validator`, `version_validator`, `migration_service`).
- [~] ScriptEngineService: extracted MaterialX helpers into micro services (`materialx_path_resolver`, `materialx_search_path_builder`, `materialx_document_loader`, `materialx_surface_node_resolver`, `materialx_surface_parameter_reader`).
- [x] Reorganized `src/services/impl` into domain subfolders (app/audio/config/diagnostics/graphics/gui/input/materialx/platform/render/scene/script/shader/soundboard/workflow) to align with workflow boundaries and DI.
- Move self-contained helpers into `*_helpers.cpp` with clear headers.
- Extract pure data transforms into free functions with unit tests.
- Preserve public interfaces; no behavior change in this phase.

### Phase B: Responsibility Split (2-5 days)
- Create focused classes (e.g., `ConfigSchemaValidator`, `ConfigMergeService`,
  `LuaBindingRegistry`, `BgfxPipelineCache`, `TextureLoader`, `GuiFontCache`).
- Reduce cross-module knowledge by passing simple data structs.
- Add trace logging at handoff boundaries to retain diagnostics.

### Phase C: API Stabilization (2-4 days)
- Tighten constructor injection to only needed dependencies.
- Remove circular dependencies; make order-of-operations explicit.
- Add targeted unit tests for each new helper/service.

### Acceptance Criteria
- Each refactored service has < 800 LOC in its primary implementation file.
- 1–3 unit tests per extracted module (minimum happy + failure path).
- No regression in existing integration tests or runtime logs.
- Services only run when the active workflow references them, avoiding unused initialization and respecting the "if it's not in the workflow, don't run it" rule.

## Validation Tour (Production Self-Test)
### Multi-Method Screen Validation
- Image compare (baseline diff with tolerance + max diff pixels).
- Non-black ratio checks (detect all-black or missing render output).
- Luma range checks (detect over/under-exposed frames).
- Mean color checks (verify dominant color scenes without exact baselines).
- Sample point checks (pinpoint color at specific normalized coordinates).

### Engine Tester Config
- `config/engine_tester_runtime.json` provides a default self-test config.
- Designed for production binaries; no golden image required by default.
- Produces capture artifacts in `artifacts/validation/`.
- The config-first workflow teleports the camera through key waypoints, renders the prescribed scenes, and validates each capture with the multi-method assertions; mismatched output fails with a descriptive JSON-path error so bad code can't silently crash the machine.
- Multiple validation strategies (image diff, luma, sample points, non-black ratio, mean color) are orchestrated from the workflow so even unknown scenes (new JSON/Lua outputs) get covered by at least one detector.

### Default Config Behavior (Config-First)
- Default config resolution remains `--json-file-in` → `--set-default-json` path → stored default config → seed config.
- Config-first is the default runtime path after the config is loaded.
- Lua scene/scripts execute only when explicitly enabled in config.

### Shader System Schema Options (For Future Selection)
Option A: global default + per-shader override
```json
"shader_systems": {
  "active": "materialx",
  "systems": {
    "materialx": { "enabled": true, "libraryPath": "...", "materialName": "..." },
    "glsl": { "enabled": false }
  }
},
"assets": {
  "shaders": {
    "pbr": { "vs": "shaders/pbr.vs", "fs": "shaders/pbr.fs", "system": "glsl" },
    "mx": { "system": "materialx", "material": "MyMaterial" }
  }
}
```

Option B: per-shader only
```json
"assets": {
  "shaders": {
    "mx": { "system": "materialx", "material": "MyMaterial" },
    "glsl_pbr": { "system": "glsl", "vs": "...", "fs": "..." }
  }
}
```

## Workflow Engine (n8n-Style Micro Steps)
### Goals
- Describe boot + frame pipelines as a declarative JSON workflow graph.
- Keep each step tiny (<100 LOC), with explicit inputs/outputs and DI-backed plugin lookup.
- Package common pipelines as templates so users don't start from scratch.
- Only register/instantiate step plugins that are referenced by the active workflow.
- Treat workflows as the portable definition of the entire engine (boot → gameplay → frame); each JSON step should map to a single C++ plugin/service that stays lean, and users should be able to describe camera modes, bullet physics, and render expectations just by swapping workflow nodes.
- Allow workflows to reference other files or templates when they grow large so each JSON artifact stays focused on one responsibility.

### Status
- [x] Workflow core: step registry + executor + JSON definition parser (n8n nodes + legacy steps).
- [x] Default step package: `config.load`, `config.version.validate`, `config.migrate`, `config.schema.validate`, `runtime.config.build`.
- [x] Boot config workflow execution (load/version/migrate/schema + runtime config build).
- [x] Workflow schema: `config/schema/workflow_v1.schema.json` supports n8n nodes + connections.
- [x] Templates: `config/workflows/templates/boot_default.json`, `config/workflows/templates/frame_default.json`.
- [~] Package workflows converted to n8n nodes (`seed`, `gui`, `soundboard`, `quake3`, `engine_tester`); stub placeholders still present and must be replaced.
- [x] Workflow steps implemented: `frame.bullet_physics`, `frame.camera`, `validation.tour.checkpoint`, `soundboard.catalog.scan`, `soundboard.gui`, `soundboard.audio`.
- [x] Render coordinator supports workflow-supplied GUI command overrides (bypass Lua GUI path).
- [x] Generic step library implemented (core data ops + camera/audio/scene/model coverage).

### Generic Step Catalog (Implemented)
- Value: `value.copy`, `value.default`, `value.literal`, `value.clear`, `value.assert.exists`, `value.assert.type`.
- Number: `number.add`, `number.sub`, `number.mul`, `number.div`, `number.min`, `number.max`, `number.clamp`, `number.abs`, `number.round`.
- Compare: `compare.eq`, `compare.ne`, `compare.gt`, `compare.gte`, `compare.lt`, `compare.lte`.
- Bool: `bool.and`, `bool.or`, `bool.not`.
- String: `string.concat`, `string.trim`, `string.lower`, `string.upper`, `string.replace`, `string.contains`, `string.equals`, `string.split`, `string.join`.
- List: `list.literal`, `list.append`, `list.concat`, `list.filter.equals`, `list.filter.gt`, `list.map.add`, `list.map.mul`, `list.reduce.sum`, `list.reduce.min`, `list.reduce.max`, `list.count`.
- Camera: `camera.set_pose`, `camera.look_at`, `camera.teleport`, `camera.set_fov`, `camera.build_view_state`.
- Audio: `audio.play`, `audio.stop`, `audio.set_volume`.
- Mesh/Model/Scene: `mesh.load`, `model.spawn`, `model.despawn`, `model.set_transform`, `scene.load`, `scene.clear`, `scene.set_active`.

### Next Steps
- [ ] Publish gameplay workflow templates (FPS/passive camera variants, bullet physics, validation/teleport checks).
- [ ] Expand JSON-driven GUI steps beyond soundboard (replace Lua GUI scripts in demo packages).
- [ ] Publish a workflow step catalog with example JSON snippets for each generic step.
- [ ] Add workflow step analytics (probe events with JSON path + node id + per-step timing/counters).

## Feature Matrix (What You Get, When You Get It)

| Feature | Status | Starter | Pro | Ultra | Enterprise |
| --- | --- | --- | --- | --- | --- |
| Config version gating (`schema_version` / `configVersion`) | Live | [x] | [ ] | [ ] | [ ] |
| JSON Schema validation | Live | [x] | [ ] | [ ] | [ ] |
| Layered config merges + deterministic rules | Partial (extends + `@delete` only) | [x] | [ ] | [ ] | [ ] |
| JSON-path diagnostics | Partial (schema pointers + compiler paths) | [x] | [ ] | [ ] | [ ] |
| IR compilation (scene/resources/render) | Partial (IR built; runtime still Lua-driven) | [ ] | [x] | [ ] | [ ] |
| Render graph DAG build + cycle checks | Live | [ ] | [x] | [ ] | [ ] |
| Pass scheduling + submission planning | Partial (topological order only) | [ ] | [x] | [ ] | [ ] |
| Probe system + structured reports | Partial (no runtime hook coverage yet) | [ ] | [ ] | [x] | [ ] |
| Pipeline compatibility checks | Live | [ ] | [ ] | [x] | [ ] |
| Sampler limits enforced | Live | [ ] | [ ] | [x] | [ ] |
| Shader uniform compatibility enforcement | Planned | [ ] | [ ] | [x] | [ ] |
| Budget enforcement + fallback policies | Partial (textures + GUI caches) | [ ] | [ ] | [x] | [ ] |
| Crash recovery integration | Live | [ ] | [ ] | [x] | [ ] |
| JSON-driven component trees | Planned | [ ] | [ ] | [ ] | [x] |
| Cube demo upgrade | Planned | [ ] | [ ] | [ ] | [x] |
| Hot-reload + rollback | Planned | [ ] | [ ] | [ ] | [x] |

## Deliverables Checklist
- [x] `config/schema/` with versioned JSON Schema and migration notes
- [x] n8n workflow schema + parser support (nodes + connections)
- [~] n8n templates + package workflow conversion (templates shipped; package workflows still contain stubs)
- [x] Workflow step plugins for camera + validation checkpoints + bullet physics + soundboard catalog/GUI/audio
- [x] `src/services/impl/config/config_compiler_service.*` for JSON -> IR compilation
- [x] `src/services/impl/render/render_graph_service.*` for graph build and scheduling
- [x] `src/services/interfaces/i_probe_service.hpp` plus report/event types
- [x] `src/services/impl/diagnostics/probe_service.*` for logging/queueing probe reports
- [x] `src/services/interfaces/config_ir_types.hpp` for typed IR payloads
- [x] `src/services/impl/shader/shader_pipeline_validator.*` for mesh/shader compatibility checks
- [x] `src/services/impl/diagnostics/crash_recovery_service.*` for heartbeat + hang detection
- [~] Budget enforcement with clear failure modes and fallback resources (textures + GUI caches today)
- [ ] Cube demo config-only boot path

## Tests and Verification Checklist
- [~] Unit tests for schema validation, merge rules, and reference resolution (remaining gaps: component payload validation)
- [x] Graph validation tests for cycles and invalid dependencies
- [x] Pipeline compatibility tests (shader inputs vs mesh layouts)
- [x] Crash recovery timeout tests (`tests/crash_recovery_timeout_test.cpp`)
- [~] Budget enforcement tests (GUI cache pruning + texture tracker covered; transient pool pending)
- [~] Config-driven validation tour (checkpoint captures + image/ratio/luma/sample-point checks)
- [ ] Smoke test: cube demo boots with config-first scene definition
- [x] Workflow parser tests (template loading + invalid step diagnostics + n8n connections)

## Test Strategy (Solid Coverage Plan)
### Goals
- Fail fast on config errors, graph issues, and resource constraints before runtime.
- Protect crash recovery and rendering safety invariants with regression tests.
- Keep config-first path validated even while Lua fallback exists.

### Layered Test Plan
- Unit: schema validation, config merges (`extends`, `@delete`), IR compilation edge cases.
- Service: render graph validation (cycles, unknown refs, duplicates), shader pipeline validation, budget enforcement.
- Integration: shader compilation, MaterialX generation + validation, crash recovery timeouts.
- Smoke: config-first boot of the cube demo with no Lua scene execution.
- Runtime: validation tour checkpoints for production self-test.

### Coverage Matrix (What We Must Prove)
- Config parsing + schema errors produce JSON Pointer diagnostics.
- Merge behavior is deterministic and well-documented for arrays and deletes.
- Render graph validation detects cycles, unknown passes/outputs, and produces stable schedules.
- Shader pipelines reject layout mismatches and uniform incompatibilities.
- Budget enforcement fails safely (textures + GUI caches now, buffers later).
- Crash recovery detects hangs and returns promptly.

### Test Assets + Determinism
- Prefer tiny synthetic assets in `tests/` for deterministic behavior.
- Keep large MaterialX assets for integration tests only.
- Avoid network access in tests; all inputs must be local.

### CI Gate Suggestions
- Quick: unit + service tests (schema/merge/render graph/pipeline validator).
- Full: integration tests (MaterialX + shader linking) and smoke config-first boot.

## Automation Priorities
- Prioritize lint/static analysis (clang-tidy or similar) so no code reaches CI with unchecked warnings; make the lint target part of the default build/test sprint.
- Always run the "triangle/run" regression harness, the static-analysis pass, and the e2e validation workflow (boot → frame → capture) on workstation hardware before claiming readiness.
- Tie the game engine tester config (`config/engine_tester_runtime.json`) into CI to execute teleport/capture checks plus the multi-method screen validation suite (image compare, non-black/luma/mean/samples).

## Troubleshooting Guide (Segfaults, Ordering, Shader Quirks)
### Common Failure Modes
- Segfaults after startup: often caused by invalid bgfx handles, resource exhaustion, or pre-frame usage.
- Draw crashes: index/vertex buffer mismatch or using buffers before upload.
- Shader issues: missing uniforms, incorrect layout qualifiers, or wrong backend profile.
- Ordering bugs: loading shaders/textures before the first `BeginFrame` + `EndFrame` priming pass.
- Vendor blobs in `src/` might have local tweaks; verify the sanitized version before assuming upstream behavior.

### Immediate Triage Steps
- Re-run with trace logging enabled (`--trace`) and capture the last 50 lines of the log.
- Confirm config schema validation passes and print loaded JSON (`--dump-json`).
- Check that shaders are compiled for the active renderer (Vulkan vs OpenGL).
- Ensure bgfx is initialized and has seen a frame before loading textures/shaders.

### Known Hotspots To Inspect
- Shader pipeline validation: `src/services/impl/shader/shader_pipeline_validator.cpp`
- Texture load guards + budgets: `src/services/impl/graphics/bgfx_graphics_backend.cpp`
- Render graph scheduling: `src/services/impl/render/render_graph_service.cpp`
- Config compiler diagnostics: `src/services/impl/config/config_compiler_service.cpp`
- Crash recovery timeouts: `src/services/impl/diagnostics/crash_recovery_service.cpp`

### Ordering Checklist (When Things Crash)
- `InitializeDevice` → `InitializeSwapchain` → `BeginFrame` → `EndFrame` before loading shaders/textures.
- Load shaders once, then upload geometry, then render.
- Avoid calling bgfx APIs after shutdown or on invalid handles.

### Shader Debug Checklist
- Verify `layout(location = N)` on all GLSL inputs/outputs (SPIR-V requirement).
- Check uniform types match expected (sampler vs vec types).
- Validate vertex layout matches shader inputs.

### When Filing A Bug
- Include config JSON, active renderer, last log lines, and crash report (if any).
- Note whether `runtime.scene_source` is `config` or `lua`.

### Known Fixes And Evidence
- Texture load crashes: see `tests/bgfx_texture_loading_test.cpp` and `FIXES_IMPLEMENTED.md`.
- Shader uniform mapping failures: see `tests/shaderc_uniform_mapping_test.cpp` and `tests/gui_shader_linking_failure_test.cpp`.
- Initialization order regressions: see `tests/bgfx_initialization_order_test.cpp` and `tests/bgfx_frame_requirement_test.cpp`.
- Render graph validation gaps: see `tests/render_graph_service_test.cpp` (cycles/unknown refs/duplicates).
- Crash recovery timeouts: see `tests/crash_recovery_timeout_test.cpp`.

### Vendored Library Caveat
- Treat any library code pasted into `src/` (or similar vendor folders) as locally modified until verified.
- Do not assume upstream behavior; always confirm against the local copy when debugging.
## Open Questions
- Preferred merge behavior for array fields (replace vs keyed merge by `id`)
- Scope of hot-reload (full scene reload vs incremental updates)
- Target shader reflection source (bgfx, MaterialX, or custom metadata)
- Strategy for moving from Lua-driven scene scripts to config-first IR execution

## Declarative Package Catalog

### Manifest Expectations
- Treat each package like an npm module: `package.json` + `workflows/` folder + clear `assets/`, `scene/` (not “levels”), optional `shaders/`, `gui/`, and `assets/sound` sub-folders so editors, artists, and automation can find the data without guessing.
- Include `defaultWorkflow`, `workflows`, `assets`, `scene`, and `dependencies` fields in `package.json`; bundle notes, template guidance, and a `bundled` flag for platform-specific exports. A package linter will scan these manifests and warn when fields are missing, workflows are orphaned, dependencies are unspecified, or an active workflow lacks an associated C++ step registry entry.
- Let `scripts/package_lint.py` scan `packages/` for `package.json` manifests so workflow loaders can validate dependencies (e.g., `materialx`) before executing the JSON control plane. Packages that list unused services should emit lint warnings and remain dormant unless a workflow references them.

### Asset & Vendor Hygiene
- Copy static assets from `MaterialX/` and the legacy `config/` asset folders (poster textures, fonts, audio catalogs, procedural generator outputs like `scripts/generate_audio_assets.py` and `scripts/generate_cube_stl.py`) into the appropriate `packages/<name>/assets/` subfolders. When a package owns enough copies, the on-disk `MaterialX` depot becomes optional; treat it as historical/archival until the workflow-only path is exercised.
- Soundboard is more than a GUI prototype—its package must ship fonts, audio samples, and `scene/soundboard_layout.json` definitions so the workflow can reconstitute the old experience. Don’t assume baked-in MaterialX files are canonical; any library code or shaders pasted into `src/` or `MaterialX/` must be audited because they are often locally modified (treat them as untrusted until verified).
- Every package that replaced a Lua-heavy example (seed, gui, soundboard, quake3, engine_tester) needs a workflow that reproduces or improves upon the Lua experience, including teleport checkpoints, animation graphs, GUI cues, and audio triggers. Stub steps are not allowed; workflows must only reference real plugins or they should fail fast.

### Engine Tester & Workflow Templates
- Feature packages ship template workflows and assets so users rarely start from scratch. When a new mechanic is required (e.g., bullet physics, FPS vs. third-person toggles), the workflow simply adds a counted micro step and selects the appropriate scene/shader package.
- The engine tester package (`packages/engine_tester`) doubles as the CI validation tour: teleport checkpoints, expected render captures, and a multi-method screen verification suite (image diff + brightness/histogram heuristics) live entirely in JSON so the runtime can teleport the camera, render the requested view, and fail fast if what the user sees deviates from the script. This complements the `config/engine_tester_runtime.json` sample.
- Dependencies between packages follow a strict DAG (materialx → shader systems → packages). If a workflow requires `bullet_physics_service` or `audio_service`, those steps must appear as entries in `workflows/<flow>.json`, and the manifest must list the services (instead of implicitly loading them via Lua or global registries).

## Workflow-Driven Service Discipline

### Micro-Stepped Execution
- Think in n8n-sized micro steps: each JSON action maps to a C++ service/plugin with as few methods as possible (aim for <5) and a single well-defined responsibility. Keep files below 100 LOC unless the complexity genuinely demands it, and prefer loops, references, and declarative templates over copy/pasted branches.
- Organize the codebase into plugin/service/controller modules wired through DI registries. A workflow should be able to describe the entire boot → frame → capture pipeline, including teleporting, physics, rendering, GUI, and audio, without falling back to Lua. If “bullet physics” is required, add that step to the workflow and link it to a `BulletPhysicsStep` plugin; if it is absent, the service stays unregistered and unloaded.
- Every workflow must pair a step with a real C++ implementation; stub steps are prohibited. Services that are not referenced by at least one workflow should log a warning and remain dormant.

### Schema & Shader System Extensions
- Continue extending `runtime_config_v2.schema.json` so that scene components, passes, attachments, budgets, and shader-system selectors are validated before runtime. Allow schema extensions per package and support `@extends`/`@delete` so templates can be layered without copying everything.
- The shader pipeline should depend on a pluggable `IShaderSystem` registry; MaterialX stays as the first registered system, but the registry should also be able to host GLSL-only or future runtime shader builders. The config compiler validates the requested shader system, material metadata, and uniform bindings and raises JSON Pointer diagnostics now.
- The workflow should describe which shader system, render pass, and telemetry probes to use so that the C++ services can wire the correct pipeline without implicit logic.

## Lua Sunset & Config Defaults

- Config-first execution is the default path. The CLI flag flow (`--json-file-in` → `--set-default-json` → stored default → seed) stays locked in. Lua only runs when a workflow explicitly routes to a `SceneScriptStep` and the user opts into `runtime.scene_source = "lua"`. Otherwise, running with a JSON workflow should upgrade the config compiler into an IR loader before Lua ever touches the renderer.
- Provide a persistent default config that can be swapped via `--set-default-json` so that automation can change the baseline without shipping a new binary. Warn when the default config is corrupted or missing required `schema_version`.
- When replacing Lua, keep trace logging in place for regressions. Drop in DEBUG/INFO/TRACE/ERROR logs when diagnosing segfaults, shader quirks, or ordering issues; they should reference the JSON path or workflow step so triage teams can quickly identify the failing stage.

## Package Linter & Automation Hooks

- Build a package linter that runs as part of `scripts/lint.sh` (or a dedicated CI job) and flags:
  - missing `defaultWorkflow` or `workflow` definitions that cover boot/frame phases,
  - absent `assets`/`scene` references that the workflow expects,
-  - dependencies pointing at packages or directories that do not exist,
  - unused services or steps referenced by the workflow but lacking C++ counterparts.
- When we repackage an existing demo, the linter should compare the legacy config/workflow + assets (e.g., the old Lua-driven bundle) to the restored package and warn about any missing pieces (assets, scenes, workflows, or service steps) so we can see what still needs to be ported.
- The linter also ensures that packages import the right assets (GUI folder under `gui/`, sound under `assets/sound`, fonts under `assets/fonts`, shaders under `shaders/`) so the runtime can find them deterministically.
- Automation stays disciplined: lint/static analysis must pass before accepting code, and the “triangle/run” regression harness (or its eventual equivalent) stays part of every sprint along with E2E boot→frame→capture execution and `config/engine_tester_runtime.json` teleport checks.

## Testing Strategy Additions

- Maintain the full testing pyramid: unit (schema, merge, IR), service (render graph, shader pipeline, budgets), integration (MaterialX shader generation, shader linking with both Vulkan/OpenGL), and end-to-end verification (boot the package, run the configured workflow, capture the frame).
- In addition to the existing automation bullets, add multi-method screen validation (pixel diffs, non-black coverage, histogram/mean heuristics) because we occasionally do not know what the image should contain when new JSON/lua combos hit the runtime. This logic lives inside `engine_tester` and should be pluggable so we can add new heuristics later.
- Static analysis and linters are part of the default sprint (`scripts/lint.sh` covering clang-tidy, cppcheck, compiler warning builds, and sanitizer builds). Build verification must also re-run on `build-ninja-asan` or similar sanitized configurations so we catch ordering/segfault issues early.

n8n style schema:

```json
{
"$schema": "https://json-schema.org/draft/2020-12/schema",
"$id": "https://example.com/schemas/n8n-workflow.schema.json",
"title": "N8N-Style Workflow",
"type": "object",
"additionalProperties": false,
"required": ["name", "nodes", "connections"],
"properties": {
"id": {
"description": "Optional external identifier (DB id, UUID, etc.).",
"type": ["string", "integer"]
},
"name": {
"type": "string",
"minLength": 1
},
"active": {
"type": "boolean",
"default": false
},
"versionId": {
"description": "Optional version identifier for optimistic concurrency.",
"type": "string"
},
"createdAt": {
"type": "string",
"format": "date-time"
},
"updatedAt": {
"type": "string",
"format": "date-time"
},
"tags": {
"type": "array",
"items": { "$ref": "#/$defs/tag" },
"default": []
},
"meta": {
"description": "Arbitrary metadata. Keep stable keys for tooling.",
"type": "object",
"additionalProperties": true,
"default": {}
},
"settings": {
"$ref": "#/$defs/workflowSettings"
},
"pinData": {
"description": "Optional pinned execution data (useful for dev).",
"type": "object",
"additionalProperties": {
"type": "array",
"items": {
"type": "object",
"additionalProperties": true
}
}
},
"nodes": {
"type": "array",
"minItems": 1,
"items": { "$ref": "#/$defs/node" }
},
"connections": {
"$ref": "#/$defs/connections"
},
"staticData": {
"description": "Reserved for engine-managed workflow state.",
"type": "object",
"additionalProperties": true,
"default": {}
},
"credentials": {
"description": "Optional top-level credential bindings (engine-specific).",
"type": "array",
"items": { "$ref": "#/$defs/credentialBinding" },
"default": []
}
},
"$defs": {
"tag": {
"type": "object",
"additionalProperties": false,
"required": ["name"],
"properties": {
"id": { "type": ["string", "integer"] },
"name": { "type": "string", "minLength": 1 }
}
},
"workflowSettings": {
"type": "object",
"additionalProperties": false,
"properties": {
"timezone": {
"description": "IANA timezone name, e.g. Europe/London.",
"type": "string"
},
"executionTimeout": {
"description": "Hard timeout in seconds for a workflow execution.",
"type": "integer",
"minimum": 0
},
"saveExecutionProgress": {
"type": "boolean",
"default": true
},
"saveManualExecutions": {
"type": "boolean",
"default": true
},
"saveDataErrorExecution": {
"description": "Persist execution data on error.",
"type": "string",
"enum": ["all", "none"],
"default": "all"
},
"saveDataSuccessExecution": {
"description": "Persist execution data on success.",
"type": "string",
"enum": ["all", "none"],
"default": "all"
},
"saveDataManualExecution": {
"description": "Persist execution data for manual runs.",
"type": "string",
"enum": ["all", "none"],
"default": "all"
},
"errorWorkflowId": {
"description": "Optional workflow id to call on error.",
"type": ["string", "integer"]
},
"callerPolicy": {
"description": "Optional policy controlling which workflows can call this workflow.",
"type": "string"
}
},
"default": {}
},
"node": {
"type": "object",
"additionalProperties": false,
"required": ["id", "name", "type", "typeVersion", "position"],
"properties": {
"id": {
"description": "Stable unique id within the workflow. Prefer UUID.",
"type": "string",
"minLength": 1
},
"name": {
"description": "Human-friendly name; should be unique in workflow.",
"type": "string",
"minLength": 1
},
"type": {
"description": "Node type identifier, e.g. n8n-nodes-base.httpRequest.",
"type": "string",
"minLength": 1
},
"typeVersion": {
"description": "Node implementation version.",
"type": ["integer", "number"],
"minimum": 1
},
"disabled": {
"type": "boolean",
"default": false
},
"notes": {
"type": "string",
"default": ""
},
"notesInFlow": {
"description": "When true, notes are displayed on canvas.",
"type": "boolean",
"default": false
},
"retryOnFail": {
"type": "boolean",
"default": false
},
"maxTries": {
"type": "integer",
"minimum": 1
},
"waitBetweenTries": {
"description": "Milliseconds.",
"type": "integer",
"minimum": 0
},
"continueOnFail": {
"type": "boolean",
"default": false
},
"alwaysOutputData": {
"type": "boolean",
"default": false
},
"executeOnce": {
"description": "If true, node executes only once per execution (engine-dependent).",
"type": "boolean",
"default": false
},
"position": {
"$ref": "#/$defs/position"
},
"parameters": {
"description": "Node-specific parameters. Typically JSON-serializable.",
"type": "object",
"additionalProperties": true,
"default": {}
},
"credentials": {
"description": "Node-level credential references.",
"type": "object",
"additionalProperties": {
"$ref": "#/$defs/credentialRef"
},
"default": {}
},
"webhookId": {
"description": "Optional webhook id (for webhook-based trigger nodes).",
"type": "string"
},
"onError": {
"description": "Node-level error routing policy (engine-dependent).",
"type": "string",
"enum": ["stopWorkflow", "continueRegularOutput", "continueErrorOutput"]
}
}
},
"position": {
"type": "array",
"minItems": 2,
"maxItems": 2,
"items": {
"type": "number"
}
},
"credentialRef": {
"type": "object",
"additionalProperties": false,
"required": ["id"],
"properties": {
"id": {
"description": "Credential id or stable key.",
"type": ["string", "integer"]
},
"name": {
"description": "Optional human label.",
"type": "string"
}
}
},
"credentialBinding": {
"type": "object",
"additionalProperties": false,
"required": ["nodeId", "credentialType", "credentialId"],
"properties": {
"nodeId": { "type": "string", "minLength": 1 },
"credentialType": { "type": "string", "minLength": 1 },
"credentialId": { "type": ["string", "integer"] }
}
},
"connections": {
"description": "Adjacency map: fromNodeName -> outputType -> outputIndex -> array of targets.",
"type": "object",
"additionalProperties": {
"$ref": "#/$defs/nodeConnectionsByType"
},
"default": {}
},
"nodeConnectionsByType": {
"type": "object",
"additionalProperties": false,
"properties": {
"main": {
"$ref": "#/$defs/outputIndexMap"
},
"error": {
"$ref": "#/$defs/outputIndexMap"
}
},
"anyOf": [
{ "required": ["main"] },
{ "required": ["error"] }
]
},
"outputIndexMap": {
"description": "Output index -> array of connection targets.",
"type": "object",
"additionalProperties": {
"type": "array",
"items": { "$ref": "#/$defs/connectionTarget" }
},
"default": {}
},
"connectionTarget": {
"type": "object",
"additionalProperties": false,
"required": ["node", "type", "index"],
"properties": {
"node": {
"description": "Target node name (n8n uses node 'name' in connections).",
"type": "string",
"minLength": 1
},
"type": {
"description": "Input type on target node (typically 'main' or 'error').",
"type": "string",
"minLength": 1
},
"index": {
"description": "Input index on target node.",
"type": "integer",
"minimum": 0
}
}
}
}
}
```


```json
example data:
{
"id": "wf-001",
"name": "Example HTTP → Transform → Log",
"active": false,
"createdAt": "2026-01-10T10:15:00Z",
"updatedAt": "2026-01-10T10:20:00Z",
"tags": [
{ "id": 1, "name": "example" },
{ "id": 2, "name": "demo" }
],
"settings": {
"timezone": "Europe/London",
"executionTimeout": 300,
"saveExecutionProgress": true,
"saveDataErrorExecution": "all",
"saveDataSuccessExecution": "all"
},
"nodes": [
{
"id": "node-1",
"name": "HTTP Request",
"type": "n8n-nodes-base.httpRequest",
"typeVersion": 4,
"position": [0, 0],
"parameters": {
"method": "GET",
"url": "https://api.example.com/users",
"responseFormat": "json"
},
"credentials": {
"httpBasicAuth": {
"id": "cred-123",
"name": "Example API Auth"
}
}
},
{
"id": "node-2",
"name": "Transform Data",
"type": "n8n-nodes-base.function",
"typeVersion": 1,
"position": [300, 0],
"parameters": {
"code": "return items.map(i => ({ json: { id: i.json.id, email: i.json.email } }));"
}
},
{
"id": "node-3",
"name": "Log Output",
"type": "n8n-nodes-base.noOp",
"typeVersion": 1,
"position": [600, 0],
"parameters": {}
}
],
"connections": {
"HTTP Request": {
"main": {
"0": [
{
"node": "Transform Data",
"type": "main",
"index": 0
}
]
}
},
"Transform Data": {
"main": {
"0": [
{
"node": "Log Output",
"type": "main",
"index": 0
}
]
}
}
},
"staticData": {},
"credentials": [
{
"nodeId": "node-1",
"credentialType": "httpBasicAuth",
"credentialId": "cred-123"
}
]
}
```
