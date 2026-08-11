#include "services/interfaces/workflow/quake3/workflow_q3_md3_load_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <zip.h>

#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include <stdint.h>

namespace sdl3cpp::services::impl {

namespace {

// MD3 XYZ scale: Q3 stores vertices as int16 * (1/64) = Q3 world units
constexpr float kMd3XyzScale = 1.0f / 64.0f;

// BSP world scale: 1 Q3 world unit → 0.03125 engine units (matches bsp.load scale=0.03125)
// Combined scale: raw_xyz * kMd3XyzScale * kMd3WorldScale → engine units
constexpr float kMd3WorldScale = 0.03125f;

#pragma pack(push, 1)
struct Md3Header {
    int32_t ident, version;
    char    name[64];
    int32_t flags, numFrames, numTags, numSurfaces, numSkins;
    int32_t ofsFrames, ofsTags, ofsSurfaces, ofsEnd;
};
struct Md3Tag {
    char  name[64];
    float origin[3];
    float axis[3][3];  // row-major: axis[0]=right, axis[1]=fwd, axis[2]=up in Q3 space
};
struct Md3Surface {
    int32_t ident;
    char    name[64];
    int32_t flags, numFrames, numShaders, numVerts, numTriangles;
    int32_t ofsTriangles, ofsShaders, ofsSt, ofsXyzNormals, ofsEnd;
};
struct Md3Triangle { int32_t indexes[3]; };
struct Md3Shader   { char name[64]; int32_t shaderIndex; };
struct Md3St       { float st[2]; };
struct Md3XyzNormal { int16_t xyz[3]; int16_t normal; };
#pragma pack(pop)

// ── PK3 helpers ──────────────────────────────────────────────────────────────

static std::vector<uint8_t> ReadFromPk3(const std::string& pk3, const std::string& entry) {
    int err = 0;
    zip_t* arc = zip_open(pk3.c_str(), ZIP_RDONLY, &err);
    if (!arc) return {};
    zip_stat_t st;
    if (zip_stat(arc, entry.c_str(), 0, &st) != 0) { zip_close(arc); return {}; }
    std::vector<uint8_t> buf(st.size);
    zip_file_t* zf = zip_fopen(arc, entry.c_str(), 0);
    if (!zf) { zip_close(arc); return {}; }
    zip_fread(zf, buf.data(), st.size);
    zip_fclose(zf);
    zip_close(arc);
    return buf;
}

// ── GPU upload helpers ───────────────────────────────────────────────────────

static SDL_GPUBuffer* UploadBuffer(SDL_GPUDevice* dev, SDL_GPUBufferUsageFlags usage,
                                   const void* data, uint32_t size) {
    SDL_GPUBufferCreateInfo bi = {}; bi.usage = usage; bi.size = size;
    auto* buf = SDL_CreateGPUBuffer(dev, &bi);
    if (!buf) return nullptr;
    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD; tbi.size = size;
    auto* tb = SDL_CreateGPUTransferBuffer(dev, &tbi);
    std::memcpy(SDL_MapGPUTransferBuffer(dev, tb, false), data, size);
    SDL_UnmapGPUTransferBuffer(dev, tb);
    auto* cmd = SDL_AcquireGPUCommandBuffer(dev);
    auto* cp  = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src = {tb, 0};
    SDL_GPUBufferRegion dst = {buf, 0, size};
    SDL_UploadToGPUBuffer(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp); SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(dev, tb);
    return buf;
}

static SDL_GPUTexture* UploadTexture(SDL_GPUDevice* dev, const uint8_t* px, int w, int h) {
    SDL_GPUTextureCreateInfo ti = {};
    ti.type = SDL_GPU_TEXTURETYPE_2D;
    ti.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width = (uint32_t)w; ti.height = (uint32_t)h;
    ti.layer_count_or_depth = 1; ti.num_levels = 1;
    ti.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    auto* tex = SDL_CreateGPUTexture(dev, &ti);
    if (!tex) return nullptr;
    uint32_t bytes = (uint32_t)(w * h * 4);
    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD; tbi.size = bytes;
    auto* tb = SDL_CreateGPUTransferBuffer(dev, &tbi);
    std::memcpy(SDL_MapGPUTransferBuffer(dev, tb, false), px, bytes);
    SDL_UnmapGPUTransferBuffer(dev, tb);
    auto* cmd = SDL_AcquireGPUCommandBuffer(dev);
    auto* cp  = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo src = {}; src.transfer_buffer = tb;
    src.pixels_per_row = (uint32_t)w; src.rows_per_layer = (uint32_t)h;
    SDL_GPUTextureRegion dst = {};
    dst.texture = tex; dst.w = (uint32_t)w; dst.h = (uint32_t)h; dst.d = 1;
    SDL_UploadToGPUTexture(cp, &src, &dst, false);
    SDL_EndGPUCopyPass(cp); SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(dev, tb);
    return tex;
}

static SDL_GPUSampler* MakeLinearSampler(SDL_GPUDevice* dev) {
    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter = SDL_GPU_FILTER_LINEAR; si.mag_filter = SDL_GPU_FILTER_LINEAR;
    si.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    si.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    return SDL_CreateGPUSampler(dev, &si);
}

static SDL_GPUTexture* TryLoadTexture(SDL_GPUDevice* dev, const std::string& pk3,
                                       const std::vector<std::string>& candidates) {
    for (const auto& entry : candidates) {
        auto raw = ReadFromPk3(pk3, entry);
        if (raw.empty()) continue;
        int w, h, ch;
        unsigned char* px = stbi_load_from_memory(raw.data(), (int)raw.size(), &w, &h, &ch, 4);
        if (!px) continue;
        auto* tex = UploadTexture(dev, px, w, h);
        stbi_image_free(px);
        if (tex) return tex;
    }
    return nullptr;
}

// ── animation.cfg parser ─────────────────────────────────────────────────────

static nlohmann::json ParseAnimCfg(const std::vector<uint8_t>& data) {
    nlohmann::json result = nlohmann::json::array();
    std::string txt(data.begin(), data.end());
    std::istringstream ss(txt);
    std::string line;
    while (std::getline(ss, line)) {
        auto cp = line.find("//");
        if (cp != std::string::npos) line = line.substr(0, cp);
        std::istringstream ls(line);
        int first, num, loop, fps;
        if (ls >> first >> num >> loop >> fps) {
            result.push_back({{"first", first}, {"num", num},
                              {"loop", loop},   {"fps", fps}});
        }
    }
    return result;
}

}  // namespace

// ── Step implementation ──────────────────────────────────────────────────────

WorkflowQ3Md3LoadStep::WorkflowQ3Md3LoadStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3Md3LoadStep::GetPluginId() const { return "q3.md3.load"; }

void WorkflowQ3Md3LoadStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepParameterResolver params;
    auto getStr = [&](const char* k, const std::string& def) -> std::string {
        const auto* p = params.FindParameter(step, k);
        return (p && p->type == WorkflowParameterValue::Type::String) ? p->stringValue : def;
    };

    const std::string prefix = getStr("prefix", "model");
    const std::string path   = getStr("path",   "");
    const std::string skin   = getStr("skin",   "");
    const std::string anim   = getStr("anim",   "");

    // Idempotent: skip if already loaded
    if (context.Get<int>("q3.md3." + prefix + "_num_frames", -1) >= 0) return;

    // Read pk3 path from bsp_config (set by bsp.load step)
    const auto bspCfg = context.Get<nlohmann::json>("bsp_config", nlohmann::json{});
    const std::string pk3 = bspCfg.value("pk3_path", std::string(""));
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (pk3.empty() || !device || path.empty()) {
        if (logger_) logger_->Warn("q3.md3.load[" + prefix + "]: pk3/device/path missing");
        return;
    }

    auto md3raw = ReadFromPk3(pk3, path);
    if (md3raw.size() < sizeof(Md3Header)) {
        if (logger_) logger_->Warn("q3.md3.load[" + prefix + "]: cannot read " + path);
        return;
    }

    const uint8_t* base = md3raw.data();
    const auto& hdr = *reinterpret_cast<const Md3Header*>(base);
    const int nFrames = hdr.numFrames, nTags = hdr.numTags, nSurfs = hdr.numSurfaces;
    if (nFrames <= 0 || nSurfs <= 0) return;

    // ── Tags: convert from Q3 Z-up → engine Y-up ─────────────────────────────
    // Q3 tag axis[i] are rows of the rotation matrix (right, forward, up in Q3 space).
    // In engine space: x'=x, y'=z, z'=-y (for both origin and each basis vector).
    nlohmann::json tagsJson = nlohmann::json::array();
    if (nTags > 0 && hdr.ofsTags > 0) {
        const auto* rawTags = reinterpret_cast<const Md3Tag*>(base + hdr.ofsTags);
        for (int f = 0; f < nFrames; ++f) {
            nlohmann::json frameObj = nlohmann::json::object();
            for (int t = 0; t < nTags; ++t) {
                const auto& tag = rawTags[f * nTags + t];
                std::string tname(tag.name, strnlen(tag.name, 64));

                // Origin in engine coords (scaled to match BSP world)
                nlohmann::json tagObj;
                tagObj["origin"] = nlohmann::json::array({
                    tag.origin[0] * kMd3XyzScale * kMd3WorldScale,
                    tag.origin[2] * kMd3XyzScale * kMd3WorldScale,   // Q3 Z → eng Y
                   -tag.origin[1] * kMd3XyzScale * kMd3WorldScale    // -Q3 Y → eng Z
                });

                // Axis rows converted to engine space
                nlohmann::json axisArr = nlohmann::json::array();
                for (int i = 0; i < 3; ++i) {
                    axisArr.push_back(nlohmann::json::array({
                        tag.axis[i][0],
                        tag.axis[i][2],   // Q3 Z → eng Y
                       -tag.axis[i][1]    // -Q3 Y → eng Z
                    }));
                }
                tagObj["axis"] = axisArr;
                frameObj[tname] = tagObj;
            }
            tagsJson.push_back(frameObj);
        }
    }
    context.Set("q3.md3." + prefix + "_tags",      tagsJson);
    context.Set<int>("q3.md3." + prefix + "_num_frames", nFrames);
    context.Set<int>("q3.md3." + prefix + "_num_surfs",  nSurfs);
    context.Set<int>("q3.md3." + prefix + "_num_tags",   nTags);

    // ── animation.cfg ─────────────────────────────────────────────────────────
    if (!anim.empty()) {
        auto animData = ReadFromPk3(pk3, anim);
        if (!animData.empty())
            context.Set("q3.md3." + prefix + "_anim", ParseAnimCfg(animData));
    }

    // Derive the directory from path (for relative skin lookups)
    std::string pathDir = path;
    auto lastSlash = pathDir.rfind('/');
    if (lastSlash != std::string::npos) pathDir = pathDir.substr(0, lastSlash + 1);

    // ── Surfaces ──────────────────────────────────────────────────────────────
    const uint8_t* surfPtr = base + hdr.ofsSurfaces;

    struct PosUv { float x, y, z, u, v; };  // matches pipeline "position_uv"

    for (int s = 0; s < nSurfs; ++s) {
        const auto& surf = *reinterpret_cast<const Md3Surface*>(surfPtr);
        const std::string sk = "q3.md3." + prefix + "_surf" + std::to_string(s);

        if (surf.numVerts <= 0 || surf.numTriangles <= 0) {
            surfPtr += surf.ofsEnd; continue;
        }

        // Index buffer (uint16) — MD3 triangles are already CCW (OpenGL convention), no swap needed
        const auto* tris = reinterpret_cast<const Md3Triangle*>(surfPtr + surf.ofsTriangles);
        std::vector<uint16_t> indices;
        indices.reserve(surf.numTriangles * 3);
        for (int i = 0; i < surf.numTriangles; ++i) {
            indices.push_back((uint16_t)tris[i].indexes[0]);
            indices.push_back((uint16_t)tris[i].indexes[1]);
            indices.push_back((uint16_t)tris[i].indexes[2]);
        }
        auto* ib = UploadBuffer(device, SDL_GPU_BUFFERUSAGE_INDEX,
                                indices.data(), (uint32_t)(indices.size() * 2));
        context.Set<SDL_GPUBuffer*>(sk + "_ib", ib);
        context.Set<int>(sk + "_num_idx", (int)indices.size());

        // UV coordinates (shared across all frames)
        const auto* uvs = reinterpret_cast<const Md3St*>(surfPtr + surf.ofsSt);

        // Per-frame vertex buffers: decode int16 xyz → float, convert coords, bake scale
        const auto* xyzn = reinterpret_cast<const Md3XyzNormal*>(surfPtr + surf.ofsXyzNormals);
        const int nv = surf.numVerts;
        std::vector<PosUv> verts((size_t)nv);

        for (int f = 0; f < nFrames; ++f) {
            const Md3XyzNormal* fxyz = xyzn + f * nv;
            for (int vi = 0; vi < nv; ++vi) {
                // Q3 Z-up → engine Y-up, scaled to BSP world
                verts[vi].x =  fxyz[vi].xyz[0] * kMd3XyzScale * kMd3WorldScale;
                verts[vi].y =  fxyz[vi].xyz[2] * kMd3XyzScale * kMd3WorldScale;
                verts[vi].z = -fxyz[vi].xyz[1] * kMd3XyzScale * kMd3WorldScale;
                verts[vi].u = uvs[vi].st[0];
                verts[vi].v = uvs[vi].st[1];
            }
            auto* vb = UploadBuffer(device, SDL_GPU_BUFFERUSAGE_VERTEX,
                                    verts.data(), (uint32_t)(nv * sizeof(PosUv)));
            context.Set<SDL_GPUBuffer*>(sk + "_f" + std::to_string(f) + "_vb", vb);
        }

        // ── Texture: prefer shader name from surface, then path-derived candidates ──
        std::string shaderName;
        if (surf.numShaders > 0) {
            const auto* sh = reinterpret_cast<const Md3Shader*>(surfPtr + surf.ofsShaders);
            shaderName = std::string(sh[0].name, strnlen(sh[0].name, 64));
        }
        std::vector<std::string> texCandidates;
        if (!shaderName.empty()) {
            texCandidates.push_back(shaderName + ".tga");
            texCandidates.push_back(shaderName + ".jpg");
            texCandidates.push_back(shaderName);
        }
        if (!skin.empty()) {
            texCandidates.push_back(skin);
            auto dot = skin.rfind('.');
            if (dot != std::string::npos)
                texCandidates.push_back(skin.substr(0, dot) + ".jpg");
        }
        // Fallback: same name as MD3 but with texture extensions
        {
            std::string noext = path;
            auto dot = noext.rfind('.');
            if (dot != std::string::npos) noext = noext.substr(0, dot);
            texCandidates.push_back(noext + ".tga");
            texCandidates.push_back(noext + ".jpg");
        }

        auto* tex = TryLoadTexture(device, pk3, texCandidates);
        if (!tex) {
            // 1×1 grey fallback
            const uint8_t grey[4] = {128, 128, 128, 255};
            tex = UploadTexture(device, grey, 1, 1);
        }
        auto* samp = MakeLinearSampler(device);
        context.Set<SDL_GPUTexture*>(sk + "_tex",  tex);
        context.Set<SDL_GPUSampler*>(sk + "_samp", samp);

        surfPtr += surf.ofsEnd;
    }

    if (logger_) logger_->Info("q3.md3.load[" + prefix + "]: loaded " +
        path + " (" + std::to_string(nSurfs) + " surfs, " +
        std::to_string(nFrames) + " frames)");
}

}  // namespace sdl3cpp::services::impl
