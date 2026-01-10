#pragma once

#include "services/interfaces/config_types.hpp"
#include "services/interfaces/graphics_types.hpp"
#include "services/interfaces/i_logger.hpp"
#include <filesystem>
#include <memory>

namespace sdl3cpp::services::impl {

class MaterialXShaderGenerator {
public:
    MaterialXShaderGenerator(std::shared_ptr<ILogger> logger);

    ShaderPaths Generate(const MaterialXConfig& config,
                         const std::filesystem::path& scriptDirectory) const;

private:
    std::filesystem::path ResolvePath(const std::filesystem::path& path,
                                      const std::filesystem::path& scriptDirectory) const;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
