#include "services/interfaces/workflow/stunts/world/stunts_world_upload.hpp"

#include "services/interfaces/workflow/stunts/data/stunts_shape_mesh.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_wheel_mesh.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_track_mesh.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

StuntsGpuMesh Upload(SDL_GPUDevice* device, const GeometryPlaneMesh& mesh) {
    StuntsGpuMesh out;
    if (!device || mesh.indices.empty()) return out;
    const GeometryPlaneBuffers buffers =
        UploadGeometryPlaneMesh(device, mesh);
    out.vertexBuffer = buffers.vertexBuffer;
    out.indexBuffer = buffers.indexBuffer;
    out.indexCount = static_cast<std::uint32_t>(mesh.indices.size());
    return out;
}

void Release(SDL_GPUDevice* device, StuntsGpuMesh& mesh) {
    if (device && mesh.vertexBuffer) {
        SDL_ReleaseGPUBuffer(device, mesh.vertexBuffer);
    }
    if (device && mesh.indexBuffer) {
        SDL_ReleaseGPUBuffer(device, mesh.indexBuffer);
    }
    mesh = StuntsGpuMesh{};
}

constexpr int kStuntsWheelSegments = 16;

/// The car body's real geometry: its flat-shaded panels, plus the
/// wheel discs generated from its Wheel primitives' vertex refs.
void UploadStuntsCarBody(SDL_GPUDevice* device, StuntsWorldState& state) {
    if (!state.carBody.valid) return;
    GeometryPlaneMesh wheels;
    for (const StuntsShapeFace& face : state.carBody.faces) {
        AppendStuntsWheelMesh(wheels, state.carBody, face,
                              kStuntsWheelSegments);
    }
    state.carPanels = Upload(device, BuildStuntsShapeMesh(state.carBody));
    state.carWheels = Upload(device, wheels);
    state.carBodyLoaded = state.carPanels.indexCount > 0;
}

}  // namespace

void UploadStuntsWorld(WorkflowContext& context, StuntsWorldState& state,
                       const std::shared_ptr<ILogger>& logger) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) return;

    const StuntsTrackMesh mesh =
        BuildStuntsTrackMesh(state.track, state.table, state.params);
    state.roadTiles = mesh.roadTiles;
    try {
        state.road = Upload(device, mesh.road);
        state.ground = Upload(device, mesh.ground);
        UploadStuntsCarBody(device, state);
    } catch (const std::runtime_error& error) {
        Release(device, state.road);
        Release(device, state.ground);
        Release(device, state.carPanels);
        Release(device, state.carWheels);
        if (logger) {
            logger->Error(std::string("stunts.world.load: upload failed: ") +
                          error.what());
        }
        return;
    }
    state.loaded = state.road.indexCount > 0;
    if (logger && state.carBodyLoaded) {
        logger->Info("stunts.world.load: car body " +
                     std::to_string(state.carBody.vertices.size()) +
                     " vertices, " +
                     std::to_string(state.carBody.faces.size()) +
                     " primitives");
    }
}

void ReleaseStuntsWorld(SDL_GPUDevice* device, StuntsWorldState& state) {
    Release(device, state.road);
    Release(device, state.ground);
    Release(device, state.carPanels);
    Release(device, state.carWheels);
    state.loaded = false;
    state.carBodyLoaded = false;
}

}  // namespace sdl3cpp::services::impl
