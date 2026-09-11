#pragma once

#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"

namespace sdl3cpp::services::impl {

/// Draw one instance, one call per material. Returns the draw count.
///
/// `boundTexture` carries the currently bound texture across calls so a
/// district sharing one texture does not rebind per submesh.
int DrawGta5Instance(const Gta5Instance& instance,
                     const Gta5DrawContext& draw,
                     SDL_GPUTexture*& boundTexture);

}  // namespace sdl3cpp::services::impl
