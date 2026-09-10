#include "services/interfaces/workflow/workflow_connection_reader.hpp"
#include "services/interfaces/workflow/workflow_connection_main_parser.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowConnectionReader::WorkflowConnectionReader(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("WorkflowConnectionReader", "Constructor", "Entry");
    }
}

std::vector<std::pair<std::string, std::string>>
WorkflowConnectionReader::ReadConnections(
    const rapidjson::Value& document) const {
    if (logger_) {
        logger_->Trace("WorkflowConnectionReader", "ReadConnections",
                       "Entry");
    }

    if (!document.HasMember("connections")) {
        return {};
    }
    const auto& connectionsValue = document["connections"];
    if (!connectionsValue.IsObject()) {
        throw std::runtime_error("Workflow 'connections' must be an object");
    }

    std::vector<std::pair<std::string, std::string>> edges;
    for (auto it = connectionsValue.MemberBegin();
        it != connectionsValue.MemberEnd(); ++it) {
        const std::string fromNode = it->name.GetString();
        if (!it->value.IsObject()) {
            throw std::runtime_error("Workflow connections for '" +
                                     fromNode + "' must be an object");
        }
        if (!it->value.HasMember("main")) {
            continue;
        }
        ParseMainConnections(it->value["main"], fromNode, edges);
    }
    return edges;
}

}  // namespace sdl3cpp::services::impl
