#pragma once

#include "services/interfaces/workflow/gta5/gta5_map_frame.hpp"

namespace sdl3cpp::services::impl {

/// The world the six tiles cover, in GTA metres: x east from minX, y
/// south from maxY. The defaults are fitted to GTA's road path nodes.
struct Gta5MapRect {
    float minX{-4140.f};
    float maxY{8400.f};
    float width{9000.f};
    float height{13500.f};
    float U(float x) const { return (x - minX) / width; }
    float V(float y) const { return (maxY - y) / height; }
};

/// The whole overlay: backdrop, tiles, points of interest, legend, the
/// cars at world (x, y) in the YOUR CAR category's icon, and the
/// player's arrow at world (x, y), turned `angle` clockwise from north.
Gta5MapFrame BuildGta5MapFrame(const Gta5MapOverlay& map,
                               const Gta5MapRect& rect, int width,
                               int height, const std::vector<glm::vec2>& cars,
                               glm::vec2 player, float angle);

/// Each category's icon and label in a box at the map's bottom left --
/// open sea, off the south-west coast.
void AddGta5MapLegend(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                      const Gta5MapOverlay& map);

/// Upload `frame` and draw it over the swapchain image, keeping what the
/// frame already drew there.
void DrawGta5MapOverlay(Gta5MapOverlay& map, SDL_GPUDevice* device,
                        SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchain,
                        const Gta5MapFrame& frame);

}  // namespace sdl3cpp::services::impl
