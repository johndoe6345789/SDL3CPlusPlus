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
- Config compiler builds Scene/Resource/RenderGraph IR, resolves asset/material/render-pass refs, and schedules a render pass DAG; IR is not yet driving runtime rendering.
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
- [~] Probe hooks (config/render graph/graphics reports wired; missing `OnLoadScene`, `OnDraw`, `OnPresent`, `OnFrameEnd`)
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
- Deliverable: schema guarantees all data needed for IR compilation and render execution.
- Acceptance: invalid configs fail with JSON Pointer diagnostics from schema validation.

### Phase 2: Pluggable Shader System Architecture (3-6 days)
- Define an `IShaderSystem` interface with explicit methods: `GetId()`, `BuildShader`, `GetReflection`, `GetDefaultTextures`.
- Add a shader system registry for discovery and selection.
- Implement `MaterialXShaderSystem` using existing MaterialX generator logic.
- Update shader loading to use the selected shader system to build `ShaderPaths`.
- Deliverable: shader generation/compilation becomes a plugin choice, not hardcoded.
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
- Deliverable: render graph scheduling is executed, not just computed.
- Acceptance: a two-pass graph (offscreen + swapchain) renders correctly.

### Phase 6: Runtime Probe Hooks And Recovery Policy (3-6 days)
- Add runtime probe hooks (`OnDraw`, `OnPresent`, `OnFrameEnd`) in render coordinator + graphics backend.
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
- Add shader system registry tests for multi-system support.
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
- [ ] Unit tests for schema validation, reference resolution, and merge rules
- [ ] Graph validation tests for cycles and invalid dependencies
- [x] Pipeline compatibility tests (shader inputs vs mesh layouts)
- [x] Crash recovery timeout tests (`tests/crash_recovery_timeout_test.cpp`)
- [ ] Budget enforcement tests (over-limit textures, transient pool caps)
- [ ] Smoke test: cube demo boots with config-first scene definition

## Open Questions
- Preferred merge behavior for array fields (replace vs keyed merge by `id`)
- Scope of hot-reload (full scene reload vs incremental updates)
- Target shader reflection source (bgfx, MaterialX, or custom metadata)
- Strategy for moving from Lua-driven scene scripts to config-first IR execution
