#include "services/interfaces/workflow/workflow_config_pipeline.hpp"

#include "services/interfaces/workflow/workflow_default_step_registrar.hpp"
#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow/workflow_executor.hpp"
#include "services/interfaces/workflow/workflow_step_registry.hpp"
#include "services/interfaces/workflow/workflow_template_resolver.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <stdexcept>
#include <string>
#include <utility>
namespace sdl3cpp::services::impl {

WorkflowConfigPipeline::WorkflowConfigPipeline(std::shared_ptr<ILogger> logger,
                                               std::shared_ptr<IProbeService> probeService)
    : logger_(std::move(logger)),
      probeService_(std::move(probeService)) {}

WorkflowResult WorkflowConfigPipeline::Execute(const std::filesystem::path& configPath,
                                               std::optional<int>* versionOut) const {
    if (logger_) {
        logger_->Trace("WorkflowConfigPipeline", "Execute",
                       "configPath=" + configPath.string(),
                       "Starting boot workflow");
    }

    WorkflowTemplateResolver resolver;
    const std::filesystem::path templatePath = resolver.ResolveBootTemplate(configPath);
    if (templatePath.empty()) {
        throw std::runtime_error("WorkflowConfigPipeline: boot workflow template not found for " +
                                 configPath.string());
    }

    WorkflowDefinitionParser parser;
    const WorkflowDefinition workflow = parser.ParseFile(templatePath);

    auto registry = std::make_shared<WorkflowStepRegistry>();
    WorkflowDefaultStepRegistrar registrar(logger_, probeService_);
    registrar.RegisterUsedSteps(workflow, registry);

    WorkflowExecutor executor(registry, logger_);
    WorkflowContext context;
    context.Set("config.path", configPath);
    executor.Execute(workflow, context);

    const auto* documentHandle = context.TryGet<std::shared_ptr<rapidjson::Document>>("config.document");
    if (!documentHandle || !(*documentHandle)) {
        throw std::runtime_error("WorkflowConfigPipeline: boot workflow did not provide config.document");
    }

    if (logger_) {
        logger_->Trace("WorkflowConfigPipeline", "Execute",
                       "templatePath=" + templatePath.string(),
                       "Boot workflow complete");
    }

    WorkflowResult result;
    result.document = *documentHandle;
    if (versionOut) {
        const auto* versionHandle = context.TryGet<std::optional<int>>("config.version");
        *versionOut = versionHandle ? *versionHandle : std::nullopt;
    }
    result.context = std::move(context);
    return result;
}

}  // namespace sdl3cpp::services::impl
