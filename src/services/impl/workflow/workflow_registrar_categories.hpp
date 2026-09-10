#pragma once

/// Internal split of WorkflowRegistrar::RegisterSteps() into per-category
/// registration functions. Each function registers one contiguous slice of
/// the original registration order; WorkflowRegistrar::RegisterSteps()
/// (workflow_registrar.cpp) calls them in the exact original order and sums
/// their returned counts for the "base steps registered" log line.

#include "services/interfaces/i_audio_service.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_input_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_scene_service.hpp"
#include "services/interfaces/i_shader_system_registry.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"

#include <memory>

namespace sdl3cpp::services::impl::registrar_detail {

int RegisterGraphicsInitSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                               std::shared_ptr<ILogger> logger);

int RegisterRenderingCoreSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger);

int RegisterRenderingBspSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                               std::shared_ptr<ILogger> logger);

int RegisterRenderingPostfxSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger);

int RegisterRenderingOverlaySteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger);

int RegisterRenderingQ3HudSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger);

int RegisterRenderingQ3ModelSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger);

int RegisterRenderingQ3PmoveSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger);

int RegisterRenderingQ3CombatSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger);

int RegisterGraphicsMiscSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                               std::shared_ptr<ILogger> logger);

int RegisterCameraSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                         std::shared_ptr<ILogger> logger);

int RegisterPhysicsSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                          std::shared_ptr<ILogger> logger);

int RegisterInputSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                        std::shared_ptr<ILogger> logger,
                        std::shared_ptr<IInputService> inputSvc);

int RegisterAudioSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                        std::shared_ptr<ILogger> logger,
                        std::shared_ptr<IAudioService> audioSvc);

int RegisterControlSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                          std::shared_ptr<ILogger> logger);

int RegisterDataOpsCoreSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                              std::shared_ptr<ILogger> logger);

int RegisterListNumberSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                             std::shared_ptr<ILogger> logger);

int RegisterParticleStringValueVfxSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger);

int RegisterModelSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                        std::shared_ptr<ILogger> logger);

int RegisterSceneSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                        std::shared_ptr<ILogger> logger,
                        std::shared_ptr<ISceneService> sceneSvc);

int RegisterCameraServiceSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IConfigService> configSvc);

int RegisterSystemDataSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                             std::shared_ptr<ILogger> logger);

int RegisterMediaSteps(std::shared_ptr<IWorkflowStepRegistry> registry,
                        std::shared_ptr<ILogger> logger,
                        std::shared_ptr<IConfigService> configSvc,
                        std::shared_ptr<IAudioService> audioSvc);

int RegisterShaderSteps(
    std::shared_ptr<IWorkflowStepRegistry> registry,
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<IGraphicsService> graphicsSvc,
    std::shared_ptr<IShaderSystemRegistry> shaderRegistry);

int RegisterExitStep(std::shared_ptr<IWorkflowStepRegistry> registry,
                      std::shared_ptr<ILogger> logger);

}  // namespace sdl3cpp::services::impl::registrar_detail
