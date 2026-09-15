#pragma once

#include <stdint.h>

namespace sdl3cpp::q3 {

/// MD3 stores vertex coordinates as int16 in 1/64th Q3 world units.
inline constexpr float kMd3XyzScale = 1.0f / 64.0f;

/// Q3 world unit -> engine unit, matching the bsp.load step's scale.
inline constexpr float kMd3WorldScale = 0.03125f;

#pragma pack(push, 1)

struct Md3Header {
    int32_t ident, version;
    char name[64];
    int32_t flags, numFrames, numTags, numSurfaces, numSkins;
    int32_t ofsFrames, ofsTags, ofsSurfaces, ofsEnd;
};

struct Md3Tag {
    char name[64];
    float origin[3];
    /// Row-major rotation: axis[0]=right, axis[1]=forward, axis[2]=up (Q3
    /// space).
    float axis[3][3];
};

struct Md3Surface {
    int32_t ident;
    char name[64];
    int32_t flags, numFrames, numShaders, numVerts, numTriangles;
    int32_t ofsTriangles, ofsShaders, ofsSt, ofsXyzNormals, ofsEnd;
};

struct Md3Triangle {
    int32_t indexes[3];
};

struct Md3Shader {
    char name[64];
    int32_t shaderIndex;
};

struct Md3St {
    float st[2];
};

struct Md3XyzNormal {
    int16_t xyz[3];
    int16_t normal;
};

#pragma pack(pop)

/// Vertex layout matching the "position_uv" pipeline format.
struct Md3PosUv {
    float x, y, z, u, v;
};

}  // namespace sdl3cpp::q3
