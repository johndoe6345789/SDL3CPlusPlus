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

## Launch Packages (Cheesy Edition)

### Starter Plan: "Bootstrap Hosting"
- [x] Config version gating (`schema_version` / `configVersion` checks)
- [ ] JSON Schema validation (external schema + validator)
- [~] JSON-path diagnostics (path strings in exceptions, not full JSON Pointer coverage)
- [~] Layered config merges (supports `extends` + `@delete`; no profile/local/CLI yet)
- [x] Trace logging around config load, validation, and merge steps
- [ ] Migration stubs for future versions

### Pro Plan: "Graph Builder"
- [ ] Typed IRs: `SceneIR`, `ResourceIR`, `RenderGraphIR`
- [ ] Symbol tables + reference resolution with clear diagnostics
- [ ] Render graph DAG compile with cycle detection
- [ ] "Use before produce" validation for render pass inputs
- [ ] Explicit pass scheduling and backend submission planning

### Ultra Plan: "Probe Fortress"
- [ ] Probe hooks: `OnLoadScene`, `OnCreatePipeline`, `OnDraw`, `OnPresent`, `OnFrameEnd`
- [x] Pipeline compatibility checks (mesh layout vs shader inputs) via shader pipeline validator
- [x] Sampler limits enforced from bgfx caps
- [ ] Shader uniform compatibility enforcement
- [~] Resource budget enforcement (texture memory + max texture dim + GUI caches)
- [x] Crash recovery service (heartbeats, GPU hang detection, memory monitoring)
- [ ] Probe severity mapped to crash recovery policies

### Enterprise Plan: "Demo Deluxe"
- [ ] Service refactors to reduce boilerplate and hardcoded state
- [ ] JSON-driven component tree generation (entities, materials, passes)
- [ ] Cube demo rebuilt on config-first scene + render pipeline
- [ ] Hot-reload with diff logging and rollback on validation failure
- [ ] Configurable feature flags to isolate subsystems quickly

## Feature Matrix (What You Get, When You Get It)

| Feature | Status | Starter | Pro | Ultra | Enterprise |
| --- | --- | --- | --- | --- | --- |
| Config version gating (`schema_version` / `configVersion`) | Live | [x] | [ ] | [ ] | [ ] |
| JSON Schema validation | Planned | [x] | [ ] | [ ] | [ ] |
| Layered config merges + deterministic rules | Partial | [x] | [ ] | [ ] | [ ] |
| JSON-path diagnostics | Partial | [x] | [ ] | [ ] | [ ] |
| IR compilation (scene/resources/render) | Planned | [ ] | [x] | [ ] | [ ] |
| Render graph DAG build + cycle checks | Planned | [ ] | [x] | [ ] | [ ] |
| Pass scheduling + submission planning | Planned | [ ] | [x] | [ ] | [ ] |
| Probe system + structured reports | Planned | [ ] | [ ] | [x] | [ ] |
| Pipeline compatibility checks | Live | [ ] | [ ] | [x] | [ ] |
| Sampler limits enforced | Live | [ ] | [ ] | [x] | [ ] |
| Shader uniform compatibility enforcement | Planned | [ ] | [ ] | [x] | [ ] |
| Budget enforcement + fallback policies | Partial | [ ] | [ ] | [x] | [ ] |
| Crash recovery integration | Live | [ ] | [ ] | [x] | [ ] |
| JSON-driven component trees | Planned | [ ] | [ ] | [ ] | [x] |
| Cube demo upgrade | Planned | [ ] | [ ] | [ ] | [x] |
| Hot-reload + rollback | Planned | [ ] | [ ] | [ ] | [x] |

## Deliverables Checklist
- [ ] `config/schema/` with versioned JSON Schema and migration notes
- [ ] `src/services/impl/config_compiler_service.*` for JSON -> IR compilation
- [ ] `src/services/impl/render_graph_service.*` for graph build and scheduling
- [ ] `src/services/interfaces/i_probe_service.hpp` plus report/event types
- [~] Budget enforcement with clear failure modes and fallback resources (textures + GUI caches today)
- [ ] Cube demo config-only boot path

## Tests and Verification Checklist
- [ ] Unit tests for schema validation, reference resolution, and merge rules
- [ ] Graph validation tests for cycles and invalid dependencies
- [x] Pipeline compatibility tests (shader inputs vs mesh layouts)
- [ ] Budget enforcement tests (over-limit textures, transient pool caps)
- [ ] Smoke test: cube demo boots with config-first scene definition

## Open Questions
- Preferred merge behavior for array fields (replace vs keyed merge by `id`)
- Scope of hot-reload (full scene reload vs incremental updates)
- Target shader reflection source (bgfx, MaterialX, or custom metadata)
