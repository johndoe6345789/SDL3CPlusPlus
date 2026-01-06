local shader_toolkit = {}

local path_sep = package.config:sub(1, 1)
local is_windows = path_sep == "\\"

local function quote_arg(value)
    return string.format("%q", value)
end

local function execute_command(command)
    local ok, exit_type, code = os.execute(command)
    if ok == true then
        return true
    end
    if type(ok) == "number" then
        return ok == 0
    end
    return exit_type == "exit" and code == 0
end

local function ensure_directory(path)
    if not path or path == "" or path == "." then
        return true
    end
    local command
    if is_windows then
        command = string.format("mkdir %s >NUL 2>&1", quote_arg(path))
    else
        command = string.format("mkdir -p %s >/dev/null 2>&1", quote_arg(path))
    end
    return execute_command(command)
end

local function get_directory(path)
    return path:match("^(.*)[/\\\\]") or ""
end

local function path_join(...)
    local parts = {...}
    local result = ""
    for index, part in ipairs(parts) do
        if part and part ~= "" then
            if result ~= "" and result:sub(-1) ~= path_sep then
                result = result .. path_sep
            end
            if index > 1 and part:sub(1, 1) == path_sep then
                part = part:sub(2)
            end
            result = result .. part
        end
    end
    return result
end

local function is_absolute_path(path)
    if not path or path == "" then
        return false
    end
    if path:sub(1, 1) == "/" then
        return true
    end
    return is_windows and path:match("^%a:[/\\\\]") ~= nil
end

local function file_exists(path)
    local file = io.open(path, "rb")
    if file then
        file:close()
        return true
    end
    return false
end

local function normalize_output_name(name)
    if not name or name == "" then
        return nil
    end
    if name:match("%.vert$") then
        return name:gsub("%.vert$", "")
    end
    if name:match("%.frag$") then
        return name:gsub("%.frag$", "")
    end
    return name
end

local function get_module_directory()
    if not debug or not debug.getinfo then
        return nil
    end
    local info = debug.getinfo(1, "S")
    local source = info and info.source or ""
    if source:sub(1, 1) == "@" then
        source = source:sub(2)
    end
    return get_directory(source)
end

local function resolve_output_dir(output_dir)
    if output_dir and output_dir ~= "" then
        return output_dir
    end
    local shader_dir = "shaders"
    local project_root = nil
    if type(config) == "table" then
        shader_dir = config.shaders_directory or shader_dir
        project_root = config.project_root
    end
    if is_absolute_path(shader_dir) then
        return shader_dir
    end

    local base_dir = get_module_directory() or "."
    if project_root and project_root ~= "" then
        if is_absolute_path(project_root) then
            base_dir = project_root
        else
            base_dir = path_join(base_dir, project_root)
        end
    else
        base_dir = path_join(base_dir, "..")
    end
    return path_join(base_dir, shader_dir)
end

local function find_in_path(command)
    local check_command
    if is_windows then
        check_command = "where " .. command .. " 2>NUL"
    else
        check_command = "command -v " .. command .. " 2>/dev/null"
    end
    local handle = io.popen(check_command)
    if not handle then
        return false
    end
    local output = handle:read("*a") or ""
    handle:close()
    return output ~= ""
end

local function detect_compiler()
    local candidates = {"glslangValidator", "glslc"}
    for _, candidate in ipairs(candidates) do
        if find_in_path(candidate) then
            return candidate
        end
    end
    return nil
end

local function compile_shader(compiler, input_path, output_path)
    local quoted_compiler = quote_arg(compiler)
    local quoted_input = quote_arg(input_path)
    local quoted_output = quote_arg(output_path)
    local command
    if compiler:find("glslangValidator") then
        command = string.format("%s -V %s -o %s", quoted_compiler, quoted_input, quoted_output)
    else
        command = string.format("%s %s -o %s", quoted_compiler, quoted_input, quoted_output)
    end
    if not execute_command(command) then
        error("Shader compilation failed: " .. command)
    end
end

local function write_text_file(path, contents)
    local directory = get_directory(path)
    if directory ~= "" and not ensure_directory(directory) then
        error("Failed to create directory: " .. directory)
    end
    local file = io.open(path, "w")
    if not file then
        error("Failed to open file for writing: " .. path)
    end
    file:write(contents)
    file:close()
end

local function read_text_file(path)
    local file = io.open(path, "rb")
    if not file then
        return nil
    end
    local contents = file:read("*a")
    file:close()
    return contents
end

local function write_text_file_if_changed(path, contents)
    local existing = read_text_file(path)
    if existing == contents then
        return
    end
    write_text_file(path, contents)
end

local function normalize_number(value, fallback)
    local number_value = tonumber(value)
    if number_value == nil then
        return fallback
    end
    return number_value
end

local function normalize_color(color, fallback)
    local default = fallback or {1.0, 1.0, 1.0, 1.0}
    if type(color) ~= "table" then
        return {default[1], default[2], default[3], default[4]}
    end
    local r = normalize_number(color[1], default[1])
    local g = normalize_number(color[2], default[2])
    local b = normalize_number(color[3], default[3])
    local a = normalize_number(color[4], default[4])
    return {r, g, b, a}
end

local function normalize_vec3(value, fallback)
    local default = fallback or {1.0, 1.0, 1.0}
    if type(value) ~= "table" then
        return {default[1], default[2], default[3]}
    end
    return {
        normalize_number(value[1], default[1]),
        normalize_number(value[2], default[2]),
        normalize_number(value[3], default[3]),
    }
end

local function format_vec3(value)
    return string.format("vec3(%.3f, %.3f, %.3f)", value[1], value[2], value[3])
end

local function merge_tables(base, overrides)
    local merged = {}
    if type(base) == "table" then
        for key, value in pairs(base) do
            merged[key] = value
        end
    end
    if type(overrides) == "table" then
        for key, value in pairs(overrides) do
            merged[key] = value
        end
    end
    return merged
end

local ShaderTemplate = {}
ShaderTemplate.__index = ShaderTemplate

function ShaderTemplate:new(name, generator, defaults)
    if type(name) ~= "string" or name == "" then
        error("Template name must be a non-empty string")
    end
    if type(generator) ~= "function" then
        error("Template generator must be a function")
    end
    local instance = {
        name = name,
        generator = generator,
        defaults = defaults or {},
    }
    return setmetatable(instance, ShaderTemplate)
end

function ShaderTemplate:Generate(options)
    local merged = merge_tables(self.defaults, options)
    return self.generator(merged)
end

local ShaderVariant = {}
ShaderVariant.__index = ShaderVariant

function ShaderVariant:new(options)
    if type(options) ~= "table" then
        error("Shader variant options must be a table")
    end
    local instance = {
        key = options.key,
        template = options.template or options.template_name,
        output_name = options.output_name or options.outputName,
        output_dir = options.output_dir or options.outputDir,
        output_mode = options.output_mode or options.outputMode,
        compile = options.compile,
        compiler = options.compiler,
        skip_if_present = options.skip_if_present,
        parameters = options.parameters or options.params,
    }
    return setmetatable(instance, ShaderVariant)
end

local push_constants_block = [[
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
]]

local vertex_color_source = [[
#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

]] .. push_constants_block .. [[

void main() {
    fragColor = inColor;
    gl_Position = pushConstants.viewProj * pushConstants.model * vec4(inPos, 1.0);
}
]]

local vertex_color_fragment_source = [[
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
]]

local gui_2d_vertex_source = [[
#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics (ignored by basic shaders)
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
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
    vec4 worldPos = pushConstants.model * vec4(inPos, 1.0);
    gl_Position = pushConstants.viewProj * worldPos;
}
]]

local gui_2d_fragment_source = [[
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}
]]

local shadow_vertex_source = [[
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pc;

void main() {
    gl_Position = pc.lightViewProj * pc.model * vec4(inPosition, 1.0);
}
]]

local shadow_fragment_source = [[
#version 450

void main() {
    // Empty fragment shader for shadow mapping
    // Depth is automatically written
}
]]

local fullscreen_vertex_source = [[
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragPosition;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    fragPosition = inPosition;
    fragTexCoord = inTexCoord;
    gl_Position = vec4(inPosition, 0.0, 1.0);
}
]]

local ssgi_fragment_source = [[
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneColor;
layout(set = 0, binding = 1) uniform sampler2D normalBuffer;
layout(set = 0, binding = 2) uniform sampler2D depthBuffer;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pc;

const int NUM_SAMPLES = 16;
const float SAMPLE_RADIUS = 0.5;

// Reconstruct world position from depth
vec3 worldPosFromDepth(float depth, vec2 texCoord) {
    vec4 clipSpace = vec4(texCoord * 2.0 - 1.0, depth, 1.0);
    vec4 viewSpace = pc.proj * clipSpace;  // Use proj as invProj for now (dummy)
    viewSpace /= viewSpace.w;
    vec4 worldSpace = pc.view * viewSpace;  // Use view as invView for now (dummy)
    return worldSpace.xyz;
}

vec3 ssao(vec2 texCoord) {
    float depth = texture(depthBuffer, texCoord).r;
    if (depth >= 1.0) return vec3(1.0);  // Skybox

    vec3 worldPos = worldPosFromDepth(depth, texCoord);
    vec3 normal = normalize(texture(normalBuffer, texCoord).rgb * 2.0 - 1.0);

    float occlusion = 0.0;

    for (int i = 0; i < NUM_SAMPLES; i++) {
        // Generate sample position in hemisphere around normal
        float angle = (float(i) / float(NUM_SAMPLES)) * 6.283185;
        vec2 offset = vec2(cos(angle), sin(angle)) * SAMPLE_RADIUS;

        vec2 sampleTexCoord = texCoord + offset / textureSize(depthBuffer, 0);
        float sampleDepth = texture(depthBuffer, sampleTexCoord).r;

        if (sampleDepth < depth - 0.01) {  // Occluded
            occlusion += 1.0;
        }
    }

    occlusion = 1.0 - (occlusion / float(NUM_SAMPLES));
    return vec3(occlusion);
}

void main() {
    vec3 sceneColor = texture(sceneColor, inTexCoord).rgb;
    vec3 ao = ssao(inTexCoord);

    // Apply ambient occlusion
    vec3 finalColor = sceneColor * (0.3 + 0.7 * ao);  // Mix AO with direct lighting

    outColor = vec4(finalColor, 1.0);
}
]]

local volumetric_fragment_source = [[
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneColor;
layout(set = 0, binding = 1) uniform sampler2D depthBuffer;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pc;

const int NUM_SAMPLES = 100;
const float DECAY_BASE = 0.96815;
const float WEIGHT_BASE = 0.58767;
const float EXPOSURE = 0.2;

void main() {
    vec2 texCoord = inTexCoord;
    vec2 lightScreenPos = vec2(0.5, 0.5);  // Dummy light position
    vec2 deltaTexCoord = (texCoord - lightScreenPos);
    deltaTexCoord *= 1.0 / float(NUM_SAMPLES) * 0.5;  // Scale for effect

    vec3 color = texture(sceneColor, texCoord).rgb;

    // Only apply god rays if we're looking towards the light
    float centerDistance = length(lightScreenPos - vec2(0.5, 0.5));
    if (centerDistance < 0.8) {  // Light is visible on screen
        vec3 godRayColor = vec3(0.0);
        float weight = WEIGHT_BASE;

        for (int i = 0; i < NUM_SAMPLES; i++) {
            texCoord -= deltaTexCoord;
            vec3 sampleColor = texture(sceneColor, texCoord).rgb;
            godRayColor += sampleColor * weight;
            weight *= DECAY_BASE;
        }

        color += godRayColor * EXPOSURE * 1.0;  // Dummy intensity
    }

    outColor = vec4(color, 1.0);
}
]]

local vertex_world_color_source = [[
#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics (ignored by basic shaders)
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
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
    vec4 worldPos = pushConstants.model * vec4(inPos, 1.0);
    fragWorldPos = worldPos.xyz;
    gl_Position = pushConstants.viewProj * worldPos;
}
]]

local function build_cube_rainbow_fragment_source(options)
    local band_scale = normalize_number(options.band_scale, 0.35)
    local diagonal_scale = normalize_number(options.diagonal_scale, 0.25)
    local mix_factor = normalize_number(options.mix_factor, 0.08)
    local ambient_strength = normalize_number(options.ambient_strength, 0.28)
    local key_intensity = normalize_number(options.key_intensity, 0.9)
    local fill_intensity = normalize_number(options.fill_intensity, 0.4)
    local key_dir = format_vec3(normalize_vec3(options.key_dir, {-0.35, 1.0, -0.25}))
    local fill_dir = format_vec3(normalize_vec3(options.fill_dir, {0.45, 0.6, 0.2}))

    return string.format([[
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

vec3 ComputeNormal() {
    vec3 dx = dFdx(fragWorldPos);
    vec3 dy = dFdy(fragWorldPos);
    return normalize(cross(dx, dy));
}

vec3 RainbowBand(float t) {
    t = fract(t);
    float scaled = t * 5.0;
    int index = int(floor(scaled));
    float blend = fract(scaled);

    vec3 red = vec3(0.91, 0.19, 0.21);
    vec3 orange = vec3(0.99, 0.49, 0.09);
    vec3 yellow = vec3(0.99, 0.86, 0.22);
    vec3 green = vec3(0.16, 0.74, 0.39);
    vec3 blue = vec3(0.24, 0.48, 0.88);
    vec3 purple = vec3(0.56, 0.25, 0.75);

    vec3 a = red;
    vec3 b = orange;
    if (index == 1) {
        a = orange;
        b = yellow;
    } else if (index == 2) {
        a = yellow;
        b = green;
    } else if (index == 3) {
        a = green;
        b = blue;
    } else if (index == 4) {
        a = blue;
        b = purple;
    } else if (index >= 5) {
        a = purple;
        b = purple;
    }

    return mix(a, b, blend);
}

void main() {
    float bandPos = fragWorldPos.y * %.3f;
    float diagonal = (fragWorldPos.x + fragWorldPos.z) * %.3f;
    vec3 rainbow = RainbowBand(bandPos + diagonal);
    vec3 baseColor = mix(rainbow, fragColor, %.3f);

    vec3 normal = ComputeNormal();
    vec3 lighting = vec3(0.0);

    vec3 keyDir = normalize(%s);
    vec3 fillDir = normalize(%s);
    float keyNdotL = abs(dot(normal, keyDir));
    float fillNdotL = abs(dot(normal, fillDir));

    lighting += vec3(1.0, 0.92, 0.85) * keyNdotL * %.3f;
    lighting += vec3(0.35, 0.45, 0.65) * fillNdotL * %.3f;

    vec3 finalColor = baseColor * (%.3f + lighting);
    outColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}
]], band_scale, diagonal_scale, mix_factor, key_dir, fill_dir, key_intensity, fill_intensity,
    ambient_strength)
end

local function build_solid_lit_fragment_source(options)
    local light_color = format_vec3(normalize_vec3(options.light_color, {1.0, 0.9, 0.6}))
    local light_intensity = normalize_number(options.light_intensity, 0.9)
    local ambient_strength = normalize_number(options.ambient_strength, 0.22)

    return string.format([[
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

// Lantern positions (8 lights)
const vec3 LIGHT_POSITIONS[8] = vec3[8](
    vec3(13.0, 4.5, 13.0),    // Corner
    vec3(-13.0, 4.5, 13.0),   // Corner
    vec3(13.0, 4.5, -13.0),   // Corner
    vec3(-13.0, 4.5, -13.0),  // Corner
    vec3(0.0, 4.5, 13.0),     // Wall midpoint
    vec3(0.0, 4.5, -13.0),    // Wall midpoint
    vec3(13.0, 4.5, 0.0),     // Wall midpoint
    vec3(-13.0, 4.5, 0.0)     // Wall midpoint
);

const vec3 LIGHT_COLOR = %s;  // Warm lantern color
const float LIGHT_INTENSITY = %.3f;
const float AMBIENT_STRENGTH = %.3f;  // Boost ambient to preserve surface color

float calculateAttenuation(float distance) {
    // Quadratic attenuation: 1 / (constant + linear*d + quadratic*d^2)
    const float kConstant = 1.0;
    const float kLinear = 0.09;
    const float kQuadratic = 0.032;
    return 1.0 / (kConstant + kLinear * distance + kQuadratic * distance * distance);
}

void main() {
    vec3 ambient = AMBIENT_STRENGTH * fragColor;
    vec3 lighting = vec3(0.0);

    // Calculate contribution from each lantern
    for (int i = 0; i < 8; i++) {
        vec3 lightDir = LIGHT_POSITIONS[i] - fragWorldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);

        // Distance attenuation
        float attenuation = calculateAttenuation(distance);

        // Add light contribution
        lighting += LIGHT_COLOR * LIGHT_INTENSITY * attenuation;
    }

    // Combine ambient and dynamic lighting with surface color
    vec3 finalColor = ambient + fragColor * lighting;

    // Clamp to prevent over-bright areas
    finalColor = clamp(finalColor, 0.0, 1.0);

    outColor = vec4(finalColor, 1.0);
}
]], light_color, light_intensity, ambient_strength)
end

local function build_floor_fragment_source(options)
    local surface_base = format_vec3(normalize_vec3(options.surface_base, {0.02, 0.95, 0.72}))
    local checker_scale = normalize_number(options.checker_scale, 0.55)
    local light_color = format_vec3(normalize_vec3(options.light_color, {1.0, 0.92, 0.7}))
    local light_intensity = normalize_number(options.light_intensity, 1.0)
    local ambient_strength = normalize_number(options.ambient_strength, 0.3)

    return string.format([[
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

const vec3 SURFACE_BASE = %s;

vec3 ComputeNormal() {
    vec3 dx = dFdx(fragWorldPos);
    vec3 dy = dFdy(fragWorldPos);
    return normalize(cross(dx, dy));
}

const vec3 LIGHT_POSITIONS[8] = vec3[8](
    vec3(13.0, 4.5, 13.0),
    vec3(-13.0, 4.5, 13.0),
    vec3(13.0, 4.5, -13.0),
    vec3(-13.0, 4.5, -13.0),
    vec3(0.0, 4.5, 13.0),
    vec3(0.0, 4.5, -13.0),
    vec3(13.0, 4.5, 0.0),
    vec3(-13.0, 4.5, 0.0)
);

const vec3 LIGHT_COLOR = %s;
const float LIGHT_INTENSITY = %.3f;
const float AMBIENT_STRENGTH = %.3f;

float calculateAttenuation(float distance) {
    const float kConstant = 1.0;
    const float kLinear = 0.09;
    const float kQuadratic = 0.032;
    return 1.0 / (kConstant + kLinear * distance + kQuadratic * distance * distance);
}

void main() {
    vec3 baseColor = SURFACE_BASE;
    float checkerScale = %.3f;
    float cx = step(0.5, fract(fragWorldPos.x * checkerScale));
    float cz = step(0.5, fract(fragWorldPos.z * checkerScale));
    float checker = abs(cx - cz);
    float grit = hash(floor(fragWorldPos.xz * 2.2));
    float pattern = mix(0.82, 1.08, checker) * mix(0.96, 1.04, grit);
    baseColor *= pattern;

    vec3 normal = ComputeNormal();
    vec3 ambient = AMBIENT_STRENGTH * baseColor;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < 8; i++) {
        vec3 lightDir = LIGHT_POSITIONS[i] - fragWorldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        float attenuation = calculateAttenuation(distance);
        float ndotl = abs(dot(normal, lightDir));
        lighting += LIGHT_COLOR * LIGHT_INTENSITY * attenuation * ndotl;
    }

    vec3 keyDir = normalize(vec3(-0.3, 1.0, -0.4));
    float keyNdotL = abs(dot(normal, keyDir));
    lighting += vec3(0.9, 0.95, 1.0) * keyNdotL * 0.25;

    vec3 finalColor = ambient + baseColor * lighting;
    finalColor = clamp(finalColor, 0.0, 1.0);

    outColor = vec4(finalColor, 1.0);
}
]], surface_base, light_color, light_intensity, ambient_strength, checker_scale)
end

local function build_wall_fragment_source(options)
    local surface_base = format_vec3(normalize_vec3(options.surface_base, {0.98, 0.18, 0.08}))
    local plank_scale = normalize_number(options.plank_scale, 0.4)
    local light_color = format_vec3(normalize_vec3(options.light_color, {1.0, 0.88, 0.6}))
    local light_intensity = normalize_number(options.light_intensity, 0.95)
    local ambient_strength = normalize_number(options.ambient_strength, 0.24)

    return string.format([[
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(91.7, 127.3))) * 43758.5453123);
}

const vec3 SURFACE_BASE = %s;

vec3 ComputeNormal() {
    vec3 dx = dFdx(fragWorldPos);
    vec3 dy = dFdy(fragWorldPos);
    return normalize(cross(dx, dy));
}

const vec3 LIGHT_POSITIONS[8] = vec3[8](
    vec3(13.0, 4.5, 13.0),
    vec3(-13.0, 4.5, 13.0),
    vec3(13.0, 4.5, -13.0),
    vec3(-13.0, 4.5, -13.0),
    vec3(0.0, 4.5, 13.0),
    vec3(0.0, 4.5, -13.0),
    vec3(13.0, 4.5, 0.0),
    vec3(-13.0, 4.5, 0.0)
);

const vec3 LIGHT_COLOR = %s;
const float LIGHT_INTENSITY = %.3f;
const float AMBIENT_STRENGTH = %.3f;

float calculateAttenuation(float distance) {
    const float kConstant = 1.0;
    const float kLinear = 0.09;
    const float kQuadratic = 0.032;
    return 1.0 / (kConstant + kLinear * distance + kQuadratic * distance * distance);
}

void main() {
    vec3 baseColor = SURFACE_BASE;
    float axisSelector = step(abs(fragWorldPos.z), abs(fragWorldPos.x));
    float coord = mix(fragWorldPos.z, fragWorldPos.x, axisSelector);
    float plankScale = %.3f;
    float plank = abs(fract(coord * plankScale) - 0.5);
    float groove = smoothstep(0.46, 0.5, plank);
    float grain = hash(floor(vec2(coord * 1.2, fragWorldPos.y * 2.0)));
    baseColor *= mix(0.92, 1.05, grain);
    baseColor *= mix(1.0, 0.78, groove);

    vec3 normal = ComputeNormal();
    vec3 ambient = AMBIENT_STRENGTH * baseColor;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < 8; i++) {
        vec3 lightDir = LIGHT_POSITIONS[i] - fragWorldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        float attenuation = calculateAttenuation(distance);
        float ndotl = abs(dot(normal, lightDir));
        lighting += LIGHT_COLOR * LIGHT_INTENSITY * attenuation * ndotl;
    }

    vec3 keyDir = normalize(vec3(0.35, 0.9, 0.2));
    float keyNdotL = abs(dot(normal, keyDir));
    lighting += vec3(1.0, 0.9, 0.8) * keyNdotL * 0.2;

    vec3 finalColor = ambient + baseColor * lighting;
    finalColor = clamp(finalColor, 0.0, 1.0);

    outColor = vec4(finalColor, 1.0);
}
]], surface_base, light_color, light_intensity, ambient_strength, plank_scale)
end

local function build_ceiling_fragment_source(options)
    local surface_base = format_vec3(normalize_vec3(options.surface_base, {0.98, 0.96, 0.1}))
    local grid_scale = normalize_number(options.grid_scale, 0.45)
    local light_color = format_vec3(normalize_vec3(options.light_color, {0.92, 0.96, 1.0}))
    local light_intensity = normalize_number(options.light_intensity, 0.85)
    local ambient_strength = normalize_number(options.ambient_strength, 0.38)

    return string.format([[
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(63.1, 157.9))) * 43758.5453123);
}

const vec3 SURFACE_BASE = %s;

vec3 ComputeNormal() {
    vec3 dx = dFdx(fragWorldPos);
    vec3 dy = dFdy(fragWorldPos);
    return normalize(cross(dx, dy));
}

const vec3 LIGHT_POSITIONS[8] = vec3[8](
    vec3(13.0, 4.5, 13.0),
    vec3(-13.0, 4.5, 13.0),
    vec3(13.0, 4.5, -13.0),
    vec3(-13.0, 4.5, -13.0),
    vec3(0.0, 4.5, 13.0),
    vec3(0.0, 4.5, -13.0),
    vec3(13.0, 4.5, 0.0),
    vec3(-13.0, 4.5, 0.0)
);

const vec3 LIGHT_COLOR = %s;
const float LIGHT_INTENSITY = %.3f;
const float AMBIENT_STRENGTH = %.3f;

float calculateAttenuation(float distance) {
    const float kConstant = 1.0;
    const float kLinear = 0.09;
    const float kQuadratic = 0.032;
    return 1.0 / (kConstant + kLinear * distance + kQuadratic * distance * distance);
}

void main() {
    vec3 baseColor = SURFACE_BASE;
    vec2 gridUv = fragWorldPos.xz * %.3f;
    vec2 grid = abs(fract(gridUv) - 0.5);
    float gridLine = step(0.48, max(grid.x, grid.y));
    float speckle = hash(floor(fragWorldPos.xz * 3.0));
    baseColor *= mix(0.94, 1.04, speckle);
    baseColor *= mix(1.0, 0.84, gridLine);

    vec3 normal = ComputeNormal();
    vec3 ambient = AMBIENT_STRENGTH * baseColor;
    vec3 lighting = vec3(0.0);

    for (int i = 0; i < 8; i++) {
        vec3 lightDir = LIGHT_POSITIONS[i] - fragWorldPos;
        float distance = length(lightDir);
        lightDir = normalize(lightDir);
        float attenuation = calculateAttenuation(distance);
        float ndotl = abs(dot(normal, lightDir));
        lighting += LIGHT_COLOR * LIGHT_INTENSITY * attenuation * ndotl;
    }

    vec3 keyDir = normalize(vec3(-0.15, 1.0, 0.25));
    float keyNdotL = abs(dot(normal, keyDir));
    lighting += vec3(0.85, 0.95, 1.0) * keyNdotL * 0.28;

    vec3 finalColor = ambient + baseColor * lighting;
    finalColor = clamp(finalColor, 0.0, 1.0);

    outColor = vec4(finalColor, 1.0);
}
]], surface_base, light_color, light_intensity, ambient_strength, grid_scale)
end

local pbr_vertex_source = [[
#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 viewProj;
    // Extended fields for PBR/atmospherics
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pc;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition, 1.0);
    gl_Position = pc.proj * pc.view * worldPos;

    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(mat3(pc.model) * inNormal);
    fragTexCoord = vec2(0.0, 0.0);
    fragColor = inColor;  // Use vertex color
}
]]

local function build_pbr_fragment_source(options)
    local material_albedo = format_vec3(normalize_vec3(options.material_albedo, {0.8, 0.8, 0.8}))
    local material_roughness = normalize_number(options.material_roughness, 0.3)
    local material_metallic = normalize_number(options.material_metallic, 0.1)
    local light_color = format_vec3(normalize_vec3(options.light_color, {1.0, 0.9, 0.6}))
    local light_intensity = normalize_number(options.light_intensity, 1.2)

    return string.format([[
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

// Material properties
layout(push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 lightViewProj;
    vec3 cameraPos;
    float time;
    // Atmospherics parameters
    float ambientStrength;
    float fogDensity;
    float fogStart;
    float fogEnd;
    vec3 fogColor;
    float gamma;
    float exposure;
    int enableShadows;
    int enableFog;
} pc;

// Lighting uniforms
// layout(set = 0, binding = 0) uniform sampler2D shadowMap;

// Material parameters (can be extended to use textures)
const vec3 MATERIAL_ALBEDO = %s;
const float MATERIAL_ROUGHNESS = %.3f;
const float MATERIAL_METALLIC = %.3f;

// Atmospheric parameters
// const vec3 FOG_COLOR = vec3(0.05, 0.05, 0.08);
// const float FOG_DENSITY = 0.003;
// const float AMBIENT_STRENGTH = 0.01;  // Much darker ambient

// Light properties
const vec3 LIGHT_COLOR = %s;
const float LIGHT_INTENSITY = %.3f;
const vec3 LIGHT_POSITIONS[8] = vec3[8](
    vec3(13.0, 4.5, 13.0),
    vec3(-13.0, 4.5, 13.0),
    vec3(13.0, 4.5, -13.0),
    vec3(-13.0, 4.5, -13.0),
    vec3(0.0, 4.5, 13.0),
    vec3(0.0, 4.5, -13.0),
    vec3(13.0, 4.5, 0.0),
    vec3(-13.0, 4.5, 0.0)
);

// PBR functions
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159 * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float calculateShadow(vec3 worldPos) {
    // TODO: Implement shadow mapping
    return 0.0;  // No shadow for now
}

vec3 calculateLighting(vec3 worldPos, vec3 normal, vec3 viewDir) {
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, MATERIAL_ALBEDO, MATERIAL_METALLIC);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < 8; i++) {
        vec3 lightDir = normalize(LIGHT_POSITIONS[i] - worldPos);
        vec3 halfway = normalize(viewDir + lightDir);

        float distance = length(LIGHT_POSITIONS[i] - worldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = LIGHT_COLOR * LIGHT_INTENSITY * attenuation;

        float NDF = DistributionGGX(normal, halfway, MATERIAL_ROUGHNESS);
        float G = GeometrySmith(normal, viewDir, lightDir, MATERIAL_ROUGHNESS);
        vec3 F = fresnelSchlick(max(dot(halfway, viewDir), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - MATERIAL_METALLIC;

        float NdotL = max(dot(normal, lightDir), 0.0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;

        Lo += (kD * MATERIAL_ALBEDO / 3.14159 + specular) * radiance * NdotL;
    }

    return Lo;
}

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(pc.cameraPos - fragWorldPos);

    // Ambient lighting
    vec3 ambient = pc.ambientStrength * MATERIAL_ALBEDO;

    // Direct lighting with PBR
    vec3 lighting = calculateLighting(fragWorldPos, normal, viewDir);

    // Shadow calculation
    float shadow = calculateShadow(fragWorldPos);
    lighting *= (1.0 - shadow * 0.7);  // Soften shadows

    // Combine lighting
    vec3 finalColor = ambient + lighting;

    // Fog
    if (pc.enableFog != 0) {
        float fogFactor = 1.0 - exp(-pc.fogDensity * length(pc.cameraPos - fragWorldPos));
        finalColor = mix(finalColor, pc.fogColor, fogFactor);
    }

    // Simple tone mapping (Reinhard)
    finalColor = finalColor / (finalColor + vec3(1.0));

    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0 / pc.gamma));

    outColor = vec4(finalColor, 1.0);
}
]], material_albedo, material_roughness, material_metallic, light_color, light_intensity)
end

local function build_vertex_color_sources()
    return {
        vertex = vertex_color_source,
        fragment = vertex_color_fragment_source,
    }
end

local function build_gui_2d_sources()
    return {
        vertex = gui_2d_vertex_source,
        fragment = gui_2d_fragment_source,
    }
end

local function build_solid_color_sources(options)
    local color = normalize_color(options.color)
    local fragment = string.format([[
#version 450

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(%.3f, %.3f, %.3f, %.3f);
}
]], color[1], color[2], color[3], color[4])
    return {
        vertex = vertex_color_source,
        fragment = fragment,
    }
end

local function build_shadow_sources()
    return {
        vertex = shadow_vertex_source,
        fragment = shadow_fragment_source,
    }
end

local function build_cube_rainbow_sources(options)
    return {
        vertex = vertex_world_color_source,
        fragment = build_cube_rainbow_fragment_source(options),
    }
end

local function build_solid_lit_sources(options)
    return {
        vertex = vertex_world_color_source,
        fragment = build_solid_lit_fragment_source(options),
    }
end

local function build_floor_sources(options)
    return {
        vertex = vertex_world_color_source,
        fragment = build_floor_fragment_source(options),
    }
end

local function build_wall_sources(options)
    return {
        vertex = vertex_world_color_source,
        fragment = build_wall_fragment_source(options),
    }
end

local function build_ceiling_sources(options)
    return {
        vertex = vertex_world_color_source,
        fragment = build_ceiling_fragment_source(options),
    }
end

local function build_pbr_sources(options)
    return {
        vertex = pbr_vertex_source,
        fragment = build_pbr_fragment_source(options),
    }
end

local function build_ssgi_sources()
    return {
        vertex = fullscreen_vertex_source,
        fragment = ssgi_fragment_source,
    }
end

local function build_volumetric_sources()
    return {
        vertex = fullscreen_vertex_source,
        fragment = volumetric_fragment_source,
    }
end

local templates = {}

local function register_template_object(template)
    if type(template) ~= "table" or type(template.name) ~= "string" or template.name == "" then
        error("Shader template must include a non-empty name")
    end
    if type(template.Generate) ~= "function" then
        error("Shader template must implement Generate(options)")
    end
    templates[template.name] = template
end

register_template_object(ShaderTemplate:new("vertex_color", build_vertex_color_sources))
register_template_object(ShaderTemplate:new("gui_2d", build_gui_2d_sources))
register_template_object(ShaderTemplate:new("solid_color", build_solid_color_sources, {
    color = {1.0, 1.0, 1.0, 1.0},
}))
register_template_object(ShaderTemplate:new("shadow", build_shadow_sources))
register_template_object(ShaderTemplate:new("cube_rainbow", build_cube_rainbow_sources, {
    band_scale = 0.35,
    diagonal_scale = 0.25,
    mix_factor = 0.08,
    ambient_strength = 0.28,
    key_intensity = 0.9,
    fill_intensity = 0.4,
    key_dir = {-0.35, 1.0, -0.25},
    fill_dir = {0.45, 0.6, 0.2},
}))
register_template_object(ShaderTemplate:new("solid_lit", build_solid_lit_sources, {
    light_color = {1.0, 0.9, 0.6},
    light_intensity = 0.9,
    ambient_strength = 0.22,
}))
register_template_object(ShaderTemplate:new("room_floor", build_floor_sources, {
    surface_base = {0.02, 0.95, 0.72},
    checker_scale = 0.55,
    light_color = {1.0, 0.92, 0.7},
    light_intensity = 1.0,
    ambient_strength = 0.3,
}))
register_template_object(ShaderTemplate:new("room_wall", build_wall_sources, {
    surface_base = {0.98, 0.18, 0.08},
    plank_scale = 0.4,
    light_color = {1.0, 0.88, 0.6},
    light_intensity = 0.95,
    ambient_strength = 0.24,
}))
register_template_object(ShaderTemplate:new("room_ceiling", build_ceiling_sources, {
    surface_base = {0.98, 0.96, 0.1},
    grid_scale = 0.45,
    light_color = {0.92, 0.96, 1.0},
    light_intensity = 0.85,
    ambient_strength = 0.38,
}))
register_template_object(ShaderTemplate:new("pbr", build_pbr_sources, {
    material_albedo = {0.8, 0.8, 0.8},
    material_roughness = 0.3,
    material_metallic = 0.1,
    light_color = {1.0, 0.9, 0.6},
    light_intensity = 1.2,
}))
register_template_object(ShaderTemplate:new("ssgi", build_ssgi_sources))
register_template_object(ShaderTemplate:new("volumetric", build_volumetric_sources))

shader_toolkit.templates = templates

shader_toolkit.ShaderTemplate = ShaderTemplate
shader_toolkit.ShaderVariant = ShaderVariant

local function is_template_object(value)
    return type(value) == "table"
        and type(value.name) == "string"
        and value.name ~= ""
        and type(value.Generate) == "function"
end

local function resolve_template(template_ref)
    if type(template_ref) == "string" then
        return templates[template_ref]
    end
    if is_template_object(template_ref) then
        return template_ref
    end
    return nil
end

local function resolve_variant(value)
    if getmetatable(value) == ShaderVariant then
        return value
    end
    return ShaderVariant:new(value)
end

function shader_toolkit.create_template(name, generator, defaults)
    return ShaderTemplate:new(name, generator, defaults)
end

function shader_toolkit.create_variant(options)
    return ShaderVariant:new(options)
end

function shader_toolkit.register_template(name_or_template, generator, defaults)
    local template = name_or_template
    if type(name_or_template) == "string" then
        template = ShaderTemplate:new(name_or_template, generator, defaults)
    end
    if not is_template_object(template) then
        error("Template registration requires a ShaderTemplate or name/generator pair")
    end
    register_template_object(template)
end

function shader_toolkit.list_templates()
    local names = {}
    for name in pairs(templates) do
        names[#names + 1] = name
    end
    table.sort(names)
    return names
end

function shader_toolkit.generate_variant(options)
    local variant = resolve_variant(options)
    local template = resolve_template(variant.template)
    if not template then
        error("Unknown shader template: " .. tostring(variant.template))
    end
    local output_mode = variant.output_mode or "source"
    local output_name = normalize_output_name(variant.output_name or variant.key or template.name)
    if not output_name then
        error("Shader variant requires output_name or key")
    end

    local sources = template:Generate(variant.parameters)
    if type(sources) ~= "table" or not sources.vertex or not sources.fragment then
        error("Shader template did not return vertex and fragment source")
    end

    if output_mode ~= "files" then
        if variant.compile == true then
            error("Inline shader output does not support external compilation")
        end
        local result = {}
        for stage, source in pairs(sources) do
            if type(source) == "string" then
                result[stage .. "_source"] = source
            end
        end
        return result
    end

    local output_dir = resolve_output_dir(variant.output_dir)
    if not ensure_directory(output_dir) then
        error("Failed to create shader output directory: " .. output_dir)
    end

    local base_name = path_join(output_dir, output_name)
    local vertex_source = base_name .. ".vert"
    local fragment_source = base_name .. ".frag"
    local vertex_spv = vertex_source .. ".spv"
    local fragment_spv = fragment_source .. ".spv"

    write_text_file_if_changed(vertex_source, sources.vertex)
    write_text_file_if_changed(fragment_source, sources.fragment)

    local compile = variant.compile ~= false
    if compile then
        local compiler = variant.compiler or detect_compiler()
        if not compiler then
            error("No shader compiler found. Install glslangValidator or glslc, or pass options.compiler")
        end
        local skip_if_present = variant.skip_if_present == true
        if not (skip_if_present and file_exists(vertex_spv)) then
            compile_shader(compiler, vertex_source, vertex_spv)
        end
        if not (skip_if_present and file_exists(fragment_spv)) then
            compile_shader(compiler, fragment_source, fragment_spv)
        end
    end

    if compile then
        return {vertex = vertex_spv, fragment = fragment_spv}
    end
    return {vertex = vertex_source, fragment = fragment_source}
end

function shader_toolkit.generate_variants(variant_list)
    if type(variant_list) ~= "table" then
        error("Shader variants must be a list of tables")
    end
    local result = {}
    for index, entry in ipairs(variant_list) do
        if type(entry) ~= "table" then
            error("Shader variant at index " .. index .. " must be a table")
        end
        local variant = resolve_variant(entry)
        local key = variant.key
        if type(key) ~= "string" or key == "" then
            error("Shader variant at index " .. index .. " must include a non-empty key")
        end
        result[key] = shader_toolkit.generate_variant(variant)
    end
    return result
end

function shader_toolkit.generate_cube_demo_variants(options)
    local settings = options or {}
    local parameters = settings.parameters or {}
    local output_mode = settings.output_mode or "source"
    local compile = settings.compile
    if compile == nil then
        compile = false
    end
    local output_dir = settings.output_dir

    local variants = {
        shader_toolkit.create_variant({
            key = "default",
            template = "cube_rainbow",
            output_name = "cube",
            output_mode = output_mode,
            compile = compile,
            output_dir = output_dir,
            parameters = parameters.default,
        }),
        shader_toolkit.create_variant({
            key = "solid",
            template = "solid_lit",
            output_mode = output_mode,
            compile = compile,
            output_dir = output_dir,
            parameters = parameters.solid,
        }),
        shader_toolkit.create_variant({
            key = "floor",
            template = "room_floor",
            output_mode = output_mode,
            compile = compile,
            output_dir = output_dir,
            parameters = parameters.floor,
        }),
        shader_toolkit.create_variant({
            key = "wall",
            template = "room_wall",
            output_mode = output_mode,
            compile = compile,
            output_dir = output_dir,
            parameters = parameters.wall,
        }),
        shader_toolkit.create_variant({
            key = "ceiling",
            template = "room_ceiling",
            output_mode = output_mode,
            compile = compile,
            output_dir = output_dir,
            parameters = parameters.ceiling,
        }),
        shader_toolkit.create_variant({
            key = "pbr",
            template = "pbr",
            output_mode = output_mode,
            compile = compile,
            output_dir = output_dir,
            parameters = parameters.pbr,
        }),
    }

    return shader_toolkit.generate_variants(variants)
end

return shader_toolkit
