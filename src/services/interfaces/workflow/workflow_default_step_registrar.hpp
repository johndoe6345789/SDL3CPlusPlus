#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/i_workflow_step_registry.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_window_service.hpp"
#include "services/interfaces/i_platform_service.hpp"
#include "services/interfaces/i_input_service.hpp"
#include "services/interfaces/workflow_definition.hpp"

#include <string>
#include <unordered_set>
#include <memory>

namespace sdl3cpp::services::impl {

class WorkflowDefaultStepRegistrar {
public:
    WorkflowDefaultStepRegistrar(std::shared_ptr<ILogger> logger,
                                 std::shared_ptr<IProbeService> probeService,
                                 std::shared_ptr<IGraphicsService> graphicsService = nullptr,
                                 std::shared_ptr<IWindowService> windowService = nullptr,
                                 std::shared_ptr<IPlatformService> platformService = nullptr,
                                 std::shared_ptr<IInputService> inputService = nullptr);

    void RegisterUsedSteps(const WorkflowDefinition& workflow,
                           const std::shared_ptr<IWorkflowStepRegistry>& registry) const;

private:
    void RegisterConfigSteps(const std::unordered_set<std::string>& plugins,
                             const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterGraphicsSteps(const std::unordered_set<std::string>& plugins,
                               const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterMathSteps(const std::unordered_set<std::string>& plugins,
                           const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterStringSteps(const std::unordered_set<std::string>& plugins,
                             const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterListSteps(const std::unordered_set<std::string>& plugins,
                           const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterDebugSteps(const std::unordered_set<std::string>& plugins,
                            const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterSystemSteps(const std::unordered_set<std::string>& plugins,
                             const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterStateSteps(const std::unordered_set<std::string>& plugins,
                            const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterControlSteps(const std::unordered_set<std::string>& plugins,
                              const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterInputSteps(const std::unordered_set<std::string>& plugins,
                            const std::shared_ptr<IWorkflowStepRegistry>& registry) const;
    void RegisterInputPollSteps(const std::unordered_set<std::string>& plugins,
                                const std::shared_ptr<IWorkflowStepRegistry>& registry) const;

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IProbeService> probeService_;
    std::shared_ptr<IGraphicsService> graphicsService_;
    std::shared_ptr<IWindowService> windowService_;
    std::shared_ptr<IPlatformService> platformService_;
    std::shared_ptr<IInputService> inputService_;
};

}  // namespace sdl3cpp::services::impl
