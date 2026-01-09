
Think of the JSON as a declarative scene + render plan. In a 3D engine, the “layout” you can probe is not just UI structure; it is the scene graph, resource graph, and render graph. You can validate those early and continuously.

Practical mapping:

JSON describes entities, components, materials, and render passes.
Engine compiles JSON into a render graph (passes, dependencies, resource lifetimes).
Probes inspect that graph and the runtime state during frame execution.
Where probes help most:

Load-time schema + graph validation: missing components, invalid references, cycles in render graph, illegal pass order.
Pipeline compatibility: vertex layout vs shader inputs, sampler counts, texture formats, uniform ranges.
Resource budget checks: VRAM budget, transient allocations, texture size limits, buffer counts, descriptor limits.
Runtime liveness: frame heartbeat, GPU hang detection, device-lost detection, swapchain errors.
Draw safety: buffer bounds, valid handles, lifetime tracking (avoid use-after-free).
A lightweight probe architecture:

Probe hooks like OnLoadScene, OnCreatePipeline, OnDraw, OnPresent, OnFrameEnd.
Each probe emits a structured report (severity, JSON path, render pass, resource id).
Fatal probes can skip the draw or trigger recovery; non-fatal probes just warn.
Examples of early catches in a 3D engine:

A material references a texture that exceeds max size or blows the VRAM budget.
A shader expects a TexCoord attribute that the mesh never provides.
Render pass order uses a depth target before it is produced.
A draw call submits indices out of bounds for the active vertex buffer.
If you want, I can outline a concrete probe interface and wire points for your engine (e.g., scene load, pipeline creation, draw submission) or tailor it to this repo’s bgfx pipeline.

Yes—your JSON config can become a real “declarative control plane” for the engine instead of just startup settings. The easiest wins are around validation, layering, and constraints.

Ways to power it up

Schema + versioning: add configVersion and validate with a JSON schema so you fail fast with clear errors (and optional migration).
Layered configs: support extends/includes with deterministic merge rules (base → profile → local → CLI overrides).
Resource constraints: declare budgets (VRAM, texture max dims, shader size limits) and enforce at load time with fallback policies.
Runtime hooks: hot‑reload configs with diff logging + rollback if validation fails.
Feature flags: enable/disable systems or render paths from config to isolate issues quickly.

## Target architecture: JSON as a declarative control plane

Treat your JSON as **one source of truth** for three graphs that the engine compiles and executes:

1. **Scene graph** (entities + components + transforms + hierarchy)
2. **Resource graph** (meshes, textures, shaders, materials, pipelines; ownership + lifetimes)
3. **Render graph** (passes, dependencies, attachments, transient resources, scheduling)

The key is to make compilation explicit:

* **JSON → IR (validated, resolved, typed)**
* **IR → RenderGraph (passes + resources + edges + lifetimes)**
* **RenderGraph → Backend submission plan (bgfx encoders/views/frame loop)**

That gives you deterministic “wire points” for probes and also makes error reporting land on **JSON paths** consistently.

---

## JSON shape that compiles cleanly

A practical, compilation-friendly top-level layout:

```json
{
  "configVersion": 3,
  "profiles": { "active": "dev", "overrides": [] },

  "budgets": {
    "vramMB": 2048,
    "transientMB": 256,
    "maxTextureDim": 8192,
    "maxSamplersPerMaterial": 16,
    "maxDrawsPerFrame": 20000
  },

  "assets": {
    "textures": { "albedo01": { "uri": "tex/albedo.ktx2", "format": "BC7" } },
    "meshes":    { "ship": { "uri": "mesh/ship.glb", "submeshes": ["hull","glass"] } },
    "shaders":   { "pbr":  { "vs": "shaders/pbr.vsc", "fs": "shaders/pbr.fsc" } }
  },

  "materials": {
    "shipPbr": {
      "shader": "pbr",
      "defines": { "USE_IBL": true },
      "textures": { "u_albedo": "albedo01" },
      "uniforms": { "u_roughness": 0.45 }
    }
  },

  "scene": {
    "entities": [
      { "id": "cam", "components": { "Transform": {}, "Camera": { "fov": 60 } } },
      { "id": "ship", "components": { "Transform": {}, "Renderable": {
          "mesh": "ship",
          "material": "shipPbr"
      } } }
    ]
  },

  "render": {
    "passes": [
      {
        "id": "gbuffer",
        "type": "drawScene",
        "outputs": {
          "color0": { "format": "RGBA8", "usage": "renderTarget" },
          "depth":  { "format": "D24S8", "usage": "depth" }
        }
      },
      {
        "id": "tonemap",
        "type": "fullscreen",
        "inputs": { "hdr": "@pass.gbuffer.color0" },
        "outputs": { "backbuffer": "@swapchain" }
      }
    ]
  }
}
```

### Design choices that pay off immediately

* Every reference is either:

  * a **symbol** (`"material": "shipPbr"`) or
  * an **addressable handle** (`"@pass.gbuffer.color0"`, `"@swapchain"`)
* You can resolve all refs during compilation and emit crisp, path-based errors.

---

## Compilation pipeline (with probe wire points)

### Phase 0: Parse + schema validation

* Validate `configVersion`
* Validate JSON Schema (types, required fields, allowed enums)
* Optionally run a migration step: `v2 → v3`

**Probe hook:** `OnLoadSceneBegin(jsonRoot)`
**Probe hook:** `OnSchemaValidated(report)`

### Phase 1: Symbol table + reference resolution

Build registries:

* `TextureId → TextureDesc`
* `ShaderId → ShaderDesc`
* `MaterialId → MaterialIR`
* `EntityId → EntityIR`
* `PassId → PassIR`

Resolve references:

* material.shader exists
* renderable.material exists
* render graph inputs reference valid outputs

**Probe hook:** `OnResolveReferences(symbols, report)`

### Phase 2: Compatibility compilation

* Mesh vertex layout vs shader inputs
* Material sampler count vs limits
* Uniform ranges/packing rules
* Texture formats/srgb expectations
* Pass attachment format compatibility

**Probe hook:** `OnCreatePipeline(pipelineDesc, report)`

### Phase 3: Render graph construction + scheduling

* Build DAG of passes
* Detect cycles, missing producers, illegal orders
* Compute resource lifetimes (transients)
* Compute aliasing opportunities (if you want)
* Produce an execution schedule + barrier/transition plan (abstract; backend-specific mapping)

**Probe hook:** `OnRenderGraphBuilt(graph, report)`

### Phase 4: Runtime frame execution

* Submit passes in order
* Track liveness, device state, swapchain state
* Track draw safety invariants

**Probe hooks:**

* `OnFrameBegin(frameIndex, state)`
* `OnDraw(drawCall, bindings, report)`
* `OnPresent(report)`
* `OnFrameEnd(frameStats, report)`

---

## Probe system: concrete interface and reporting

### Core concepts

* **Probe**: stateless or stateful checker that subscribes to hooks
* **Report**: structured events emitted by probes
* **Policy**: what to do on severity (continue, skip draw, fallback, recover, abort)

### Report shape (engine-internal, serializable)

Each event should be attributable to:

* `severity`: `info | warn | error | fatal`
* `code`: stable identifier (e.g., `RG_CYCLE`, `PIPE_ATTR_MISSING`)
* `jsonPath`: pointer-like (`/render/passes/1/inputs/hdr`)
* `renderPass`: optional (`tonemap`)
* `resourceId`: optional (`texture:albedo01`)
* `message`: human string
* `details`: structured fields for tooling

Example event:

```json
{
  "severity": "error",
  "code": "PIPE_ATTR_MISSING",
  "jsonPath": "/scene/entities/1/components/Renderable",
  "renderPass": "gbuffer",
  "resourceId": "mesh:ship",
  "message": "Shader expects attribute a_texcoord0 but mesh vertex layout does not provide it",
  "details": { "expected": ["a_position","a_normal","a_texcoord0"], "provided": ["a_position","a_normal"] }
}
```

### Hook signatures (minimal but sufficient)

A simple, backend-agnostic set:

* `OnLoadSceneBegin(jsonRoot)`
* `OnSchemaValidated(schemaResult)`
* `OnResolveReferences(symbolTable, resolveResult)`
* `OnRenderGraphBuilt(renderGraph, schedule)`
* `OnCreatePipeline(pipelineKey, pipelineDesc)`
* `OnFrameBegin(frameCtx)`
* `OnDraw(drawCtx)`
* `OnPresent(presentCtx)`
* `OnFrameEnd(frameStats)`

A “drawCtx” should include:

* pass id, view id (bgfx)
* pipeline key
* bound buffers/textures/uniforms
* index count / vertex count
* handles (so liveness checks can run)

### Policies (what a fatal probe can do)

* **Skip draw** (most useful for bad draw submissions)
* **Skip pass** (if pass inputs invalid; degrade gracefully)
* **Fallback resource** (substitute texture/material/pipeline)
* **Trigger recovery** (device-lost path)
* **Abort load** (schema/graph invalid)

Keep the policy table declarative too (configurable by profile).

---

## Probes that give you the best ROI

### 1) Load-time schema + graph validation probes

* Missing required components
* Invalid IDs / dangling references
* Render graph cycle detection
* “Input used before produced”
* Attachment format mismatches

### 2) Pipeline compatibility probes

* Mesh vertex layout ↔ shader attribute reflection map
* Sampler count / texture unit limit
* Uniform type/range checks
* Texture format compatibility (e.g., sampling depth as color)

### 3) Resource budget probes

* VRAM estimate per texture/mesh (rough but useful)
* Transient render target pool size
* Descriptor-like limits (in bgfx terms: texture stage count, uniform limits)

### 4) Runtime liveness probes

* Frame heartbeat (CPU + GPU pacing signals)
* Device lost / reset detection
* Swapchain present failures
* Hung frame detection (time since last present)

### 5) Draw safety probes

* Index buffer bounds
* Vertex buffer bounds / stride correctness
* Handle validity and lifetime tracking (avoid use-after-free)
* “Draw without pipeline” / missing bindings

---

## Tailoring to bgfx specifically

bgfx already resembles a render-graph-ish system via **views** + **encoder submissions**, but you will want a thin abstraction:

### Mapping render passes → bgfx views

* Each pass becomes a **view id** (stable mapping; e.g., hash(passId) % range, or allocate sequentially)
* Use `bgfx::setViewFrameBuffer(view, fb)` for attachments
* Use `bgfx::setViewClear(view, ...)`
* Use `bgfx::setViewRect(view, ...)`
* Use `bgfx::touch(view)` to ensure execution even if empty (useful for liveness probes)

### Dependency management

bgfx doesn’t expose explicit barriers; ordering is primarily:

* by view id ordering (and calls to setView... and submit)
* plus explicit resource usage implied by handles

So your render graph compiler should:

* produce a **topologically sorted pass list**
* assign monotonically increasing view ids for that schedule (or remap ids per frame deterministically)

### Pipeline creation probe points

* When creating/choosing `bgfx::ProgramHandle`
* When creating vertex layout: `bgfx::VertexLayout`
* When binding textures: stage index constraints

### Runtime probes in bgfx loop

* `OnFrameBegin`: before any encoder work
* `OnDraw`: just before `bgfx::submit(view, program)`
* `OnPresent`: around `bgfx::frame()` return value / timing
* `OnFrameEnd`: after `bgfx::frame()`

---

## Layered configs and deterministic overrides

To support base → profile → local → CLI without chaos:

* `extends`: list of config files
* Merge rules:

  * objects: deep merge
  * arrays: either replace or keyed-merge (recommend keyed by `id` for entities/passes)
  * allow `@delete` directives for removals

Add a probe:

* `CONFIG_MERGE_CONFLICT` when two layers define the same id with incompatible types

---

## What I would implement first (minimum viable, high leverage)

1. **Schema validation + JSON path error reporting**
2. **Symbol resolution + reference validation**
3. **Render graph DAG + cycle detection + “use before produce”**
4. **Pipeline compatibility checks (mesh ↔ shader inputs; sampler counts)**
5. **Draw safety probe for index/vertex bounds**
6. **Budget estimation for textures + transient render targets**

That set catches the majority of “mysterious black screen” and “random GPU crash” classes early.

---

## If you want “repo-grade” concreteness next

I can produce a full, implementable spec for:

* the **exact JSON Schema**
* the **IR types** (SceneIR, MaterialIR, PassIR, RenderGraph)
* the **probe API** (interfaces, event bus, policies, report sinks)
* the **bgfx integration points** (view allocation, pass execution template)

If you share:

* your current bgfx setup (views, framebuffers, shader pipeline conventions),
* and whether you want glTF as the primary asset format,
  I will lock the design to your conventions and avoid inventing abstractions you will later delete.
  
See if anything in NEW_DESIGN.md we can implement. Do refactoring. Update cube demo. See if we can use our JSON config concept to make app more rugged (VRAM limits on everything, crash resistant settings) and use less boilerplate. I do a similar thing with React to generate component trees and declaratively set props.
