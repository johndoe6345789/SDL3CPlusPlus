#include "mesh_service.hpp"
#include <utility>

#include "../../script/mesh_loader.hpp"

namespace sdl3cpp::services::impl {

MeshService::MeshService(std::shared_ptr<IScriptEngineService> engineService)
    : engineService_(std::move(engineService)) {
}

bool MeshService::LoadFromFile(const std::string& requestedPath,
                               script::MeshPayload& outPayload,
                               std::string& outError) {
    return script::MeshLoader::LoadFromFile(engineService_->GetScriptDirectory(),
                                            requestedPath,
                                            outPayload,
                                            outError);
}

void MeshService::PushMeshToLua(lua_State* L, const script::MeshPayload& payload) {
    script::MeshLoader::PushMeshToLua(L, payload);
}

}  // namespace sdl3cpp::services::impl
