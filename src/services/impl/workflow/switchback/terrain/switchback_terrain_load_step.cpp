#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_load_step.hpp"

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_mesh.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kDefaultStepM = 1800.f / 1024.f;

void UploadChunks(SDL_GPUDevice* device, const SwitchbackHeightmap& map,
                  const SwitchbackChunkSpec& base,
                  SwitchbackTerrainState& state) {
    const int perSide = (map.size - 1) / base.cells;
    state.chunks.reserve(static_cast<std::size_t>(perSide) * perSide);
    for (int j = 0; j < perSide; ++j) {
        for (int i = 0; i < perSide; ++i) {
            SwitchbackChunkSpec spec = base;
            spec.firstI = i * base.cells;
            spec.firstJ = j * base.cells;
            const GeometryPlaneMesh mesh = BuildSwitchbackChunkMesh(map, spec);
            const GeometryPlaneBuffers buffers =
                UploadGeometryPlaneMesh(device, mesh);
            SwitchbackTerrainChunk chunk;
            chunk.vertexBuffer = buffers.vertexBuffer;
            chunk.indexBuffer = buffers.indexBuffer;
            chunk.indexCount = static_cast<std::uint32_t>(mesh.indices.size());
            state.chunks.push_back(chunk);
        }
    }
}

void ReleaseChunks(SDL_GPUDevice* device, SwitchbackTerrainState& state) {
    for (const SwitchbackTerrainChunk& chunk : state.chunks) {
        SDL_ReleaseGPUBuffer(device, chunk.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, chunk.indexBuffer);
    }
    state.chunks.clear();
}

}  // namespace

WorkflowSwitchbackTerrainLoadStep::WorkflowSwitchbackTerrainLoadStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<SwitchbackTerrainState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowSwitchbackTerrainLoadStep::GetPluginId() const {
    return "switchback.terrain.load";
}

void WorkflowSwitchbackTerrainLoadStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!state_ || state_->attempted) return;
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!device || !world) return;
    state_->attempted = true;

    const std::string path = Gta5ResolvePath(
        step, context, "heightmap",
        "packages/switchback/assets/spiral_pass_heightmap.r16");
    const float heightMaxM = Gta5NumberOr(step, "height_max_m", 512.f);
    const float stepM = Gta5NumberOr(step, "step_m", kDefaultStepM);
    const int cells = Gta5ParameterOrInt(step, "chunk_cells", 128);
    SwitchbackHeightmap map;
    if (!ReadSwitchbackHeightmap(path, heightMaxM, map)) {
        if (logger_) {
            logger_->Error("switchback.terrain.load: cannot read " + path);
        }
        return;
    }
    if (cells <= 0 || (map.size - 1) % cells != 0) {
        if (logger_) {
            logger_->Error("switchback.terrain.load: chunk_cells " +
                           std::to_string(cells) + " does not divide " +
                           std::to_string(map.size - 1));
        }
        return;
    }

    SwitchbackChunkSpec base;
    base.cells = cells;
    base.stepM = stepM;
    base.uvMetres = Gta5NumberOr(step, "uv_metres", 20.f);
    try {
        UploadChunks(device, map, base, *state_);
    } catch (const std::runtime_error& error) {
        ReleaseChunks(device, *state_);
        if (logger_) {
            logger_->Error(std::string("switchback.terrain.load: ") +
                           error.what());
        }
        return;
    }
    BuildSwitchbackCollision(*world, map, stepM, heightMaxM, *state_);
    if (logger_) {
        logger_->Trace("WorkflowSwitchbackTerrainLoadStep", "Execute",
                       "samples=" + std::to_string(map.size) + ", chunks=" +
                           std::to_string(state_->chunks.size()),
                       "Terrain loaded");
    }
}

}  // namespace sdl3cpp::services::impl
