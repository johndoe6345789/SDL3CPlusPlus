#pragma once

/// Umbrella header for bsp.portal_view's helpers, split across
/// bsp_portal_destination.hpp, bsp_portal_view_targets.hpp,
/// bsp_portal_view_uniforms.hpp, and bsp_portal_view_draw.hpp (each kept
/// under the 80-line cap). Include this for convenience, or include only
/// the specific split header(s) a new caller actually needs.
#include "services/interfaces/workflow/rendering/bsp_portal_destination.hpp"
#include "services/interfaces/workflow/rendering/bsp_portal_view_draw.hpp"
#include "services/interfaces/workflow/rendering/bsp_portal_view_targets.hpp"
#include "services/interfaces/workflow/rendering/bsp_portal_view_uniforms.hpp"
