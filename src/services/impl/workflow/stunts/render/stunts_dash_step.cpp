#include "services/interfaces/workflow/stunts/render/stunts_dash_step.hpp"

#include "services/impl/workflow/stunts/render/stunts_dashboard_draw.hpp"
#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/stunts/render/stunts_dashboard_mesh.hpp"
#include "services/interfaces/workflow/stunts/stunts_step_params.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

void ReleaseMesh(SDL_GPUDevice* device, SDL_GPUBuffer*& vb,
                 SDL_GPUBuffer*& ib) {
    if (device && vb) SDL_ReleaseGPUBuffer(device, vb);
    if (device && ib) SDL_ReleaseGPUBuffer(device, ib);
    vb = nullptr;
    ib = nullptr;
}

}  // namespace

WorkflowStuntsDashboardDrawStep::WorkflowStuntsDashboardDrawStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowStuntsDashboardDrawStep::GetPluginId() const {
    return "stunts.dashboard.draw";
}

void WorkflowStuntsDashboardDrawStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    const bool inCockpit =
        context.Get<std::string>("stunts.camera_mode", "chase") == "cockpit";
    if (context.GetBool("frame_skip", false) || !inCockpit) {
        ReleaseMesh(device, vertexBuffer_, indexBuffer_);
        indexCount_ = 0;
        return;
    }
    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "gpu_pipeline_textured", nullptr);
    if (!device || !pass || !cmd || !pipeline) return;

    const float speedMax = StuntsNumberOr(step, "speed_max_mph", 180.f);
    const float redline = StuntsNumberOr(step, "redline_rpm", 8000.f);
    const float speedFrac = std::clamp(
        context.Get<float>("stunts.speed_mph", 0.f) / speedMax, 0.f, 1.f);
    const float rpmFrac = std::clamp(
        context.Get<float>("stunts.rpm", 0.f) / std::max(redline, 1.f), 0.f,
        1.f);

    ReleaseMesh(device, vertexBuffer_, indexBuffer_);
    const GeometryPlaneMesh mesh =
        BuildStuntsDashboardMesh(speedFrac, rpmFrac);
    if (mesh.indices.empty()) return;
    const GeometryPlaneBuffers buffers =
        UploadGeometryPlaneMesh(device, mesh);
    vertexBuffer_ = buffers.vertexBuffer;
    indexBuffer_ = buffers.indexBuffer;
    indexCount_ = static_cast<std::uint32_t>(mesh.indices.size());

    DrawStuntsDashboardMesh(context, pass, cmd, pipeline, vertexBuffer_,
                           indexBuffer_, indexCount_);
}

}  // namespace sdl3cpp::services::impl
