#pragma once

#include "services/interfaces/i_logger.hpp"

#include <rapidjson/document.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sdl3cpp::services::impl {

/// Small focused class: parses a workflow document's `connections` object
/// into a flat edge list. Single job: understand n8n's connections format
/// (object-of-branches or simple array) so callers just get (from, to)
/// pairs using node names.
class WorkflowConnectionReader {
public:
    explicit WorkflowConnectionReader(
        std::shared_ptr<ILogger> logger = nullptr);

    /// Read connections from workflow JSON (n8n format).
    /// Returns edges as (from, to) pairs using node names.
    std::vector<std::pair<std::string, std::string>> ReadConnections(
        const rapidjson::Value& document) const;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
