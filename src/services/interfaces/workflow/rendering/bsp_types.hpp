#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

// Q3 BSP file format structures
#pragma pack(push, 1)

struct BspHeader {
    char magic[4];    // "IBSP"
    int32_t version;  // 0x2E (46) for Q3
};

struct BspLump {
    int32_t offset;
    int32_t length;
};

struct BspVertex {
    float position[3];
    float texcoord[2][2];  // [0]=surface, [1]=lightmap
    float normal[3];
    uint8_t color[4];
};

struct BspFace {
    int32_t texture;
    int32_t effect;
    int32_t type;          // 1=polygon, 2=patch, 3=mesh, 4=billboard
    int32_t vertex;
    int32_t n_vertices;
    int32_t meshvert;
    int32_t n_meshverts;
    int32_t lm_index;
    int32_t lm_start[2];
    int32_t lm_size[2];
    float lm_origin[3];
    float lm_vecs[2][3];
    float normal[3];
    int32_t size[2];       // patch dimensions
};

struct BspTexture {
    char name[64];
    int32_t flags;
    int32_t contents;
};

struct BspBrush {
    int32_t firstSide;
    int32_t numSides;
    int32_t shaderIndex;
};

struct BspBrushSide {
    int32_t planeIndex;
    int32_t shaderIndex;
};

struct BspPlane {
    float normal[3];
    float dist;
};

struct BspModel {
    float mins[3];
    float maxs[3];
    int32_t firstFace;
    int32_t numFaces;
    int32_t firstBrush;
    int32_t numBrushes;
};

#pragma pack(pop)

// Lump indices
enum {
    LUMP_ENTITIES = 0,
    LUMP_TEXTURES = 1,
    LUMP_PLANES = 2,
    LUMP_NODES = 3,
    LUMP_LEAFS = 4,
    LUMP_LEAFFACES = 5,
    LUMP_LEAFBRUSHES = 6,
    LUMP_MODELS = 7,
    LUMP_BRUSHES = 8,
    LUMP_BRUSHSIDES = 9,
    LUMP_VERTICES = 10,
    LUMP_MESHVERTS = 11,
    LUMP_EFFECTS = 12,
    LUMP_FACES = 13,
    LUMP_LIGHTMAPS = 14,
    LUMP_LIGHTVOLS = 15,
    LUMP_VISDATA = 16,
    NUM_LUMPS = 17
};

// Content flags
static constexpr int32_t CONTENTS_SOLID = 0x1;
// Lightmap block dimensions in Q3 BSP
static constexpr int LM_BLOCK_SIZE = 128;

// 40-byte render vertex: pos(12) + uv(8) + lm_uv(8) + normal(12)
struct BspRenderVertex {
    float x, y, z;
    float u, v;
    float lm_u, lm_v;
    float nx, ny, nz;
};

// Per-texture geometry group used between build_geometry and upload/extract steps
struct TextureGroup {
    std::vector<BspRenderVertex> vertices;
    std::vector<uint32_t> indices;
};

}  // namespace sdl3cpp::services::impl
