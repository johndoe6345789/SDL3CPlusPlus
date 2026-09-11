#include "services/interfaces/workflow/gta5/gta5_map_build.hpp"

#include "services/interfaces/workflow/gta5/gta5_map_art.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void AddGta5MapLegend(Gta5MapFrame& frame, const Gta5MapLayout& l,
                      const Gta5MapOverlay& map) {
    const auto& categories = map.pois.categories;
    if (categories.empty()) return;
    const float scale = l.tile > 380.f ? 3.f : 2.f;  // pixels to a dot
    const float line = 10.f * scale;  // one row
    const float icon = 7.f * scale;   // as tall as a capital
    const float pad = 3.f * scale;
    std::size_t longest = 0;
    for (const auto& c : categories) {
        longest = std::max(longest, c.label.size());
    }
    const auto rows = static_cast<float>(categories.size());
    const glm::vec2 size(
        pad * 3.f + icon + 6.f * scale * static_cast<float>(longest),
        pad * 2.f + line * rows - (line - icon));
    const glm::vec2 corner(l.left + 2.f * l.tile - size.x - 2.f * pad,
                           l.top + 3.f * l.tile - size.y - 2.f * pad);
    AddGta5MapRect(frame, l, map.shade, map.sampler, corner, corner + size);
    // Icons, then labels: two texture runs, not one per row.
    for (std::size_t i = 0; i < categories.size(); ++i) {
        const glm::vec2 at =
            corner + glm::vec2(pad, pad + line * static_cast<float>(i));
        AddGta5MapRect(frame, l, map.atlas, map.sampler, at, at + icon,
                       Gta5MapIconUv(static_cast<int>(i)));
    }
    for (std::size_t i = 0; i < categories.size(); ++i) {
        const float y = pad + line * static_cast<float>(i);
        AddGta5MapText(frame, l, map, corner + glm::vec2(2.f * pad + icon, y),
                       scale, categories[i].label);
    }
}

}  // namespace sdl3cpp::services::impl
