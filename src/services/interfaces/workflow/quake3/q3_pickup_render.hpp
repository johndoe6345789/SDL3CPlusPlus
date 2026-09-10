#pragma once

// Umbrella header: pickup rendering was split into focused pieces (quad
// buffers, cached color textures, per-entity draw, and the entity-list
// orchestrator). Kept so existing includers don't need to enumerate the
// individual pieces.
#include "services/interfaces/workflow/quake3/q3_pickup_color_texture.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_entities_draw.hpp"
#include "services/interfaces/workflow/quake3/q3_pickup_quad_buffers.hpp"
