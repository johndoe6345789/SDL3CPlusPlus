#pragma once

#include "services/interfaces/workflow/stunts/data/stunts_car.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_install.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_material.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_shape.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_tile_table.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_track.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_track_mesh.hpp"

#include <SDL3/SDL_gpu.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One uploaded mesh and the indices to draw it with.
struct StuntsGpuMesh {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    std::uint32_t indexCount = 0;
};

/// Where the car starts and which way it faces, taken from the grid.
struct StuntsStartLine {
    glm::vec3 position{0.f};
    float heading = 0.f;   ///< Radians, 0 = +X, turning toward +Z.
    bool found = false;
};

/**
 * @brief Everything the stunts.* steps share for one loaded track.
 *
 * Held by the registrar rather than the workflow context: the meshes
 * own GPU buffers whose lifetime spans the whole session, and the
 * drawing steps read them every frame.
 */
struct StuntsWorldState {
    StuntsInstall install;
    StuntsTileTable table;
    StuntsTrack track;
    StuntsCar car;
    StuntsMeshParams params;
    StuntsStartLine start;
    StuntsGpuMesh road;
    StuntsGpuMesh ground;
    int roadTiles = 0;
    bool loaded = false;

    // The car's own real body: its highest-detail shape (carBody),
    // meshed once as two draw calls -- the flat-shaded panels
    // (carPanels) and the wheels (carWheels, generated discs rather
    // than raw polygons; see stunts_wheel_mesh.hpp).
    StuntsShape carBody;
    StuntsGpuMesh carPanels;
    StuntsGpuMesh carWheels;
    StuntsMaterialTable materials;
    bool carBodyLoaded = false;
};

}  // namespace sdl3cpp::services::impl
