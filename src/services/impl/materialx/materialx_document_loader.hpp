#pragma once

#include "services/interfaces/config_types.hpp"

#include <MaterialXCore/Document.h>

#include <filesystem>

namespace sdl3cpp::services::impl {

class MaterialXDocumentLoader {
public:
    MaterialX::DocumentPtr Load(const std::filesystem::path& documentPath,
                                const MaterialXConfig& config,
                                const std::filesystem::path& scriptDirectory) const;
};

}  // namespace sdl3cpp::services::impl
