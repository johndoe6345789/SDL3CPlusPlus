#include "services/impl/workflow/workflow_registrar_categories.hpp"

#include "services/interfaces/workflow/workflow_package_shader_loader_step.hpp"
#include "services/interfaces/workflow/workflow_shader_builtin_constant_color_step.hpp"
#include "services/interfaces/workflow/workflow_shader_compile_step.hpp"
#include "services/interfaces/workflow/workflow_shader_system_initialize_step.hpp"
#include "services/interfaces/workflow/workflow_shader_system_gltf_load_step.hpp"
#include "services/interfaces/workflow/workflow_shader_system_compile_step.hpp"
#include "services/interfaces/workflow/workflow_shader_system_set_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterShaderSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IGraphicsService> graphicsSvc,
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry) {
    if (!registry) return 0;

    int count = 0;

    // ── Package shader loader ─────────────────────────────────
    registry->RegisterStep(std::make_shared<WorkflowPackageShaderLoaderStep>(
        logger, "", std::filesystem::path{}));
    count += 1;

    // ── Shader system (service-dependent, nullptr until wired) ─
    registry->RegisterStep(
        std::make_shared<WorkflowShaderBuiltinConstantColorStep>(logger,
                                                                 graphicsSvc));
    registry->RegisterStep(std::make_shared<WorkflowShaderCompileStep>(
        logger, shaderRegistry, graphicsSvc));
    // shader.system.initialize split into atomic steps; chain in order.
    registry->RegisterStep(
        std::make_shared<WorkflowShaderSystemInitializeStep>(logger));
    registry->RegisterStep(
        std::make_shared<WorkflowShaderSystemGltfLoadStep>(logger));
    registry->RegisterStep(std::make_shared<WorkflowShaderSystemCompileStep>(
        logger, shaderRegistry, graphicsSvc));
    registry->RegisterStep(
        std::make_shared<WorkflowShaderSystemSetStep>(logger, shaderRegistry));
    count += 6;

    return count;
}

}  // namespace sdl3cpp::services::impl::registrar_detail
