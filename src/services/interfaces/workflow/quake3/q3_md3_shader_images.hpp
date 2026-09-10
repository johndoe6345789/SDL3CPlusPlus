#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <map>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief The pk3's shader-name to image map, parsed once per session.
 *
 * An MD3 surface names a shader, not an image. Most of id's models name
 * one that happens to share its name with a file, but the item models do
 * not: a health cross asks for models/powerups/health/yellow, which only
 * exists as a script defining an environment map. Without the scripts
 * every cross, armour and ammo box falls back to flat grey.
 *
 * The world faces already resolve their shaders this way; this shares the
 * same parse with the MD3 loaders, caching it in @p context because
 * scanning every scripts/*.shader entry once per surface would re-read
 * the whole archive thirty times over.
 */
const std::map<std::string, std::string>& Q3Md3ShaderImages(
    const std::string& pk3Path, WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
