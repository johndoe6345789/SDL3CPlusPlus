#include "services/interfaces/workflow/rendering/workflow_draw_map_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace sdl3cpp::services::impl {

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool IsPortalTexture(const std::string& textureName) {
    const std::string lower = ToLower(textureName);
    return lower.find("portal_sfx") != std::string::npos ||
           lower.find("mapobjects/portal") != std::string::npos;
}

}  // namespace

WorkflowDrawMapStep::WorkflowDrawMapStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowDrawMapStep::GetPluginId() const {
    return "draw.map";
}

void WorkflowDrawMapStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    if (!pass || !cmd) return;

    const auto* mapNodes = context.TryGet<nlohmann::json>("map.nodes");
    if (!mapNodes || !mapNodes->is_array() || mapNodes->empty()) return;

    // Detect BSP mode: prefer gpu_pipeline_bsp if available
    auto* bspPipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_bsp", nullptr);
    auto* texPipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline_textured", nullptr);
    bool isBsp = (bspPipeline != nullptr);
    auto* pipeline = isBsp ? bspPipeline : texPipeline;
    if (!pipeline) return;

    // Read texture mapping from parameters
    std::vector<std::pair<std::string, std::string>> textureMappings;
    std::string defaultTexture;

    for (const auto& [key, param] : step.parameters) {
        if (key == "roughness" || key == "metallic" || key == "pipeline_key") continue;
        if (param.type == WorkflowParameterValue::Type::String) {
            if (key == "default_texture") {
                defaultTexture = param.stringValue;
            } else {
                textureMappings.emplace_back(key, param.stringValue);
            }
        }
    }
    if (defaultTexture.empty() && !textureMappings.empty()) {
        defaultTexture = textureMappings.back().second;
    }

    WorkflowStepParameterResolver params;
    auto getNum = [&](const char* name, float def) -> float {
        const auto* p = params.FindParameter(step, name);
        return (p && p->type == WorkflowParameterValue::Type::Number) ? static_cast<float>(p->numberValue) : def;
    };
    const float roughness = getNum("roughness", 0.8f);
    const float metallic = getNum("metallic", 0.0f);

    auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));

    auto fu = context.Get<rendering::FragmentUniformData>("render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = roughness;
    fu.material[1] = metallic;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = proj * view * model;

    rendering::VertexUniformData vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[0] = 0; vu.normal[1] = 1; vu.normal[2] = 0;
    vu.uv_scale[0] = 1.0f; vu.uv_scale[1] = 1.0f;
    vu.camera_pos[0] = camPos.x; vu.camera_pos[1] = camPos.y; vu.camera_pos[2] = camPos.z;
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    auto* shadow_tex = context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    auto* shadow_samp = context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);

    // BSP lightmap atlas (shared across all groups)
    auto* lm_tex = context.Get<SDL_GPUTexture*>("bsp_lightmap_atlas_gpu", nullptr);
    auto* lm_samp = context.Get<SDL_GPUSampler*>("bsp_lightmap_atlas_sampler", nullptr);
    auto* portal_tex = context.Get<SDL_GPUTexture*>("bsp_portal_view_texture", nullptr);
    auto* portal_samp = context.Get<SDL_GPUSampler*>("bsp_portal_view_sampler", nullptr);

    if (isBsp) {
        // BSP mode: single VB + single IB, per-group draw calls
        const auto& firstNode = (*mapNodes)[0];
        std::string meshName = firstNode["name"];
        auto* vb = context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_vb", nullptr);
        auto* ib = context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_ib", nullptr);
        if (!vb || !ib) return;

        // Bind VB + IB once
        SDL_GPUBufferBinding vbBind = {}; vbBind.buffer = vb;
        SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);
        SDL_GPUBufferBinding ibBind = {}; ibBind.buffer = ib;
        SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        // Vertex uniforms are shared by all BSP texture groups.
        SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));

        // Default texture fallback
        auto* defaultTex = context.Get<SDL_GPUTexture*>(defaultTexture + "_gpu", nullptr);
        auto* defaultSamp = context.Get<SDL_GPUSampler*>(defaultTexture + "_sampler", nullptr);
        const float elapsed = static_cast<float>(context.GetDouble("frame.elapsed", 0.0));

        for (const auto& node : *mapNodes) {
            uint32_t indexCount = node["index_count"];
            uint32_t indexOffset = node.value("index_offset", 0u);
            int texIdx = node.value("texture_index", -1);
            std::string textureName = node.value("texture_name", std::string{});

            // Look up per-texture albedo
            SDL_GPUTexture* albedoTex = nullptr;
            SDL_GPUSampler* albedoSamp = nullptr;

            if (texIdx >= 0) {
                std::string texKey = "bsp_tex_" + std::to_string(texIdx);
                albedoTex = context.Get<SDL_GPUTexture*>(texKey + "_gpu", nullptr);
                albedoSamp = context.Get<SDL_GPUSampler*>(texKey + "_sampler", nullptr);
            }

            if (!albedoTex || !albedoSamp) {
                albedoTex = defaultTex;
                albedoSamp = defaultSamp;
            }
            if (!albedoTex || !albedoSamp) continue;

            // Bind 4 samplers: albedo, shadow, lightmap, portal destination.
            SDL_GPUTextureSamplerBinding bindings[4] = {};
            bindings[0].texture = albedoTex;
            bindings[0].sampler = albedoSamp;
            bindings[1].texture = shadow_tex ? shadow_tex : albedoTex;
            bindings[1].sampler = shadow_samp ? shadow_samp : albedoSamp;
            bindings[2].texture = lm_tex ? lm_tex : albedoTex;
            bindings[2].sampler = lm_samp ? lm_samp : albedoSamp;
            bindings[3].texture = portal_tex ? portal_tex : albedoTex;
            bindings[3].sampler = portal_samp ? portal_samp : albedoSamp;
            SDL_BindGPUFragmentSamplers(pass, 0, bindings, 4);

            auto groupFu = fu;
            groupFu.material[1] = elapsed;
            groupFu.material[3] = IsPortalTexture(textureName) ? 1.0f : 0.0f;
            SDL_PushGPUFragmentUniformData(cmd, 0, &groupFu, sizeof(groupFu));
            SDL_DrawGPUIndexedPrimitives(pass, indexCount, 1, indexOffset, 0, 0);
        }
    } else {
        // Legacy non-BSP mode: per-mesh draw calls with 16-bit indices
        for (const auto& node : *mapNodes) {
            std::string meshName = node["name"];
            auto* vb = context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_vb", nullptr);
            auto* ib = context.Get<SDL_GPUBuffer*>("plane_" + meshName + "_ib", nullptr);
            if (!vb || !ib) continue;

            uint32_t indexCount = node["index_count"];

            // Find matching texture from JSON mappings
            std::string texKey = defaultTexture;
            for (const auto& [pattern, texName] : textureMappings) {
                if (meshName.find(pattern) != std::string::npos) {
                    texKey = texName;
                    break;
                }
            }

            auto* meshTex = context.Get<SDL_GPUTexture*>(texKey + "_gpu", nullptr);
            auto* meshSamp = context.Get<SDL_GPUSampler*>(texKey + "_sampler", nullptr);
            if (!meshTex || !meshSamp) continue;

            {
                SDL_GPUTextureSamplerBinding bindings[2] = {};
                bindings[0].texture = meshTex;
                bindings[0].sampler = meshSamp;
                bindings[1].texture = shadow_tex ? shadow_tex : meshTex;
                bindings[1].sampler = shadow_samp ? shadow_samp : meshSamp;
                SDL_BindGPUFragmentSamplers(pass, 0, bindings, 2);
            }

            SDL_GPUBufferBinding vbBind = {}; vbBind.buffer = vb;
            SDL_BindGPUVertexBuffers(pass, 0, &vbBind, 1);
            SDL_GPUBufferBinding ibBind = {}; ibBind.buffer = ib;
            SDL_BindGPUIndexBuffer(pass, &ibBind, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            // Normal from bounding box thinnest axis
            if (node.contains("bb_min") && node.contains("bb_max")) {
                auto bbMin = node["bb_min"];
                auto bbMax = node["bb_max"];
                float dx = bbMax[0].get<float>() - bbMin[0].get<float>();
                float dy = bbMax[1].get<float>() - bbMin[1].get<float>();
                float dz = bbMax[2].get<float>() - bbMin[2].get<float>();
                if (dy < dx && dy < dz) {
                    vu.normal[0] = 0; vu.normal[1] = 1; vu.normal[2] = 0;
                } else if (dx < dz) {
                    vu.normal[0] = 1; vu.normal[1] = 0; vu.normal[2] = 0;
                } else {
                    vu.normal[0] = 0; vu.normal[1] = 0; vu.normal[2] = 1;
                }
            }

            SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
            SDL_PushGPUFragmentUniformData(cmd, 0, &fu, sizeof(fu));
            SDL_DrawGPUIndexedPrimitives(pass, indexCount, 1, 0, 0, 0);
        }
    }
}

}  // namespace sdl3cpp::services::impl
