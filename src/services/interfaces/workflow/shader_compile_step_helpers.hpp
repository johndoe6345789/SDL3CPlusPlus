#pragma once

#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_shader_system_registry.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/// Writes `content` to `path`, swallowing any exception (a best-effort
/// debug marker; shader.compile has always tolerated a read-only or
/// missing `test_outputs/` directory).
void WriteShaderCompileDebugMarker(const std::string& path,
                                   const std::string& content);

/**
 * @brief Builds the shader map from `shaderRegistry`'s active system,
 * optionally loads it to the GPU via `graphicsService`, and stores the
 * result in `context` under the `shader.*` keys shader.compile has always
 * used.
 *
 * On success sets `shader.compiled_count`, `shader.keys`, and
 * `shader.compile_status` = "success". A GPU load failure is logged but
 * does not fail the step (shaders are still compiled). On a build failure,
 * sets `shader.compiled_count` = 0, empty `shader.keys`,
 * `shader.compile_status` = "failed", and `shader.error_message`.
 *
 * Assumes `shaderRegistry` is non-null (the caller checks that first).
 */
void CompileShadersToContext(
    const std::shared_ptr<IShaderSystemRegistry>& shaderRegistry,
    const std::shared_ptr<IGraphicsService>& graphicsService,
    const std::shared_ptr<ILogger>& logger, WorkflowContext& context);

}  // namespace sdl3cpp::services::impl
