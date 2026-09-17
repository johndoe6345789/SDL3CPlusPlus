#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// A regular grid of ground heights, as python/fs2024/bake_terrain.py
/// writes it to terrain.fst. Engine space: metres, Y up, x east, z south.
/// Column c lies at x = origin.x + c * spacing and row r at
/// z = origin.y + r * spacing, so row 0 is the northern edge.
struct Fs2024Heightfield {
    int columns = 0;
    int rows = 0;
    float spacing = 1.f;
    glm::vec2 origin{0.f};
    std::vector<float> heights;  ///< row-major, rows * columns
    float minHeight = 0.f;
    float maxHeight = 0.f;

    float At(int column, int row) const {
        const auto index = static_cast<std::size_t>(row) *
                               static_cast<std::size_t>(columns) +
                           static_cast<std::size_t>(column);
        return heights[index];
    }

    glm::vec3 Position(int column, int row) const {
        return {origin.x + static_cast<float>(column) * spacing,
                At(column, row),
                origin.y + static_cast<float>(row) * spacing};
    }
};

/// Read terrain.fst. Throws std::runtime_error naming the file when it
/// is missing, has the wrong magic, or is shorter than its header says.
Fs2024Heightfield ReadFs2024Heightfield(const std::string& path);

/// Ground height under engine (x, z), on the same triangles the mesh and
/// the collision shape use. Positions off the grid clamp to its edge.
float Fs2024HeightAt(const Fs2024Heightfield& field, float x, float z);

}  // namespace sdl3cpp::services::impl
