#include "services/interfaces/workflow/gta5/gta5_json_file.hpp"

#include <fstream>

namespace sdl3cpp::services::impl {

bool ReadGta5JsonFile(const std::string& path, nlohmann::json& out,
                      const std::shared_ptr<ILogger>& logger,
                      const std::string& what) {
    std::ifstream file(path);
    if (!file.is_open()) {
        if (logger) {
            logger->Warn(what + ": could not open '" + path +
                         "', continuing with defaults");
        }
        return false;
    }
    try {
        file >> out;
    } catch (const nlohmann::json::exception& ex) {
        if (logger) {
            logger->Warn(what + ": malformed JSON in '" + path +
                         "': " + ex.what() + ", continuing with defaults");
        }
        return false;
    }
    return true;
}

}  // namespace sdl3cpp::services::impl
