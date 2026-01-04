#pragma once

#include "../interfaces/i_mesh_service.hpp"
#include "../interfaces/i_script_engine_service.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Script-facing mesh loading service implementation.
 */
class MeshService : public IMeshService {
public:
    explicit MeshService(std::shared_ptr<IScriptEngineService> engineService);

    bool LoadFromFile(const std::string& requestedPath,
                      script::MeshPayload& outPayload,
                      std::string& outError) override;
    void PushMeshToLua(lua_State* L, const script::MeshPayload& payload) override;

private:
    std::shared_ptr<IScriptEngineService> engineService_;
};

}  // namespace sdl3cpp::services::impl
