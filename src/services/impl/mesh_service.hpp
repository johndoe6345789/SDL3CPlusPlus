#pragma once

#include "../interfaces/i_mesh_service.hpp"
#include "../interfaces/i_config_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing mesh loading service implementation.
 */
class MeshService : public IMeshService {
public:
    MeshService(std::shared_ptr<IConfigService> configService,
                std::shared_ptr<ILogger> logger);

    bool LoadFromFile(const std::string& requestedPath,
                      MeshPayload& outPayload,
                      std::string& outError) override;
    void PushMeshToLua(lua_State* L, const MeshPayload& payload) override;

private:
    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl
