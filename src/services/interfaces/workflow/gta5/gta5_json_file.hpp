#pragma once

#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace sdl3cpp::services::impl {

/// Read and parse a JSON file, warning rather than throwing.
///
/// Returns false when the file is missing or malformed so that callers can
/// fall back to documented defaults instead of taking the frame loop down.
bool ReadGta5JsonFile(const std::string& path, nlohmann::json& out,
                      const std::shared_ptr<ILogger>& logger,
                      const std::string& what);

}  // namespace sdl3cpp::services::impl
