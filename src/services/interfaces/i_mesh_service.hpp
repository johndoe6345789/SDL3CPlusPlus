#pragma once

#include "mesh_types.hpp"
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
                              MeshPayload& outPayload,
                              std::string& outError) = 0;
    virtual bool LoadFromArchive(const std::string& archivePath,
                                 const std::string& entryPath,
                                 MeshPayload& outPayload,
                                 std::string& outError) = 0;
    virtual void PushMeshToLua(lua_State* L, const MeshPayload& payload) = 0;
};

}  // namespace sdl3cpp::services
