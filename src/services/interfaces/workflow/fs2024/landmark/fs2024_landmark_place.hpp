#pragma once

#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_model.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace sdl3cpp::services::impl {

/// A landmark model's placement: its own glTF space (+y up, +z its
/// front) to tile space, standing at `at`, turned to FS2024's own
/// heading (degrees clockwise from north) and scaled. glTF is
/// right-handed, so with +z forward its +x is to the left; engine space
/// is x east, z south, so heading 0 -- front to the north -- is a half
/// turn about y.
glm::mat4 Fs2024LandmarkModel(const glm::vec3& at, float headingDegrees,
                              float scale);

/// Which of a model's LODs to load: the most detailed one whose binary
/// glTF fits `budgetBytes`, or the least detailed if none does. FS2024
/// lists them most detailed first.
std::size_t ChooseFs2024LandmarkLod(
    const std::vector<sdl3cpp::fs2024::GltfLodInfo>& lods,
    std::size_t budgetBytes);

}  // namespace sdl3cpp::services::impl
