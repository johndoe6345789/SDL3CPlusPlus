# ROADMAP

## North Star
Treat JSON config as a declarative control plane that compiles into scene, resource, and render graphs with strict validation, budget enforcement, and crash-resistant policies.

## Guiding Principles
- Fail fast with clear JSON-path error reporting.
- Keep APIs explicit, predictable, and easy to reason about.
- Prefer refactoring that reduces boilerplate and hardcoded state.

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

## Launch Packages (Cheesy Edition)

### Starter Plan: "Bootstrap Hosting"
- [x] Config version gating (`schema_version` / `configVersion` checks)
- [x] JSON Schema validation (external schema + validator)
- [~] JSON-path diagnostics (schema validator pointers + compiler-generated paths; not full JSON Pointer coverage)
- [~] Layered config merges (supports `extends` + `@delete`; no profile/local/CLI yet)
- [x] Trace logging around config load, validation, and merge steps
- [~] Migration stubs for future versions (notes + stubbed hook)

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
- Add tests for schema/merge rules, render graph validation, and budget enforcement.

## Config-First Program Plan (Verbose)

### Decisions Locked In
- Config-first is the default runtime path. Lua becomes optional or secondary.
- Users can persist a default runtime config (via `--set-default-json`).
- Schema extensions are allowed.
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
- Acceptance: MaterialX stays working, and a second stub system (e.g., `glsl`) can be registered without touching `IGraphicsService`.

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
- [x] `src/services/impl/config_compiler_service.*` for JSON -> IR compilation
- [x] `src/services/impl/render_graph_service.*` for graph build and scheduling
- [x] `src/services/interfaces/i_probe_service.hpp` plus report/event types
- [x] `src/services/impl/probe_service.*` for logging/queueing probe reports
- [x] `src/services/interfaces/config_ir_types.hpp` for typed IR payloads
- [x] `src/services/impl/shader_pipeline_validator.*` for mesh/shader compatibility checks
- [x] `src/services/impl/crash_recovery_service.*` for heartbeat + hang detection
- [~] Budget enforcement with clear failure modes and fallback resources (textures + GUI caches today)
- [ ] Cube demo config-only boot path

## Tests and Verification Checklist
- [~] Unit tests for schema validation, merge rules, and reference resolution (remaining gaps: component payload validation)
- [x] Graph validation tests for cycles and invalid dependencies
- [x] Pipeline compatibility tests (shader inputs vs mesh layouts)
- [x] Crash recovery timeout tests (`tests/crash_recovery_timeout_test.cpp`)
- [~] Budget enforcement tests (GUI cache pruning + texture tracker covered; transient pool pending)
- [ ] Smoke test: cube demo boots with config-first scene definition

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

## Troubleshooting Guide (Segfaults, Ordering, Shader Quirks)
### Common Failure Modes
- Segfaults after startup: often caused by invalid bgfx handles, resource exhaustion, or pre-frame usage.
- Draw crashes: index/vertex buffer mismatch or using buffers before upload.
- Shader issues: missing uniforms, incorrect layout qualifiers, or wrong backend profile.
- Ordering bugs: loading shaders/textures before the first `BeginFrame` + `EndFrame` priming pass.

### Immediate Triage Steps
- Re-run with trace logging enabled (`--trace`) and capture the last 50 lines of the log.
- Confirm config schema validation passes and print loaded JSON (`--dump-json`).
- Check that shaders are compiled for the active renderer (Vulkan vs OpenGL).
- Ensure bgfx is initialized and has seen a frame before loading textures/shaders.

### Known Hotspots To Inspect
- Shader pipeline validation: `src/services/impl/shader_pipeline_validator.cpp`
- Texture load guards + budgets: `src/services/impl/bgfx_graphics_backend.cpp`
- Render graph scheduling: `src/services/impl/render_graph_service.cpp`
- Config compiler diagnostics: `src/services/impl/config_compiler_service.cpp`
- Crash recovery timeouts: `src/services/impl/crash_recovery_service.cpp`

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
