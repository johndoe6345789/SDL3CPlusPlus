#include "services/interfaces/workflow/workflow_definition_parser.hpp"
#include "services/interfaces/workflow/workflow_node_parse_helpers.hpp"
#include "services/interfaces/workflow/workflow_parameter_reader.hpp"

#include <rapidjson/document.h>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

std::vector<WorkflowStepDefinition> WorkflowDefinitionParser::ParseNodes(
    const rapidjson::Document& document) const {
    if (logger_) {
        logger_->Trace("WorkflowDefinitionParser", "ParseNodes", "Entry");
    }

    if (!document["nodes"].IsArray()) {
        throw std::runtime_error("Workflow must contain a 'nodes' array");
    }

    WorkflowParameterReader paramReader;
    std::vector<WorkflowStepDefinition> nodes;
    std::vector<std::string> nodeOrder;
    // n8n uses names in connections, so track name->id alongside parse order.
    std::unordered_map<std::string, std::string> nameToId;

    for (rapidjson::SizeType i = 0; i < document["nodes"].Size(); ++i) {
        const auto& entry = document["nodes"][i];
        if (!entry.IsObject()) {
            throw std::runtime_error("Workflow nodes must be objects");
        }
        WorkflowStepDefinition step =
            ParseOneWorkflowNode(paramReader, entry, i, nameToId);
        nodeOrder.push_back(step.id);
        nodes.push_back(std::move(step));
    }

    return SortWorkflowNodes(document, nodes, nodeOrder, nameToId);
}

}  // namespace sdl3cpp::services::impl
