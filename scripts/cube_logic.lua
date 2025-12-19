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

local cube_mesh_info = {
    path = "models/cube.stl",
    loaded = false,
    vertex_count = 0,
    index_count = 0,
    error = "load_mesh_from_file() not registered",
}

local cube_vertices = {}
local cube_indices = {}

local function load_cube_mesh()
    if type(load_mesh_from_file) ~= "function" then
        cube_mesh_info.error = "load_mesh_from_file() is unavailable"
        return
    end

    local mesh, err = load_mesh_from_file(cube_mesh_info.path)
    if not mesh then
        cube_mesh_info.error = err or "load_mesh_from_file() failed"
        return
    end

    if type(mesh.vertices) ~= "table" or type(mesh.indices) ~= "table" then
        cube_mesh_info.error = "loader returned unexpected structure"
        return
    end

    cube_vertices = mesh.vertices
    cube_indices = mesh.indices
    cube_mesh_info.loaded = true
    cube_mesh_info.vertex_count = #mesh.vertices
    cube_mesh_info.index_count = #mesh.indices
    cube_mesh_info.error = nil
end

load_cube_mesh()

if not cube_mesh_info.loaded then
    error("Unable to load cube mesh: " .. (cube_mesh_info.error or "unknown"))
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

if cube_mesh_info.loaded then
    log_debug("Loaded cube mesh from %s (%d vertices, %d indices)",
        cube_mesh_info.path, cube_mesh_info.vertex_count, cube_mesh_info.index_count)
end

local cube_body_name = "cube_body"
local cube_state = {
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0, 1.0},
}
local physics_last_time = 0.0

local function quaternion_to_matrix(q)
    local x, y, z, w = q[1], q[2], q[3], q[4]
    local xx = x * x
    local yy = y * y
    local zz = z * z
    local xy = x * y
    local xz = x * z
    local yz = y * z
    local wx = w * x
    local wy = w * y
    local wz = w * z
    return {
        1.0 - 2.0 * yy - 2.0 * zz, 2.0 * xy + 2.0 * wz,     2.0 * xz - 2.0 * wy,     0.0,
        2.0 * xy - 2.0 * wz,     1.0 - 2.0 * xx - 2.0 * zz, 2.0 * yz + 2.0 * wx,     0.0,
        2.0 * xz + 2.0 * wy,     2.0 * yz - 2.0 * wx,     1.0 - 2.0 * xx - 2.0 * yy, 0.0,
        0.0,                     0.0,                     0.0,                     1.0,
    }
end

local function initialize_physics()
    if type(physics_create_box) ~= "function" then
        error("physics_create_box() is unavailable")
    end
    local ok, err = physics_create_box(
        cube_body_name,
        {1.0, 1.0, 1.0},
        1.0,
        {0.0, 2.0, 0.0},
        {0.0, 0.0, 0.0, 1.0}
    )
    if not ok then
        error("physics_create_box failed: " .. (err or "unknown"))
    end
    if type(physics_step_simulation) == "function" then
        physics_step_simulation(0.0)
    end
end
initialize_physics()

local function sync_physics(time)
    local dt = time - physics_last_time
    if dt < 0.0 then
        dt = 0.0
    end
    if dt > 0.0 and type(physics_step_simulation) == "function" then
        physics_step_simulation(dt)
    end
    physics_last_time = time
    if type(physics_get_transform) ~= "function" then
        error("physics_get_transform() is unavailable")
    end
    local transform, err = physics_get_transform(cube_body_name)
    if not transform then
        error("physics_get_transform failed: " .. (err or "unknown"))
    end
    cube_state.position = transform.position
    cube_state.rotation = transform.rotation
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

local function create_rotating_cube(position, speed_scale, shader_key)
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

local function create_physics_cube(shader_key)
    local function compute_model_matrix(time)
        sync_physics(time)
        local offset = math3d.translation(
            cube_state.position[1],
            cube_state.position[2],
            cube_state.position[3]
        )
        local rotation_matrix = quaternion_to_matrix(cube_state.rotation)
        return math3d.multiply(offset, rotation_matrix)
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
        create_physics_cube("cube"),
        create_rotating_cube({3.0, 0.0, 0.0}, 0.8, "cube"),
        create_rotating_cube({-3.0, 0.0, 0.0}, 1.2, "cube"),
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
