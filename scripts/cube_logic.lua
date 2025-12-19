local pyramid_vertices = {
    { position = {0.0, 1.0, 0.0}, color = {1.0, 0.5, 0.0} },
    { position = {-1.0, -1.0, -1.0}, color = {0.0, 1.0, 1.0} },
    { position = {1.0, -1.0, -1.0}, color = {1.0, 0.0, 1.0} },
    { position = {1.0, -1.0, 1.0}, color = {1.0, 1.0, 0.0} },
    { position = {-1.0, -1.0, 1.0}, color = {0.0, 0.0, 1.0} },
}

local pyramid_indices = {
    1, 2, 3,
    1, 3, 4,
    1, 4, 5,
    1, 5, 2,
    2, 3, 4,
    4, 5, 2,
}

local fallback_cube_vertices = {
    { position = {-1.0, -1.0, -1.0}, color = {1.0, 0.0, 0.0} },
    { position = {1.0, -1.0, -1.0}, color = {0.0, 1.0, 0.0} },
    { position = {1.0, 1.0, -1.0}, color = {0.0, 0.0, 1.0} },
    { position = {-1.0, 1.0, -1.0}, color = {1.0, 1.0, 0.0} },
    { position = {-1.0, -1.0, 1.0}, color = {1.0, 0.0, 1.0} },
    { position = {1.0, -1.0, 1.0}, color = {0.0, 1.0, 1.0} },
    { position = {1.0, 1.0, 1.0}, color = {1.0, 1.0, 1.0} },
    { position = {-1.0, 1.0, 1.0}, color = {0.2, 0.2, 0.2} },
}

local fallback_cube_indices = {
    1, 2, 3, 3, 4, 1, -- back
    5, 6, 7, 7, 8, 5, -- front
    1, 5, 8, 8, 4, 1, -- left
    2, 6, 7, 7, 3, 2, -- right
    4, 3, 7, 7, 8, 4, -- top
    1, 2, 6, 6, 5, 1, -- bottom
}

local function resolve_script_directory()
    local info = debug.getinfo(1, "S")
    local source = info and info.source
    if not source then
        return "."
    end
    if source:sub(1, 1) == "@" then
        local path = source:sub(2)
        return path:match("^(.*[\\/])") or "."
    end
    return "."
end

local function combine_paths(first, ...)
    local parts = {first, ...}
    local combined = parts[1] or ""
    for i = 2, #parts do
        local piece = parts[i]
        if piece and piece ~= "" then
            local last = combined:sub(-1)
            if last ~= "/" and last ~= "\\" then
                combined = combined .. "/"
            end
            combined = combined .. piece
        end
    end
    return combined
end

local function load_stl_mesh(path)
    local file, err = io.open(path, "r")
    if not file then
        return nil, nil, err
    end
    local vertices = {}
    local indices = {}
    local color = {0.6, 0.8, 1.0}
    local vertex_count = 0
    for line in file:lines() do
        if line:lower():match("^%s*vertex") then
            local coords = {}
            for token in line:gmatch("%S+") do
                if token:lower() ~= "vertex" then
                    local value = tonumber(token)
                    if value then
                        coords[#coords + 1] = value
                    end
                end
            end
            if #coords == 3 then
                vertex_count = vertex_count + 1
                vertices[vertex_count] = {
                    position = {coords[1], coords[2], coords[3]},
                    color = {color[1], color[2], color[3]},
                }
                indices[#indices + 1] = vertex_count
            end
        end
    end
    file:close()
    if vertex_count == 0 then
        return nil, nil, "STL did not include any vertex lines"
    end
    return vertices, indices, nil
end

local script_directory = resolve_script_directory()
local stl_cube_path = combine_paths(script_directory, "models", "cube.stl")
local stl_vertices, stl_indices, stl_error = load_stl_mesh(stl_cube_path)

local cube_vertices = fallback_cube_vertices
local cube_indices = fallback_cube_indices
local stl_debug_info = {
    path = stl_cube_path,
    loaded = false,
    vertex_count = 0,
    index_count = 0,
    error = stl_error,
}

if stl_vertices then
    cube_vertices = stl_vertices
    cube_indices = stl_indices
    stl_debug_info.loaded = true
    stl_debug_info.vertex_count = #stl_vertices
    stl_debug_info.index_count = #stl_indices
else
    stl_debug_info.error = stl_error or "STL file not available"
end

local math3d = require("math3d")
local string_format = string.format
local table_concat = table.concat

local InputState = {}
InputState.__index = InputState

function InputState:new()
    local instance = {
        mouseX = 0.0,
        mouseY = 0.0,
        mouseDown = false,
        wheel = 0.0,
        textInput = "",
        keyStates = {},
    }
    return setmetatable(instance, InputState)
end

function InputState:resetTransient()
    self.textInput = ""
    self.wheel = 0.0
end

function InputState:setMouse(x, y, isDown)
    self.mouseX = x
    self.mouseY = y
    self.mouseDown = isDown
end

function InputState:setWheel(deltaY)
    self.wheel = deltaY
end

function InputState:setKey(keyName, isDown)
    self.keyStates[keyName] = isDown
end

function InputState:addTextInput(text)
    if text then
        self.textInput = self.textInput .. text
    end
end

gui_input = InputState:new()

local function log_debug(fmt, ...)
    if not lua_debug or not fmt then
        return
    end
    print(string_format(fmt, ...))
end

if stl_debug_info.loaded then
    log_debug("Loaded cube mesh from %s (%d vertices, %d indices)",
        stl_debug_info.path, stl_debug_info.vertex_count, stl_debug_info.index_count)
else
    log_debug("Failed to load cube STL (%s); using fallback cube",
        stl_debug_info.error or "unknown")
end

local rotation_speeds = {x = 0.5, y = 0.7}

local shader_variants = {
    default = {
        vertex = "shaders/cube.vert.spv",
        fragment = "shaders/cube.frag.spv",
    },
    cube = {
        vertex = "shaders/cube.vert.spv",
        fragment = "shaders/cube.frag.spv",
    },
    pyramid = {
        vertex = "shaders/cube.vert.spv",
        fragment = "shaders/cube.frag.spv",
    },
}

local camera = {
    eye = {2.0, 2.0, 2.5},
    center = {0.0, 0.0, 0.0},
    up = {0.0, 1.0, 0.0},
    fov = 0.78,
    near = 0.1,
    far = 10.0,
}

local zoom_settings = {
    min_distance = 2.0,
    max_distance = 12.0,
    speed = 0.25,
}

local function clamp_distance(value, minValue, maxValue)
    if minValue and value < minValue then
        return minValue
    end
    if maxValue and value > maxValue then
        return maxValue
    end
    return value
end

local function update_camera_zoom(delta)
    if delta == 0 then
        return
    end
    local dx = camera.eye[1] - camera.center[1]
    local dy = camera.eye[2] - camera.center[2]
    local dz = camera.eye[3] - camera.center[3]
    local distance = math.sqrt(dx * dx + dy * dy + dz * dz)
    if distance == 0 then
        return
    end
    local normalizedX = dx / distance
    local normalizedY = dy / distance
    local normalizedZ = dz / distance
    local adjustment = -delta * zoom_settings.speed
    local targetDistance = clamp_distance(distance + adjustment, zoom_settings.min_distance, zoom_settings.max_distance)
    camera.eye[1] = camera.center[1] + normalizedX * targetDistance
    camera.eye[2] = camera.center[2] + normalizedY * targetDistance
    camera.eye[3] = camera.center[3] + normalizedZ * targetDistance
    log_debug("zoom delta=%.2f -> distance=%.2f", delta, targetDistance)
end

local function build_model(time)
    local y = math3d.rotation_y(time * rotation_speeds.y)
    local x = math3d.rotation_x(time * rotation_speeds.x)
    return math3d.multiply(y, x)
end

local function create_cube(position, speed_scale, shader_key)
    local function compute_model_matrix(time)
        local base = build_model(time * speed_scale)
        local offset = math3d.translation(position[1], position[2], position[3])
        return math3d.multiply(offset, base)
    end

    return {
        vertices = cube_vertices,
        indices = cube_indices,
        compute_model_matrix = compute_model_matrix,
        shader_key = shader_key or "cube",
    }
end

local function create_pyramid(position, shader_key)
    local function compute_model_matrix(time)
        local base = build_model(time * 0.6)
        local offset = math3d.translation(position[1], position[2], position[3])
        return math3d.multiply(offset, base)
    end

    return {
        vertices = pyramid_vertices,
        indices = pyramid_indices,
        compute_model_matrix = compute_model_matrix,
        shader_key = shader_key or "pyramid",
    }
end

function get_scene_objects()
    local objects = {
        create_cube({0.0, 0.0, 0.0}, 1.0, "cube"),
        create_cube({3.0, 0.0, 0.0}, 0.8, "cube"),
        create_cube({-3.0, 0.0, 0.0}, 1.2, "cube"),
        create_pyramid({0.0, -0.5, -4.0}, "pyramid"),
    }
    if lua_debug then
        local labels = {}
        for idx, obj in ipairs(objects) do
            table.insert(labels, string_format("[%d:%s]", idx, obj.shader_key))
        end
        log_debug("get_scene_objects -> %d entries: %s", #objects, table_concat(labels, ", "))
    end
    return objects
end

function get_shader_paths()
    return shader_variants
end

function get_view_projection(aspect)
    if gui_input then
        update_camera_zoom(gui_input.wheel)
    end
    local view = math3d.look_at(camera.eye, camera.center, camera.up)
    local projection = math3d.perspective(camera.fov, aspect, camera.near, camera.far)
    return math3d.multiply(projection, view)
end
