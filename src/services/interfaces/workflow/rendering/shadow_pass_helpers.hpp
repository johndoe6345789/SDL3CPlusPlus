#pragma once

/// Umbrella header for shadow.pass's helpers, split across
/// shadow_face_rotations.hpp (the shared per-face rotation matrices) and
/// shadow_caster_draw.hpp (drawing one shadow-casting box), each kept
/// under the 80-line cap.
#include "services/interfaces/workflow/rendering/shadow_caster_draw.hpp"
#include "services/interfaces/workflow/rendering/shadow_face_rotations.hpp"
