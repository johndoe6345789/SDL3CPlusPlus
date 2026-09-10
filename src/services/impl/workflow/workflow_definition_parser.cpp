#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow/workflow_parameter_reader.hpp"
#include "services/interfaces/config/json_config_document_parser.hpp"

#include <rapidjson/document.h>

#include <filesystem>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowDefinitionParser::WorkflowDefinitionParser(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowDefinitionParser", "Constructor", "Entry");
    }
}

WorkflowDefinition WorkflowDefinitionParser::ParseFile(
    const std::filesystem::path& path) const {
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
        throw std::runtime_error(
            "Workflow cannot define both 'steps' and 'nodes'");
    }
    if (!hasSteps && !hasNodes) {
        throw std::runtime_error(
            "Workflow must contain a 'steps' array or 'nodes' array");
    }

    WorkflowParameterReader paramReader;
    WorkflowDefinition workflow;

    // Read optional template name
    if (document.HasMember("template")) {
        workflow.templateName =
            paramReader.ReadRequiredString(document, "template");
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

    // "steps" is the simple sequential format; "nodes" is n8n-style with
    // connections. Either way, expand workflow.include nodes afterward
    // (parse-time composition, like React imports).
    workflow.steps = hasSteps ? ParseStepsFormat(document, paramReader)
                              : ParseNodes(document);
    ResolveIncludes(workflow.steps, baseDir, visited);
    return workflow;
}

}  // namespace sdl3cpp::services::impl
