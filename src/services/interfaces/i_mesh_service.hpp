#pragma once

#include "mesh_types.hpp"
#include <string>

namespace sdl3cpp::services {

/**
 * @brief Mesh loading service interface.
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
};

}  // namespace sdl3cpp::services
