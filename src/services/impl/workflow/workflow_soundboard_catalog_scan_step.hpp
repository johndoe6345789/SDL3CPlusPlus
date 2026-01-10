#pragma once

#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/soundboard_types.hpp"

#include <memory>
#include <optional>

namespace sdl3cpp::services::impl {

class WorkflowSoundboardCatalogScanStep final : public IWorkflowStep {
public:
    WorkflowSoundboardCatalogScanStep(std::shared_ptr<IConfigService> configService,
                                      std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    SoundboardCatalog LoadCatalog() const;

    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
    std::optional<SoundboardCatalog> cachedCatalog_;
};

}  // namespace sdl3cpp::services::impl
