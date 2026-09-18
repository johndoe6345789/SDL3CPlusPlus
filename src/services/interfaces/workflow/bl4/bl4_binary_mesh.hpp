#pragma once

#include "services/interfaces/workflow/bl4/bl4_mesh_extract.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/// Reads bl4x's baked binary mesh ("BL4M", see bl4x's pkg/mesh_binary.hpp):
/// the vertices are already in this engine's 40-byte layout, so loading is
/// a read instead of an assimp parse of OBJ text -- which is what made
/// streaming the full map slow.
///
/// Returns an empty Bl4MeshData if the file is missing or not a BL4M file,
/// so the caller can fall back to the .obj beside it.
Bl4MeshData LoadBl4BinaryMesh(const std::string& path);

}  // namespace sdl3cpp::services::impl
