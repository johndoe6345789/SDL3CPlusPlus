#include "services/interfaces/workflow/gta5/gta5_map_build.hpp"

#include "services/interfaces/workflow/gta5/gta5_map_art.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

Gta5MapFrame BuildGta5MapFrame(const Gta5MapOverlay& map,
                               const Gta5MapRect& rect, int width,
                               int height, glm::vec2 player, float angle) {
    Gta5MapFrame frame;
    frame.vertices.reserve(30 * (64 + map.pois.points.size()));
    const Gta5MapLayout l = FitGta5Map(width, height);
    AddGta5MapRect(frame, l, map.shade, map.sampler, glm::vec2(0.f),
                   glm::vec2(l.width, l.height));
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 2; ++column) {
            const glm::vec2 at(l.left + column * l.tile, l.top + row * l.tile);
            AddGta5MapRect(frame, l, map.tiles[row * 2 + column], map.sampler,
                           at, at + glm::vec2(l.tile));
        }
    }
    // Icons grow with the map, within reason.
    const float half = std::clamp(l.tile / 40.f, 7.f, 12.f);
    for (const Gta5MapPoi& poi : map.pois.points) {
        const glm::vec2 at = l.At(rect.U(poi.x), rect.V(poi.y));
        AddGta5MapRect(frame, l, map.atlas, map.sampler, at - half,
                       at + half, Gta5MapIconUv(poi.category));
    }
    AddGta5MapLegend(frame, l, map);
    // Last, so the player is never under an icon.
    const glm::vec2 me = l.At(std::clamp(rect.U(player.x), 0.f, 1.f),
                              std::clamp(rect.V(player.y), 0.f, 1.f));
    AddGta5MapTurned(frame, l, map.marker, map.sampler, me, 14.f, angle);
    return frame;
}

}  // namespace sdl3cpp::services::impl
