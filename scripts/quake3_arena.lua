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

local function resolve_boolean(value, fallback)
    if type(value) == "boolean" then
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
local map_shader_key = nil
if type(quake3_config.shader_key) == "string" and quake3_config.shader_key ~= "" then
    map_shader_key = quake3_config.shader_key
elseif type(config) == "table"
    and type(config.materialx_materials) == "table"
    and type(config.materialx_materials[1]) == "table"
    and type(config.materialx_materials[1].shader_key) == "string"
    and config.materialx_materials[1].shader_key ~= "" then
    map_shader_key = config.materialx_materials[1].shader_key
    log_debug("Using MaterialX shader_key for Quake3 map=%s", map_shader_key)
else
    error("Quake3 config requires a shader_key or materialx_materials[1].shader_key")
end

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

local function transform_point(matrix, point)
    local x = point[1]
    local y = point[2]
    local z = point[3]
    return {
        matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13],
        matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14],
        matrix[3] * x + matrix[7] * y + matrix[11] * z + matrix[15],
    }
end

local function transform_direction(matrix, direction)
    local x = direction[1]
    local y = direction[2]
    local z = direction[3]
    return {
        matrix[1] * x + matrix[5] * y + matrix[9] * z,
        matrix[2] * x + matrix[6] * y + matrix[10] * z,
        matrix[3] * x + matrix[7] * y + matrix[11] * z,
    }
end

local function compute_bounds(vertices, matrix)
    if type(vertices) ~= "table" then
        return nil
    end
    local min_bounds = {math.huge, math.huge, math.huge}
    local max_bounds = {-math.huge, -math.huge, -math.huge}
    for i = 1, #vertices do
        local vertex = vertices[i]
        local position = vertex and (vertex.position or vertex)
        if type(position) == "table" then
            local world_pos = transform_point(matrix, position)
            if world_pos[1] < min_bounds[1] then
                min_bounds[1] = world_pos[1]
            end
            if world_pos[2] < min_bounds[2] then
                min_bounds[2] = world_pos[2]
            end
            if world_pos[3] < min_bounds[3] then
                min_bounds[3] = world_pos[3]
            end
            if world_pos[1] > max_bounds[1] then
                max_bounds[1] = world_pos[1]
            end
            if world_pos[2] > max_bounds[2] then
                max_bounds[2] = world_pos[2]
            end
            if world_pos[3] > max_bounds[3] then
                max_bounds[3] = world_pos[3]
            end
        end
    end
    if min_bounds[1] == math.huge then
        return nil
    end
    return {min = min_bounds, max = max_bounds}
end

local function normalize(vec)
    local x, y, z = vec[1], vec[2], vec[3]
    local len = math.sqrt(x * x + y * y + z * z)
    if len == 0.0 then
        return {x, y, z}
    end
    return {x / len, y / len, z / len}
end

local function scale_vec3(vec, scale)
    return {vec[1] * scale, vec[2] * scale, vec[3] * scale}
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

local action_states = {}
local function is_action_pressed(action_name, fallback_key)
    local is_down = is_action_down(action_name, fallback_key)
    local was_down = action_states[action_name]
    action_states[action_name] = is_down
    return is_down and not was_down
end

local fallback_bindings = {
    move_forward = "W",
    move_back = "S",
    move_left = "A",
    move_right = "D",
    fly_up = "Q",
    fly_down = "Z",
    jump = "Space",
    noclip_toggle = "N",
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
local map_up = normalize(transform_direction(map_model_matrix, {0.0, 0.0, 1.0}))

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

local map_bounds = compute_bounds(map_mesh.vertices, map_model_matrix)
if map_bounds then
    log_debug("Map bounds min=(%.2f, %.2f, %.2f) max=(%.2f, %.2f, %.2f)",
        map_bounds.min[1], map_bounds.min[2], map_bounds.min[3],
        map_bounds.max[1], map_bounds.max[2], map_bounds.max[3])
end

local camera_config = resolve_table(quake3_config.camera)

local function compute_spawn_position(bounds)
    if not bounds then
        return {0.0, 48.0, 0.0}
    end
    local min_bounds = bounds.min
    local max_bounds = bounds.max
    local center = {
        (min_bounds[1] + max_bounds[1]) * 0.5,
        (min_bounds[2] + max_bounds[2]) * 0.5,
        (min_bounds[3] + max_bounds[3]) * 0.5,
    }
    local height = math.max(2.0, (max_bounds[2] - min_bounds[2]) * 0.1)
    local spawn_height = resolve_number(quake3_config.spawn_height, height)
    local spawn_offset = resolve_vec3(quake3_config.spawn_offset, {0.0, 0.0, 0.0})
    return {
        center[1] + spawn_offset[1],
        max_bounds[2] + spawn_height + spawn_offset[2],
        center[3] + spawn_offset[3],
    }
end

local function is_position_far_from_bounds(position, bounds, margin_scale)
    if not bounds then
        return false
    end
    local min_bounds = bounds.min
    local max_bounds = bounds.max
    local extent_x = max_bounds[1] - min_bounds[1]
    local extent_y = max_bounds[2] - min_bounds[2]
    local extent_z = max_bounds[3] - min_bounds[3]
    local scale = resolve_number(margin_scale, 0.5)
    local margin = math.max(extent_x, extent_y, extent_z) * scale
    if position[1] < min_bounds[1] - margin or position[1] > max_bounds[1] + margin then
        return true
    end
    if position[2] < min_bounds[2] - margin or position[2] > max_bounds[2] + margin then
        return true
    end
    if position[3] < min_bounds[3] - margin or position[3] > max_bounds[3] + margin then
        return true
    end
    return false
end

local camera_position = resolve_vec3(camera_config.position, {0.0, 48.0, 0.0})
local default_spawn_position = compute_spawn_position(map_bounds)
local spawn_position = resolve_vec3(quake3_config.spawn_position, camera_position)
local auto_spawn = quake3_config.auto_spawn
if auto_spawn == nil then
    auto_spawn = true
end
if auto_spawn and is_position_far_from_bounds(camera_position, map_bounds, quake3_config.bounds_margin_scale) then
    camera_position = {default_spawn_position[1], default_spawn_position[2], default_spawn_position[3]}
    spawn_position = resolve_vec3(quake3_config.spawn_position, camera_position)
    log_debug("Camera spawn adjusted to map bounds (%.2f, %.2f, %.2f)",
        camera_position[1], camera_position[2], camera_position[3])
end

local respawn_config = resolve_table(quake3_config.respawn)
local respawn_enabled = resolve_boolean(respawn_config.enabled, true)
local respawn_margin_scale = resolve_number(
    respawn_config.margin_scale,
    resolve_number(quake3_config.bounds_margin_scale, 0.5))

local camera = {
    position = camera_position,
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

local physics_config = resolve_table(quake3_config.physics)
local physics_enabled = resolve_boolean(physics_config.enabled, true)
local physics_available = type(physics_create_static_mesh) == "function"
    and type(physics_create_sphere) == "function"
    and type(physics_get_transform) == "function"
    and type(physics_set_linear_velocity) == "function"
    and type(physics_get_linear_velocity) == "function"
    and type(physics_step_simulation) == "function"

if physics_enabled and not physics_available then
    log_debug("Physics disabled: required bindings are unavailable")
end

local align_gravity_to_map = resolve_boolean(physics_config.align_gravity_to_map, true)
local default_gravity = {0.0, -9.8, 0.0}
if align_gravity_to_map then
    default_gravity = scale_vec3(map_up, -9.8)
    log_debug("Gravity aligned to map up=(%.2f, %.2f, %.2f) gravity=(%.2f, %.2f, %.2f)",
        map_up[1], map_up[2], map_up[3],
        default_gravity[1], default_gravity[2], default_gravity[3])
end

local physics_state = {
    enabled = physics_enabled and physics_available,
    ready = false,
    noclip = false,
    map_body_name = physics_config.map_body_name or "quake3_map",
    player_body_name = physics_config.player_body_name or "quake3_player",
    player_radius = resolve_number(physics_config.player_radius, resolve_number(quake3_config.player_radius, 0.6)),
    player_mass = resolve_number(physics_config.player_mass, resolve_number(quake3_config.player_mass, 1.0)),
    eye_height = resolve_number(physics_config.eye_height, resolve_number(quake3_config.eye_height, 0.6)),
    gravity = resolve_vec3(physics_config.gravity, default_gravity),
    jump_impulse = resolve_number(physics_config.jump_impulse, resolve_number(quake3_config.jump_impulse, 4.5)),
    jump_velocity_threshold = resolve_number(
        physics_config.jump_velocity_threshold,
        resolve_number(quake3_config.jump_velocity_threshold, 0.2)),
    max_sub_steps = resolve_number(physics_config.max_sub_steps, resolve_number(quake3_config.max_sub_steps, 8)),
}

physics_state.spawn_position = {
    spawn_position[1],
    spawn_position[2] - physics_state.eye_height,
    spawn_position[3],
}

local last_frame_time = nil
local world_up = {0.0, 1.0, 0.0}

local function get_time_seconds()
    if type(time_get_seconds) == "function" then
        local ok, value = pcall(time_get_seconds)
        if ok and type(value) == "number" then
            return value
        end
    end
    return os.clock()
end

local function update_camera_angles()
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
end

local function resolve_move_input()
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

    return move_x, move_y, move_z
end

local function update_free_fly(dt, forward_flat, right)
    local move_x, move_y, move_z = resolve_move_input()
    local planar_speed = controls.move_speed * dt
    camera.position[1] = camera.position[1] + (right[1] * move_x + forward_flat[1] * move_z) * planar_speed
    camera.position[2] = camera.position[2] + (right[2] * move_x + forward_flat[2] * move_z) * planar_speed
    camera.position[3] = camera.position[3] + (right[3] * move_x + forward_flat[3] * move_z) * planar_speed

    if move_y ~= 0.0 then
        camera.position[2] = camera.position[2] + move_y * controls.fly_speed * dt
    end
end

local function ensure_physics_setup()
    if not physics_state.enabled then
        return false
    end
    if physics_state.ready then
        return true
    end

    if type(physics_clear) == "function" then
        physics_clear()
    end
    if type(physics_set_gravity) == "function" then
        local ok, err = physics_set_gravity(physics_state.gravity)
        if not ok then
            log_debug("Physics gravity failed: %s", err or "unknown")
        end
    end

    local ok, err = physics_create_static_mesh(
        physics_state.map_body_name,
        map_mesh.vertices,
        map_mesh.indices,
        map_model_matrix)
    if not ok then
        log_debug("Physics map creation failed: %s", err or "unknown")
        physics_state.enabled = false
        return false
    end

    local rotation = {0.0, 0.0, 0.0, 1.0}
    ok, err = physics_create_sphere(
        physics_state.player_body_name,
        physics_state.player_radius,
        physics_state.player_mass,
        physics_state.spawn_position,
        rotation)
    if not ok then
        log_debug("Physics player creation failed: %s", err or "unknown")
        physics_state.enabled = false
        return false
    end

    if type(physics_set_linear_velocity) == "function" then
        physics_set_linear_velocity(physics_state.player_body_name, {0.0, 0.0, 0.0})
    end

    physics_state.ready = true
    log_debug("Physics world initialized for Quake 3 map")
    return true
end

local function apply_physics_controls(forward_flat, right)
    local move_x, _, move_z = resolve_move_input()
    local desired_x = (right[1] * move_x + forward_flat[1] * move_z) * controls.move_speed
    local desired_z = (right[3] * move_x + forward_flat[3] * move_z) * controls.move_speed

    local current_velocity = {0.0, 0.0, 0.0}
    local velocity, velocity_err = physics_get_linear_velocity(physics_state.player_body_name)
    if type(velocity) == "table" then
        current_velocity = velocity
    elseif velocity_err then
        log_debug("Physics velocity query failed: %s", velocity_err)
    end

    local desired_velocity = {desired_x, current_velocity[2] or 0.0, desired_z}
    local ok, err = physics_set_linear_velocity(physics_state.player_body_name, desired_velocity)
    if not ok then
        log_debug("Physics velocity failed: %s", err or "unknown")
    end

    if type(physics_apply_impulse) == "function"
        and is_action_pressed("jump", get_binding("jump"))
        and math.abs(current_velocity[2] or 0.0) < physics_state.jump_velocity_threshold then
        local impulse = {0.0, physics_state.jump_impulse, 0.0}
        local jump_ok, jump_err = physics_apply_impulse(physics_state.player_body_name, impulse)
        if not jump_ok then
            log_debug("Physics jump failed: %s", jump_err or "unknown")
        end
    end
end

local function sync_camera_from_physics()
    local transform, transform_err = physics_get_transform(physics_state.player_body_name)
    if type(transform) == "table" and type(transform.position) == "table" then
        camera.position[1] = transform.position[1]
        camera.position[2] = transform.position[2] + physics_state.eye_height
        camera.position[3] = transform.position[3]
    elseif transform_err then
        log_debug("Physics transform query failed: %s", transform_err)
    end
end

local function reset_player_to_spawn(reason)
    camera.position[1] = spawn_position[1]
    camera.position[2] = spawn_position[2]
    camera.position[3] = spawn_position[3]

    if physics_state.enabled and physics_state.ready and type(physics_set_transform) == "function" then
        local rotation = {0.0, 0.0, 0.0, 1.0}
        local reset_position = {
            spawn_position[1],
            spawn_position[2] - physics_state.eye_height,
            spawn_position[3],
        }
        local ok, err = physics_set_transform(
            physics_state.player_body_name,
            reset_position,
            rotation)
        if not ok then
            log_debug("Physics respawn failed: %s", err or "unknown")
        elseif type(physics_set_linear_velocity) == "function" then
            physics_set_linear_velocity(physics_state.player_body_name, {0.0, 0.0, 0.0})
        end
    end

    log_debug("Respawned player (%s) at (%.2f, %.2f, %.2f)",
        tostring(reason or "unknown"),
        spawn_position[1],
        spawn_position[2],
        spawn_position[3])
end

local function check_respawn(position)
    if not respawn_enabled then
        return
    end
    if is_position_far_from_bounds(position, map_bounds, respawn_margin_scale) then
        reset_player_to_spawn("out_of_bounds")
    end
end

function get_scene_objects()
    return {
        {
            vertices = map_mesh.vertices,
            indices = map_mesh.indices,
            shader_keys = {map_shader_key},
            compute_model_matrix = function()
                return map_model_matrix
            end,
        },
    }
end

local function build_view_state(aspect)
    local now = get_time_seconds()
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

    update_camera_angles()

    local forward = forward_from_angles(camera.yaw, camera.pitch)
    local forward_flat = normalize({forward[1], 0.0, forward[3]})
    local right = normalize(cross(forward_flat, world_up))

    local physics_ready = physics_state.enabled and ensure_physics_setup()
    if physics_ready then
        if is_action_pressed("noclip_toggle", get_binding("noclip_toggle")) then
            physics_state.noclip = not physics_state.noclip
            log_debug("Noclip toggled: %s", tostring(physics_state.noclip))
            if not physics_state.noclip and type(physics_set_transform) == "function" then
                local rotation = {0.0, 0.0, 0.0, 1.0}
                local reset_position = {
                    camera.position[1],
                    camera.position[2] - physics_state.eye_height,
                    camera.position[3],
                }
                local ok, err = physics_set_transform(
                    physics_state.player_body_name,
                    reset_position,
                    rotation)
                if not ok then
                    log_debug("Physics reset failed: %s", err or "unknown")
                elseif type(physics_set_linear_velocity) == "function" then
                    physics_set_linear_velocity(physics_state.player_body_name, {0.0, 0.0, 0.0})
                end
            end
        end
        if physics_state.noclip then
            update_free_fly(dt, forward_flat, right)
            check_respawn(camera.position)
        else
            apply_physics_controls(forward_flat, right)
            if dt > 0.0 then
                physics_step_simulation(dt, physics_state.max_sub_steps)
            end
            sync_camera_from_physics()
            check_respawn(camera.position)
        end
    else
        update_free_fly(dt, forward_flat, right)
        check_respawn(camera.position)
    end
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
