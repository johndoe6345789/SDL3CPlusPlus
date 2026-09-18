// ParseModelRiff against a small, hand-built RIFF/GLB fixture -- a
// real bug was found here only by looking at a screenshot: FS2024
// packs every primitive of a mesh that shares vertex data into ONE
// glTF "indices" accessor and tells them apart with its own
// `ASOBO_primitive` extra (`StartIndex`/`PrimitiveCount`, in index
// elements/triangles); reading the whole shared accessor for every
// primitive, as plain glTF would, drew every primitive with the
// mesh's entire geometry, so every material overlapped the same full
// shape instead of just its own slice.

#include "services/interfaces/workflow/fs2024/data/gltf/fs2024_gltf_model.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace tools = sdl3cpp::fs2024;

namespace {

void AppendU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    out.insert(out.end(), reinterpret_cast<const std::uint8_t*>(&value),
              reinterpret_cast<const std::uint8_t*>(&value) + 4);
}

void AppendChunk(std::vector<std::uint8_t>& out, const std::string& id,
                 const std::vector<std::uint8_t>& payload) {
    out.insert(out.end(), id.begin(), id.end());
    AppendU32(out, static_cast<std::uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    if (payload.size() & 1) out.push_back(0);
}

/// Four vertices and two triangles sharing one index buffer, exactly
/// as a real FS2024 mesh does: primitive 0 takes indices [0,3),
/// primitive 1 takes [3,6) of the SAME accessor, via their own
/// `ASOBO_primitive` extras rather than each having its own accessor.
std::vector<std::uint8_t> BuildTestModelRiff() {
    const std::string json =
        R"({"asset":{"version":"2.0"},"scene":0,)"
        R"("scenes":[{"nodes":[0]}],"nodes":[{"mesh":0}],)"
        R"("meshes":[{"primitives":[)"
        R"({"attributes":{"POSITION":0},"indices":1,)"
        R"("extras":{"ASOBO_primitive":{"StartIndex":0,"PrimitiveCount":1}}},)"
        R"({"attributes":{"POSITION":0},"indices":1,)"
        R"("extras":{"ASOBO_primitive":{"StartIndex":3,"PrimitiveCount":1}}})"
        R"(]}],)"
        R"("accessors":[)"
        R"({"bufferView":0,"componentType":5126,"count":4,"type":"VEC3"},)"
        R"({"bufferView":1,"componentType":5123,"count":6,"type":"SCALAR"})"
        R"(],)"
        R"("bufferViews":[)"
        R"({"buffer":0,"byteOffset":0,"byteLength":48},)"
        R"({"buffer":0,"byteOffset":48,"byteLength":12})"
        R"(],"buffers":[{"byteLength":60}]})";

    const float positions[4][3] = {
        {0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {1.f, 1.f, 0.f}};
    const std::uint16_t indices[6] = {0, 1, 2, 1, 2, 3};
    std::vector<std::uint8_t> bin(60);
    std::memcpy(bin.data(), positions, 48);
    std::memcpy(bin.data() + 48, indices, 12);

    std::vector<std::uint8_t> jsonBytes(json.begin(), json.end());
    while (jsonBytes.size() % 4 != 0) jsonBytes.push_back(' ');
    while (bin.size() % 4 != 0) bin.push_back(0);

    std::vector<std::uint8_t> glb;
    glb.insert(glb.end(), {'g', 'l', 'T', 'F'});
    AppendU32(glb, 2);
    AppendU32(glb, 0);  // total length patched below
    AppendU32(glb, static_cast<std::uint32_t>(jsonBytes.size()));
    glb.insert(glb.end(), {'J', 'S', 'O', 'N'});
    glb.insert(glb.end(), jsonBytes.begin(), jsonBytes.end());
    AppendU32(glb, static_cast<std::uint32_t>(bin.size()));
    glb.insert(glb.end(), {'B', 'I', 'N', 0});
    glb.insert(glb.end(), bin.begin(), bin.end());
    const auto total = static_cast<std::uint32_t>(glb.size());
    std::memcpy(glb.data() + 8, &total, 4);

    std::vector<std::uint8_t> glbd;
    AppendChunk(glbd, std::string("GLB\0", 4), glb);

    const std::string gxml =
        R"(<ModelInfo name="Test"><LODS>)"
        R"(<LOD ModelFile="Test_LOD0.gltf" minSize="0.0"/>)"
        R"(</LODS></ModelInfo>)";
    std::vector<std::uint8_t> gxmlBytes(gxml.begin(), gxml.end());

    std::vector<std::uint8_t> riff;
    riff.insert(riff.end(), {'R', 'I', 'F', 'F'});
    AppendU32(riff, 0);  // patched below
    riff.insert(riff.end(), {'G', 'L', 'T', 'F'});
    AppendChunk(riff, "GXML", gxmlBytes);
    AppendChunk(riff, "GLBD", glbd);
    const auto riffSize = static_cast<std::uint32_t>(riff.size() - 8);
    std::memcpy(riff.data() + 4, &riffSize, 4);
    return riff;
}

}  // namespace

TEST(Fs2024GltfModel, SlicesEachPrimitivesOwnRangeOfASharedIndexAccessor) {
    const auto lods = tools::ParseModelRiff(BuildTestModelRiff());
    ASSERT_EQ(lods.size(), 1u);
    ASSERT_EQ(lods[0].primitives.size(), 2u);

    // Each primitive gets exactly one triangle's worth of indices, not
    // the whole 6-index shared accessor both reference.
    EXPECT_EQ(lods[0].primitives[0].mesh.indices, std::vector<std::uint32_t>(
        {0u, 1u, 2u}));
    EXPECT_EQ(lods[0].primitives[1].mesh.indices, std::vector<std::uint32_t>(
        {1u, 2u, 3u}));
}

TEST(Fs2024GltfModel, ReadsLodNameAndPositionsFromTheEmbeddedGlb) {
    const auto lods = tools::ParseModelRiff(BuildTestModelRiff());
    ASSERT_EQ(lods.size(), 1u);
    EXPECT_EQ(lods[0].modelFile, "Test_LOD0.gltf");
    ASSERT_EQ(lods[0].primitives[0].mesh.vertices.size(), 4u);
    EXPECT_FLOAT_EQ(lods[0].primitives[0].mesh.vertices[1].x, 1.f);
}
