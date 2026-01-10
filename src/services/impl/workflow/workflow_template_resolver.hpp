#pragma once

#include <filesystem>

namespace sdl3cpp::services::impl {

class WorkflowTemplateResolver {
public:
    std::filesystem::path ResolveBootTemplate(const std::filesystem::path& configPath) const;
};

}  // namespace sdl3cpp::services::impl
