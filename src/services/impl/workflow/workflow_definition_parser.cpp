#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow/workflow_parameter_reader.hpp"
#include "services/interfaces/config/json_config_document_parser.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowDefinitionParser::WorkflowDefinitionParser(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowDefinitionParser", "Constructor", "Entry");
    }
}

WorkflowDefinition WorkflowDefinitionParser::ParseFile(const std::filesystem::path& path) const {
    if (logger_) {
        logger_->Trace("WorkflowDefinitionParser", "ParseFile", "Entry");
    }
    // Parse JSON document
    json_config::JsonConfigDocumentParser parser;
    rapidjson::Document document = parser.Parse(path, "workflow file");

    // Validate format
    const bool hasSteps = document.HasMember("steps");
    const bool hasNodes = document.HasMember("nodes");
    if (hasSteps && hasNodes) {
        throw std::runtime_error("Workflow cannot define both 'steps' and 'nodes'");
    }
    if (!hasSteps && !hasNodes) {
        throw std::runtime_error("Workflow must contain a 'steps' array or 'nodes' array");
    }

    WorkflowParameterReader paramReader;
    WorkflowDefinition workflow;

    // Read optional template name
    if (document.HasMember("template")) {
        workflow.templateName = paramReader.ReadRequiredString(document, "template");
    }

    // Read workflow variables (n8n-style)
    ParseVariables(document, workflow);

    // Build include-cycle detection set (canonical path of THIS file)
    std::unordered_set<std::string> visited;
    try {
        visited.insert(std::filesystem::canonical(path).string());
    } catch (...) {
        visited.insert(path.string());
    }
    const std::filesystem::path baseDir = path.parent_path();

    // Handle "steps" format (simple sequential)
    if (hasSteps) {
        if (!document["steps"].IsArray()) {
            throw std::runtime_error("Workflow must contain a 'steps' array");
        }
        for (const auto& entry : document["steps"].GetArray()) {
            if (!entry.IsObject()) {
                throw std::runtime_error("Workflow steps must be objects");
            }
            WorkflowStepDefinition step;
            step.id = paramReader.ReadRequiredString(entry, "id");
            step.plugin = paramReader.ReadRequiredString(entry, "plugin");
            step.inputs = paramReader.ReadStringMap(entry, "inputs");
            step.outputs = paramReader.ReadStringMap(entry, "outputs");
            step.parameters = paramReader.ReadParameterMap(entry, "parameters");
            workflow.steps.push_back(std::move(step));
        }
        // Expand workflow.include nodes (parse-time composition, like React imports)
        ResolveIncludes(workflow.steps, baseDir, visited);
        return workflow;
    }

    // Handle "nodes" format (n8n with connections)
    workflow.steps = ParseNodes(document);

    // Expand workflow.include nodes (parse-time composition, like React imports)
    ResolveIncludes(workflow.steps, baseDir, visited);

    return workflow;
}

}  // namespace sdl3cpp::services::impl
