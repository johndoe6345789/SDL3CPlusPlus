local math3d = require("math3d")

local function log_debug(fmt, ...)
    if not lua_debug or not fmt then
        return
    end
    print(string.format(fmt, ...))
end

local function resolve_table(value)
    if type(value) == "table" then
        return value
    end
    return {}
end

local function resolve_number(value, fallback)
    if type(value) == "number" then
        return value
    end
    return fallback
end

local function resolve_vec3(value, fallback)
    if type(value) == "table"
        and type(value[1]) == "number"
        and type(value[2]) == "number"
        and type(value[3]) == "number" then
        return {value[1], value[2], value[3]}
    end
    return {fallback[1], fallback[2], fallback[3]}
end

local quake3_config = resolve_table(type(config) == "table" and config.quake3)

local pk3_path = quake3_config.pk3_path or "/home/rewrich/Documents/GitHub/q3/pak0.pk3"
local map_entry = quake3_config.map_path or "q3dm1"
if not string.find(map_entry, "%.bsp$") then
    map_entry = map_entry .. ".bsp"
end
if not string.find(map_entry, "/") then
    map_entry = "maps/" .. map_entry
end

local rotation_config = resolve_table(quake3_config.rotation_degrees)
local map_rotate_x = resolve_number(rotation_config.x, resolve_number(quake3_config.rotate_x_degrees, -90.0))
local map_rotate_y = resolve_number(rotation_config.y, resolve_number(quake3_config.rotate_y_degrees, 0.0))
local map_rotate_z = resolve_number(rotation_config.z, resolve_number(quake3_config.rotate_z_degrees, 0.0))

local map_scale = resolve_number(quake3_config.scale, 0.01)
local map_offset = resolve_vec3(quake3_config.offset, {0.0, 0.0, 0.0})
local map_shader_key = quake3_config.shader_key or "pbr"

local function scale_matrix(x, y, z)
    return {
        x, 0.0, 0.0, 0.0,
        0.0, y, 0.0, 0.0,
        0.0, 0.0, z, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

local function rotation_z(radians)
    local c = math.cos(radians)
    local s = math.sin(radians)
    return {
        c,  s, 0.0, 0.0,
        -s, c, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

local function clamp(value, min_value, max_value)
    if value < min_value then
        return min_value
    end
    if value > max_value then
        return max_value
    end
    return value
end

local function normalize(vec)
    local x, y, z = vec[1], vec[2], vec[3]
    local len = math.sqrt(x * x + y * y + z * z)
    if len == 0.0 then
        return {x, y, z}
    end
    return {x / len, y / len, z / len}
end

local function cross(a, b)
    return {
        a[2] * b[3] - a[3] * b[2],
        a[3] * b[1] - a[1] * b[3],
        a[1] * b[2] - a[2] * b[1],
    }
end

local function forward_from_angles(yaw, pitch)
    local cos_pitch = math.cos(pitch)
    return {
        math.cos(yaw) * cos_pitch,
        math.sin(pitch),
        math.sin(yaw) * cos_pitch,
    }
end

local function is_action_down(action_name, fallback_key)
    if type(input_is_action_down) == "function" then
        return input_is_action_down(action_name)
    end
    if type(input_is_key_down) == "function" and fallback_key then
        return input_is_key_down(fallback_key)
    end
    return false
end

local fallback_bindings = {
    move_forward = "W",
    move_back = "S",
    move_left = "A",
    move_right = "D",
    fly_up = "Q",
    fly_down = "Z",
}

local input_bindings = resolve_table(type(config) == "table" and config.input_bindings)
local function get_binding(action_name)
    if type(input_bindings[action_name]) == "string" then
        return input_bindings[action_name]
    end
    return fallback_bindings[action_name]
end

local function build_map_model_matrix()
    local translation = math3d.translation(map_offset[1], map_offset[2], map_offset[3])
    local rotation_x = math3d.rotation_x(math.rad(map_rotate_x))
    local rotation_y = math3d.rotation_y(math.rad(map_rotate_y))
    local rotation = math3d.multiply(rotation_y, rotation_x)
    if map_rotate_z ~= 0.0 then
        local rotation_z_matrix = rotation_z(math.rad(map_rotate_z))
        rotation = math3d.multiply(rotation_z_matrix, rotation)
    end
    local scaling = scale_matrix(map_scale, map_scale, map_scale)
    return math3d.multiply(translation, math3d.multiply(rotation, scaling))
end

local map_model_matrix = build_map_model_matrix()

if type(load_mesh_from_pk3) ~= "function" then
    error("load_mesh_from_pk3() is unavailable; rebuild with libzip support")
end

local map_mesh, map_error = load_mesh_from_pk3(pk3_path, map_entry)
if not map_mesh then
    error("Unable to load Quake 3 map: " .. tostring(map_error))
end

log_debug("Loaded Quake 3 map %s from %s (%d vertices, %d indices)",
    map_entry,
    pk3_path,
    #map_mesh.vertices,
    #map_mesh.indices)

local camera_config = resolve_table(quake3_config.camera)
local camera = {
    position = resolve_vec3(camera_config.position, {0.0, 48.0, 0.0}),
    yaw = math.rad(resolve_number(camera_config.yaw_degrees, 0.0)),
    pitch = math.rad(resolve_number(camera_config.pitch_degrees, 0.0)),
    fov = resolve_number(camera_config.fov, 0.85),
    near = resolve_number(camera_config.near, 0.1),
    far = resolve_number(camera_config.far, 2000.0),
}

local controls = {
    move_speed = resolve_number(quake3_config.move_speed, 14.0),
    fly_speed = resolve_number(quake3_config.fly_speed, 10.0),
    mouse_sensitivity = resolve_number(quake3_config.mouse_sensitivity, 0.0025),
    max_pitch = math.rad(85.0),
}

local last_frame_time = nil
local world_up = {0.0, 1.0, 0.0}

local function update_camera(dt)
    local look_delta_x = 0.0
    local look_delta_y = 0.0
    if type(input_get_mouse_delta) == "function" then
        local dx, dy = input_get_mouse_delta()
        if type(dx) == "number" then
            look_delta_x = dx * controls.mouse_sensitivity
        end
        if type(dy) == "number" then
            look_delta_y = -dy * controls.mouse_sensitivity
        end
    end

    camera.yaw = camera.yaw + look_delta_x
    camera.pitch = clamp(camera.pitch + look_delta_y, -controls.max_pitch, controls.max_pitch)

    local forward = forward_from_angles(camera.yaw, camera.pitch)
    local forward_flat = normalize({forward[1], 0.0, forward[3]})
    local right = normalize(cross(forward_flat, world_up))

    local move_x = 0.0
    local move_z = 0.0
    local move_y = 0.0

    if is_action_down("move_forward", get_binding("move_forward")) then
        move_z = move_z + 1.0
    end
    if is_action_down("move_back", get_binding("move_back")) then
        move_z = move_z - 1.0
    end
    if is_action_down("move_right", get_binding("move_right")) then
        move_x = move_x + 1.0
    end
    if is_action_down("move_left", get_binding("move_left")) then
        move_x = move_x - 1.0
    end
    if is_action_down("fly_up", get_binding("fly_up")) then
        move_y = move_y + 1.0
    end
    if is_action_down("fly_down", get_binding("fly_down")) then
        move_y = move_y - 1.0
    end

    local length = math.sqrt(move_x * move_x + move_z * move_z)
    if length > 1.0 then
        move_x = move_x / length
        move_z = move_z / length
    end

    local planar_speed = controls.move_speed * dt
    camera.position[1] = camera.position[1] + (right[1] * move_x + forward_flat[1] * move_z) * planar_speed
    camera.position[2] = camera.position[2] + (right[2] * move_x + forward_flat[2] * move_z) * planar_speed
    camera.position[3] = camera.position[3] + (right[3] * move_x + forward_flat[3] * move_z) * planar_speed

    if move_y ~= 0.0 then
        camera.position[2] = camera.position[2] + move_y * controls.fly_speed * dt
    end
end

local shader_variants_module = require("shader_variants")
local shader_variants = shader_variants_module.build_cube_variants(config, log_debug)

function get_scene_objects()
    return {
        {
            vertices = map_mesh.vertices,
            indices = map_mesh.indices,
            shader_key = map_shader_key,
            compute_model_matrix = function()
                return map_model_matrix
            end,
        },
    }
end

function get_shader_paths()
    return shader_variants
end

local function build_view_state(aspect)
    local now = os.clock()
    local dt = 0.0
    if last_frame_time then
        dt = now - last_frame_time
    end
    last_frame_time = now
    if dt < 0.0 then
        dt = 0.0
    elseif dt > 0.1 then
        dt = 0.1
    end

    update_camera(dt)

    local forward = forward_from_angles(camera.yaw, camera.pitch)
    local center = {
        camera.position[1] + forward[1],
        camera.position[2] + forward[2],
        camera.position[3] + forward[3],
    }

    local view = math3d.look_at(camera.position, center, world_up)
    local projection = math3d.perspective(camera.fov, aspect, camera.near, camera.far)

    return {
        view = view,
        proj = projection,
        view_proj = math3d.multiply(projection, view),
        camera_pos = {camera.position[1], camera.position[2], camera.position[3]},
    }
end

function get_view_state(aspect)
    return build_view_state(aspect)
end

function get_view_projection(aspect)
    local state = build_view_state(aspect)
    return state.view_proj
end
