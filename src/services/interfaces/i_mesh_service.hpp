#pragma once

#include "../../script/mesh_payload.hpp"
#include <string>

struct lua_State;

namespace sdl3cpp::services {

/**
 * @brief Script-facing mesh loading service interface.
 */
class IMeshService {
public:
    virtual ~IMeshService() = default;

    virtual bool LoadFromFile(const std::string& requestedPath,
                              script::MeshPayload& outPayload,
                              std::string& outError) = 0;
    virtual void PushMeshToLua(lua_State* L, const script::MeshPayload& payload) = 0;
};

}  // namespace sdl3cpp::services
