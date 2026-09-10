#include "services/interfaces/workflow/scene/scene_uuid.hpp"

#if defined(_WIN32)
#include <rpc.h>
#pragma comment(lib, "rpcrt4.lib")
#else
#include <uuid/uuid.h>
#endif

namespace sdl3cpp::services::impl {

std::string GenerateSceneObjectUuid() {
#if defined(_WIN32)
    UUID uuid_val;
    UuidCreate(&uuid_val);
    RPC_CSTR str = nullptr;
    UuidToStringA(&uuid_val, &str);
    std::string result(reinterpret_cast<char*>(str));
    RpcStringFreeA(&str);
    return result;
#else
    uuid_t uuid_val;
    uuid_generate(uuid_val);
    char uuid_str[37];
    uuid_unparse(uuid_val, uuid_str);
    return std::string(uuid_str);
#endif
}

}  // namespace sdl3cpp::services::impl
