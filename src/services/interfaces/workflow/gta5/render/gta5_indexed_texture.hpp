#pragma once

#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/render/gta5_texture_upload.hpp"

#include <SDL3/SDL_gpu.h>

#include <cstdint>
#include <string>

namespace sdl3cpp::services::impl {

/// The texture cache key of a texture read from the map, kept apart from
/// the file paths the legacy glTF path uses.
std::string Gta5TextureKey(std::uint32_t nameHash);

/// Upload a blob a worker read and cache it under its key. An empty blob
/// is cached as unusable, which still settles it for geometry waiting on
/// it. Whatever a synchronous load already put there is kept.
const Gta5Texture* InstallGta5TextureBlob(Gta5StreamState& state,
                                          const Gta5TextureBlob& blob,
                                          SDL_GPUDevice* device);

/// A texture by name hash, read and uploaded here and now if it is not
/// cached yet -- the synchronous path, which vehicles use. Remembered as
/// missing when no dictionary has it.
const Gta5Texture* GetOrLoadGta5IndexedTexture(Gta5StreamState& state,
                                               std::uint32_t nameHash,
                                               SDL_GPUDevice* device);

}  // namespace sdl3cpp::services::impl
