local M = {}

local string_format = string.format

local function get_logger(log_debug)
    if type(log_debug) == "function" then
        return log_debug
    end
    return function()
    end
end

local function resolve_color3(value, fallback)
    if type(value) == "table" then
        local r = tonumber(value[1])
        local g = tonumber(value[2])
        local b = tonumber(value[3])
        if r and g and b then
            return {r, g, b}
        end
    end
    return {fallback[1], fallback[2], fallback[3]}
end

local function resolve_number_optional(value)
    if type(value) == "number" then
        return value
    end
    return nil
end

local function resolve_color3_optional(value)
    if type(value) == "table" then
        local r = tonumber(value[1])
        local g = tonumber(value[2])
        local b = tonumber(value[3])
        if r and g and b then
            return {r, g, b}
        end
    end
    return nil
end

local function format_optional_number(value)
    if type(value) == "number" then
        return string_format("%.3f", value)
    end
    return "default"
end

local function format_optional_color(value)
    if type(value) == "table"
        and type(value[1]) == "number"
        and type(value[2]) == "number"
        and type(value[3]) == "number" then
        return string_format("{%.2f, %.2f, %.2f}", value[1], value[2], value[3])
    end
    return "default"
end

local function build_shader_parameter_overrides(config, log_debug)
    if type(config) ~= "table" or type(config.atmospherics) ~= "table" then
        return {}
    end

    local atmospherics = config.atmospherics
    local ambient_strength = resolve_number_optional(atmospherics.ambient_strength)
    local light_intensity = resolve_number_optional(atmospherics.light_intensity)
    local key_intensity = resolve_number_optional(atmospherics.key_light_intensity)
    local fill_intensity = resolve_number_optional(atmospherics.fill_light_intensity)
    local light_color = resolve_color3_optional(atmospherics.light_color)
    local pbr_roughness = resolve_number_optional(atmospherics.pbr_roughness)
    local pbr_metallic = resolve_number_optional(atmospherics.pbr_metallic)

    local parameters = {}

    local function apply_common(key)
        if ambient_strength == nil and light_intensity == nil and light_color == nil then
            return
        end
        local entry = {}
        if ambient_strength ~= nil then
            entry.ambient_strength = ambient_strength
        end
        if light_intensity ~= nil then
            entry.light_intensity = light_intensity
        end
        if light_color ~= nil then
            entry.light_color = light_color
        end
        parameters[key] = entry
    end

    apply_common("solid")
    apply_common("floor")
    apply_common("wall")
    apply_common("ceiling")

    if ambient_strength ~= nil or key_intensity ~= nil or fill_intensity ~= nil or light_intensity ~= nil then
        local entry = {}
        if ambient_strength ~= nil then
            entry.ambient_strength = ambient_strength
        end
        if key_intensity ~= nil then
            entry.key_intensity = key_intensity
        elseif light_intensity ~= nil then
            entry.key_intensity = light_intensity
        end
        if fill_intensity ~= nil then
            entry.fill_intensity = fill_intensity
        elseif light_intensity ~= nil then
            entry.fill_intensity = light_intensity * 0.45
        end
        parameters.default = entry
    end

    if light_intensity ~= nil or light_color ~= nil or pbr_roughness ~= nil or pbr_metallic ~= nil then
        local entry = {}
        if light_intensity ~= nil then
            entry.light_intensity = light_intensity
        end
        if light_color ~= nil then
            entry.light_color = light_color
        end
        if pbr_roughness ~= nil then
            entry.material_roughness = pbr_roughness
        end
        if pbr_metallic ~= nil then
            entry.material_metallic = pbr_metallic
        end
        parameters.pbr = entry
    end

    if next(parameters) ~= nil then
        log_debug("Shader lighting overrides: ambient=%s light_intensity=%s light_color=%s",
            format_optional_number(ambient_strength),
            format_optional_number(light_intensity),
            format_optional_color(light_color))
    end

    return parameters
end

local function resolve_skybox_color(config, default_color)
    if type(config) ~= "table" then
        return default_color
    end
    local atmospherics = config.atmospherics
    if type(atmospherics) ~= "table" then
        return default_color
    end
    return resolve_color3(atmospherics.sky_color, default_color)
end

local function build_static_cube_variants()
    local fallback_vertex_source = [[
#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pushConstants;

void main() {
    fragColor = inColor;
    gl_Position = pushConstants.viewProj * pushConstants.model * vec4(inPos, 1.0);
}
]]

    local fallback_fragment_source = [[
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
]]

    return {
        default = {
            vertex_source = fallback_vertex_source,
            fragment_source = fallback_fragment_source,
        },
        solid = {
            vertex_source = fallback_vertex_source,
            fragment_source = fallback_fragment_source,
        },
        floor = {
            vertex_source = fallback_vertex_source,
            fragment_source = fallback_fragment_source,
        },
        wall = {
            vertex_source = fallback_vertex_source,
            fragment_source = fallback_fragment_source,
        },
        ceiling = {
            vertex_source = fallback_vertex_source,
            fragment_source = fallback_fragment_source,
        },
        pbr = {
            vertex_source = fallback_vertex_source,
            fragment_source = fallback_fragment_source,
        },
        skybox = {
            vertex_source = fallback_vertex_source,
            fragment_source = fallback_fragment_source,
        },
    }
end

local function count_shader_variants(variants)
    local count = 0
    for _ in pairs(variants) do
        count = count + 1
    end
    return count
end

function M.build_cube_variants(config, log_debug, base_skybox_color)
    local logger = get_logger(log_debug)
    local skybox_color = resolve_skybox_color(config, base_skybox_color or {0.04, 0.05, 0.08})
    local shader_parameters = build_shader_parameter_overrides(config, logger)

    local ok, toolkit = pcall(require, "shader_toolkit")
    if not ok then
        logger("Shader toolkit unavailable: %s", tostring(toolkit))
        return build_static_cube_variants(), skybox_color
    end

    local output_mode = "source"
    local compile = false
    local ok_generate, generated = pcall(toolkit.generate_cube_demo_variants,
        {compile = compile, output_mode = output_mode, parameters = shader_parameters})
    if not ok_generate then
        logger("Shader generation failed: %s", tostring(generated))
        return build_static_cube_variants(), skybox_color
    end

    local ok_skybox, skybox_variant = pcall(toolkit.generate_variant, {
        key = "skybox",
        template = "solid_color",
        output_mode = output_mode,
        compile = compile,
        parameters = {color = skybox_color},
    })
    if ok_skybox then
        generated.skybox = skybox_variant
    else
        logger("Skybox shader generation failed: %s", tostring(skybox_variant))
    end

    logger("Generated %d shader variants", count_shader_variants(generated))
    return generated, skybox_color
end

function M.build_gui_variants()
    local ok, toolkit = pcall(require, "shader_toolkit")
    if not ok then
        error("Shader toolkit unavailable: " .. tostring(toolkit))
    end

    local ok_generate, variant = pcall(toolkit.generate_variant, {
        key = "default",
        template = "gui_2d",
        output_mode = "source",
        compile = false,
    })
    if not ok_generate then
        error("Shader generation failed: " .. tostring(variant))
    end

    return {default = variant}
end

function M.build_soundboard_variants()
    local ok, toolkit = pcall(require, "shader_toolkit")
    if not ok then
        error("Shader toolkit unavailable: " .. tostring(toolkit))
    end

    local ok_generate, variant = pcall(toolkit.generate_variant, {
        key = "default",
        template = "cube_rainbow",
        output_name = "cube",
        output_mode = "source",
        compile = false,
    })
    if not ok_generate then
        error("Shader generation failed: " .. tostring(variant))
    end

    return {default = variant}
end

return M
