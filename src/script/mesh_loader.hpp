#ifndef SDL3CPP_SCRIPT_MESH_LOADER_HPP
#define SDL3CPP_SCRIPT_MESH_LOADER_HPP

#include <array>
#include <filesystem>
#include <string>
#include <vector>

struct lua_State;

namespace sdl3cpp::script {

struct MeshPayload {
    std::vector<std::array<float, 3>> positions;
    std::vector<std::array<float, 3>> colors;
    std::vector<uint32_t> indices;
};

class MeshLoader {
public:
    static bool LoadFromFile(const std::filesystem::path& scriptDirectory,
                            const std::string& requestedPath,
                            MeshPayload& outPayload,
                            std::string& outError);

    static int PushMeshToLua(lua_State* L, const MeshPayload& payload);
};

} // namespace sdl3cpp::script

#endif // SDL3CPP_SCRIPT_MESH_LOADER_HPP
