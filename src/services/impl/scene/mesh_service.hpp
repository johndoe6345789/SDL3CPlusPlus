#pragma once

#include "services/interfaces/i_mesh_service.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include <filesystem>
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Mesh loading service implementation.
 */
class MeshService : public IMeshService {
public:
    MeshService(std::shared_ptr<IConfigService> configService,
                std::shared_ptr<ILogger> logger);

    bool LoadFromFile(const std::string& requestedPath,
                      MeshPayload& outPayload,
                      std::string& outError) override;
    bool LoadFromArchive(const std::string& archivePath,
                         const std::string& entryPath,
                         MeshPayload& outPayload,
                         std::string& outError) override;

private:
    bool ResolvePath(const std::string& requestedPath,
                     std::filesystem::path& resolvedPath,
                     std::string& outError) const;

    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
