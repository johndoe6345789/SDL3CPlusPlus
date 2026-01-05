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

local function normalize_color(color)
    if type(color) ~= "table" then
        return {1.0, 1.0, 1.0, 1.0}
    end
    local r = tonumber(color[1]) or 1.0
    local g = tonumber(color[2]) or 1.0
    local b = tonumber(color[3]) or 1.0
    local a = tonumber(color[4]) or 1.0
    return {r, g, b, a}
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
layout(location = 1) in vec3 inColor;

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

local templates = {
    vertex_color = function()
        return {
            vertex = vertex_color_source,
            fragment = vertex_color_fragment_source,
        }
    end,
    solid_color = function(options)
        local color = normalize_color(options and options.color)
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
    end,
}

shader_toolkit.templates = templates

function shader_toolkit.register_template(name, generator)
    if type(name) ~= "string" or name == "" then
        error("Template name must be a non-empty string")
    end
    if type(generator) ~= "function" then
        error("Template generator must be a function")
    end
    templates[name] = generator
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
    if type(options) ~= "table" then
        error("Shader variant options must be a table")
    end
    local template_name = options.template
    if type(template_name) ~= "string" or template_name == "" then
        error("Shader variant template must be a non-empty string")
    end
    local template = templates[template_name]
    if not template then
        error("Unknown shader template: " .. template_name)
    end
    local output_name = normalize_output_name(options.output_name or options.key or template_name)
    if not output_name then
        error("Shader variant requires output_name or key")
    end
    local output_dir = resolve_output_dir(options.output_dir)
    if not ensure_directory(output_dir) then
        error("Failed to create shader output directory: " .. output_dir)
    end

    local base_name = path_join(output_dir, output_name)
    local vertex_source = base_name .. ".vert"
    local fragment_source = base_name .. ".frag"
    local vertex_spv = vertex_source .. ".spv"
    local fragment_spv = fragment_source .. ".spv"

    local sources = template(options)
    if type(sources) ~= "table" or not sources.vertex or not sources.fragment then
        error("Shader template did not return vertex and fragment source")
    end

    write_text_file(vertex_source, sources.vertex)
    write_text_file(fragment_source, sources.fragment)

    local compile = options.compile ~= false
    if compile then
        local compiler = options.compiler or detect_compiler()
        if not compiler then
            error("No shader compiler found. Install glslangValidator or glslc, or pass options.compiler")
        end
        local skip_if_present = options.skip_if_present == true
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
    for index, options in ipairs(variant_list) do
        if type(options) ~= "table" then
            error("Shader variant at index " .. index .. " must be a table")
        end
        local key = options.key
        if type(key) ~= "string" or key == "" then
            error("Shader variant at index " .. index .. " must include a non-empty key")
        end
        result[key] = shader_toolkit.generate_variant(options)
    end
    return result
end

return shader_toolkit
