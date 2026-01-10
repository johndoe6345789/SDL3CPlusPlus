#pragma once

#include "services/interfaces/config_types.hpp"

#include <MaterialXFormat/Util.h>

#include <filesystem>

namespace sdl3cpp::services::impl {

class MaterialXSearchPathBuilder {
public:
    MaterialX::FileSearchPath Build(const MaterialXConfig& config,
                                    const std::filesystem::path& scriptDirectory) const;
};

}  // namespace sdl3cpp::services::impl
