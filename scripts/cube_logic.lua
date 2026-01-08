local scene_framework = require("scene_framework")
local math3d = require("math3d")
local config_resolver = require("config_resolver")

local resolve_number = scene_framework.resolve_number
local resolve_boolean = scene_framework.resolve_boolean
local resolve_string = scene_framework.resolve_string
local resolve_table = scene_framework.resolve_table
local resolve_vec3 = scene_framework.resolve_vec3

local scene_config = resolve_table(config_resolver.resolve_scene(config))
local cube_mesh_config = resolve_table(scene_config.cube_mesh)
local cube_mesh_path = resolve_string(cube_mesh_config.path, "models/cube.stl")
local cube_mesh_double_sided = resolve_boolean(cube_mesh_config.double_sided, true)

local cube_mesh_info = {
    path = cube_mesh_path,
    loaded = false,
    vertex_count = 0,
    index_count = 0,
    error = "load_mesh_from_file() not registered",
}

local cube_vertices = {}
local cube_indices = {}
local cube_indices_double_sided = {}

local function build_double_sided_indices(indices)
    local doubled = {}
    for i = 1, #indices, 3 do
        local a = indices[i]
        local b = indices[i + 1]
        local c = indices[i + 2]
        if not (a and b and c) then
            break
        end
        doubled[#doubled + 1] = a
        doubled[#doubled + 1] = b
        doubled[#doubled + 1] = c
        doubled[#doubled + 1] = c
        doubled[#doubled + 1] = b
        doubled[#doubled + 1] = a
    end
    return doubled
end

-- Delegate to framework for plane mesh generation
local function generate_plane_mesh(width, depth, subdivisions, color)
    -- Framework returns 0-based indices, convert to 1-based for Lua
    local vertices, indices_zero = scene_framework.generate_plane_mesh(width, depth, subdivisions, color)
    local indices = {}
    for i = 1, #indices_zero do
        indices[i] = indices_zero[i] + 1
    end
    return vertices, indices
end

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
    if cube_mesh_double_sided then
        cube_indices_double_sided = build_double_sided_indices(cube_indices)
    else
        cube_indices_double_sided = {}
    end
    cube_mesh_info.loaded = true
    cube_mesh_info.vertex_count = #mesh.vertices
    cube_mesh_info.index_count = #mesh.indices
    cube_mesh_info.error = nil
end

load_cube_mesh()

if not cube_mesh_info.loaded then
    error("Unable to load cube mesh: " .. (cube_mesh_info.error or "unknown"))
end

local music_state = {
    playing = false,
    togglePressed = false,
}

local function start_music()
    if type(audio_play_background) == "function" then
        audio_play_background("piano.ogg", true)
        music_state.playing = true
    end
end

local function stop_music()
    if type(audio_stop_background) == "function" then
        audio_stop_background()
    end
    music_state.playing = false
end

local function toggle_music()
    if music_state.playing then
        stop_music()
    else
        start_music()
    end
end

start_music()

local Gui = require("gui")
local string_format = string.format

local function resolve_positive_int(value, fallback)
    local candidate = resolve_number(value, fallback)
    local rounded = math.floor(candidate)
    if rounded < 1 then
        return fallback
    end
    return rounded
end

local function resolve_window_size()
    local default_width = 1024
    local default_height = 768
    if type(config) ~= "table" then
        return default_width, default_height
    end

    local window = resolve_table(config.window)
    local size = resolve_table(window.size)
    local width = resolve_positive_int(size.width, default_width)
    local height = resolve_positive_int(size.height, default_height)

    width = resolve_positive_int(config.window_width, width)
    height = resolve_positive_int(config.window_height, height)
    return width, height
end

local InputState = {}
InputState.__index = InputState

function InputState:new()
    local sharedKeys = {}
    local instance = {
        mouseX = 0.0,
        mouseY = 0.0,
        mouseDeltaX = 0.0,
        mouseDeltaY = 0.0,
        mouseDown = false,
        mouseDownPrevious = false,
        wheel = 0.0,
        textInput = "",
        keyStates = sharedKeys,
        keys = sharedKeys,
        lastMouseX = nil,
        lastMouseY = nil,
        gamepad = {
            connected = false,
            leftX = 0.0,
            leftY = 0.0,
            rightX = 0.0,
            rightY = 0.0,
            togglePressed = false,
        },
    }
    return setmetatable(instance, InputState)
end

function InputState:resetTransient()
    self.textInput = ""
    self.wheel = 0.0
    self.mouseDeltaX = 0.0
    self.mouseDeltaY = 0.0
end

function InputState:setMouse(x, y, isDown, deltaX, deltaY)
    self.mouseDownPrevious = self.mouseDown
    if type(deltaX) == "number" and type(deltaY) == "number" then
        self.mouseDeltaX = deltaX
        self.mouseDeltaY = deltaY
    elseif self.lastMouseX ~= nil and self.lastMouseY ~= nil then
        self.mouseDeltaX = x - self.lastMouseX
        self.mouseDeltaY = y - self.lastMouseY
    end
    self.lastMouseX = x
    self.lastMouseY = y
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

function InputState:mouseJustPressed()
    return self.mouseDown and not self.mouseDownPrevious
end

function InputState:mouseJustReleased()
    return not self.mouseDown and self.mouseDownPrevious
end

function InputState:isKeyDown(keyName)
    return self.keyStates[keyName]
end

function InputState:addTextInput(text)
    if text then
        self.textInput = self.textInput .. text
    end
end

function InputState:setGamepad(connected, leftX, leftY, rightX, rightY, togglePressed)
    local pad = self.gamepad
    pad.connected = connected and true or false
    pad.leftX = leftX or 0.0
    pad.leftY = leftY or 0.0
    pad.rightX = rightX or 0.0
    pad.rightY = rightY or 0.0
    pad.togglePressed = togglePressed and true or false
end

gui_input = InputState:new()

local gui_context = Gui.newContext()
local window_width, window_height = resolve_window_size()
local ui_layout = {
    width = window_width,
    height = window_height,
    margin = 16,
}
local compass_layout = {
    size = 120,
    label_height = 18,
    padding = 10,
}
local flight_layout = {
    width = 120,
    height = 28,
    spacing = 8,
}
local physics_layout = {
    width = 140,
    height = 28,
    spacing = 8,
}
local ui_state = {
    flyUpActive = false,
    flyDownActive = false,
    flyUpPulse = false,
    flyDownPulse = false,
}

local function log_debug(fmt, ...)
    if not lua_debug or not fmt then
        return
    end
    print(string_format(fmt, ...))
end

if cube_mesh_info.loaded then
    log_debug("Loaded cube mesh from %s (%d vertices, %d indices)",
        cube_mesh_info.path, cube_mesh_info.vertex_count, cube_mesh_info.index_count)
    if #cube_indices_double_sided > 0 then
        log_debug("Built double-sided cube indices (%d -> %d)",
            cube_mesh_info.index_count, #cube_indices_double_sided)
    end
end

local camera_config = resolve_table(scene_config.camera)
local controls_config = resolve_table(scene_config.controls)
local max_pitch_degrees = resolve_number(controls_config.max_pitch_degrees, nil)
local max_pitch = resolve_number(controls_config.max_pitch, nil)
if max_pitch_degrees then
    max_pitch = math.rad(max_pitch_degrees)
end
if not max_pitch then
    max_pitch = math.rad(85.0)
end

local camera = {
    position = {0.0, 0.0, 5.0},
    yaw = resolve_number(camera_config.yaw, math.pi),
    pitch = resolve_number(camera_config.pitch, 0.0),
    fov = resolve_number(camera_config.fov, 0.78),
    near = resolve_number(camera_config.near, 0.1),
    far = resolve_number(camera_config.far, 50.0),
}

local controls = {
    move_speed = resolve_number(controls_config.move_speed, 16.0),
    fly_speed = resolve_number(controls_config.fly_speed, 3.0),
    jump_speed = resolve_number(controls_config.jump_speed, 5.5),
    gravity = resolve_number(controls_config.gravity, -12.0),
    max_fall_speed = resolve_number(controls_config.max_fall_speed, -20.0),
    mouse_sensitivity = resolve_number(controls_config.mouse_sensitivity, 0.0025),
    gamepad_look_speed = resolve_number(controls_config.gamepad_look_speed, 2.5),
    stick_deadzone = resolve_number(controls_config.stick_deadzone, 0.2),
    max_pitch = max_pitch,
    move_forward_uses_pitch = resolve_boolean(controls_config.move_forward_uses_pitch, true),
}

local last_frame_time = nil

local function get_time_seconds()
    if type(time_get_seconds) == "function" then
        local ok, value = pcall(time_get_seconds)
        if ok and type(value) == "number" then
            return value
        end
    end
    return os.clock()
end
local movement_log_cooldown = 0.0
local world_up = {0.0, 1.0, 0.0}
local room_config = resolve_table(scene_config.room)
local lantern_config = resolve_table(scene_config.lanterns)
local physics_config = resolve_table(scene_config.physics_cube)
local spinning_config = resolve_table(scene_config.spinning_cube)
local rotation_speed = resolve_number(scene_config.rotation_speed, 0.9)
local room = {
    half_size = resolve_number(room_config.half_size, 15.0),
    wall_thickness = resolve_number(room_config.wall_thickness, 0.5),
    wall_height = resolve_number(room_config.wall_height, 4.0),
    floor_half_thickness = resolve_number(room_config.floor_half_thickness, 0.3),
    floor_top = resolve_number(room_config.floor_top, 0.0),
    floor_subdivisions = resolve_positive_int(room_config.floor_subdivisions, 20),
    floor_color = resolve_vec3(room_config.floor_color, {1.0, 1.0, 1.0}),
    wall_color = resolve_vec3(room_config.wall_color, {1.0, 1.0, 1.0}),
    ceiling_color = resolve_vec3(room_config.ceiling_color, {1.0, 1.0, 1.0}),
}
local player_state = {
    eye_height = 1.6,
    radius = 0.4,
    vertical_velocity = 0.0,
    grounded = true,
    jump_pressed = false,
    noclip = false,
    noclip_toggle_pressed = false,
}

local function physics_is_available()
    return type(physics_create_box) == "function"
        and type(physics_step_simulation) == "function"
        and type(physics_get_transform) == "function"
        and type(math3d.from_transform) == "function"
end

local physics_enabled = resolve_boolean(physics_config.enabled, true)
local physics_cube_half_extents = resolve_vec3(physics_config.half_extents, {1.5, 1.5, 1.5})
local physics_cube_scale = {physics_cube_half_extents[1], physics_cube_half_extents[2], physics_cube_half_extents[3]}
local physics_cube_spawn = resolve_vec3(physics_config.spawn, {
    0.0,
    room.floor_top + room.wall_height + physics_cube_half_extents[2] + 0.5,
    0.0,
})
local physics_cube_color = resolve_vec3(physics_config.color, {0.92, 0.34, 0.28})
local physics_kick_strength = resolve_number(physics_config.kick_strength, 6.0)
local physics_cube_mass = resolve_number(physics_config.mass, 1.0)
local physics_gravity = resolve_vec3(physics_config.gravity, {0.0, -9.8, 0.0})
local physics_max_sub_steps = resolve_positive_int(physics_config.max_sub_steps, 10)
local physics_spin_speed = resolve_number(physics_config.rotation_speed, rotation_speed)
local spinning_cube_enabled = resolve_boolean(spinning_config.enabled, true)
local spinning_cube_scale = resolve_number(spinning_config.scale, 1.5)
local spinning_cube_height = resolve_number(spinning_config.height, 5.0)
local spinning_cube_color = resolve_vec3(spinning_config.color, {0.75, 0.45, 0.25})
local spinning_cube_rotation_speed = resolve_number(spinning_config.rotation_speed, rotation_speed)
local lantern_enabled = resolve_boolean(lantern_config.enabled, true)
local lantern_height = resolve_number(lantern_config.height, 8.0)
local lantern_size = resolve_number(lantern_config.size, 0.2)
local lantern_color = resolve_vec3(lantern_config.color, {1.0, 0.9, 0.6})
local lantern_corner_offset = resolve_number(lantern_config.corner_offset, 2.0)
local lantern_wall_offset = resolve_number(lantern_config.wall_offset, lantern_corner_offset)

local physics_state = {
    enabled = physics_is_available() and physics_enabled,
    ready = false,
    last_step_time = nil,
    max_sub_steps = physics_max_sub_steps,
    cube_name = "demo_cube",
    cube_half_extents = physics_cube_half_extents,
    cube_scale = physics_cube_scale,
    cube_mass = physics_cube_mass,
    cube_color = physics_cube_color,
    cube_spawn = physics_cube_spawn,
    kick_strength = physics_kick_strength,
    gravity = physics_gravity,
}

camera.position = resolve_vec3(camera_config.position, {
    0.0,
    room.floor_top + player_state.eye_height,
    10.0,
})

local function clamp(value, minValue, maxValue)
    if value < minValue then
        return minValue
    end
    if value > maxValue then
        return maxValue
    end
    return value
end

local function scale_matrix(x, y, z)
    return {
        x, 0.0, 0.0, 0.0,
        0.0, y, 0.0, 0.0,
        0.0, 0.0, z, 0.0,
        0.0, 0.0, 0.0, 1.0,
    }
end

local function ensure_physics_setup()
    if not physics_state.enabled then
        return false
    end
    if physics_state.ready then
        return true
    end

    physics_state.last_step_time = nil
    if type(physics_clear) == "function" then
        physics_clear()
    end
    if type(physics_set_gravity) == "function" then
        local ok, err = physics_set_gravity(physics_state.gravity)
        if not ok then
            log_debug("Physics gravity failed: %s", err or "unknown")
        end
    end

    local rotation = {0.0, 0.0, 0.0, 1.0}
    local function add_static_body(name, half_extents, origin)
        local ok, err = physics_create_box(name, half_extents, 0.0, origin, rotation)
        if not ok then
            log_debug("Physics static body %s failed: %s", name, err or "unknown")
        end
    end

    local floor_center_y = room.floor_top - room.floor_half_thickness
    local wall_center_y = room.floor_top + room.wall_height
    local ceiling_y = room.floor_top + room.wall_height * 2 + room.floor_half_thickness
    local wall_offset = room.half_size + room.wall_thickness

    add_static_body("room_floor",
        {room.half_size, room.floor_half_thickness, room.half_size},
        {0.0, floor_center_y, 0.0})
    add_static_body("room_ceiling",
        {room.half_size, room.floor_half_thickness, room.half_size},
        {0.0, ceiling_y, 0.0})
    add_static_body("room_wall_north",
        {room.half_size, room.wall_height, room.wall_thickness},
        {0.0, wall_center_y, -wall_offset})
    add_static_body("room_wall_south",
        {room.half_size, room.wall_height, room.wall_thickness},
        {0.0, wall_center_y, wall_offset})
    add_static_body("room_wall_west",
        {room.wall_thickness, room.wall_height, room.half_size},
        {-wall_offset, wall_center_y, 0.0})
    add_static_body("room_wall_east",
        {room.wall_thickness, room.wall_height, room.half_size},
        {wall_offset, wall_center_y, 0.0})

    local ok, err = physics_create_box(
        physics_state.cube_name,
        physics_state.cube_half_extents,
        physics_state.cube_mass,
        physics_state.cube_spawn,
        rotation)
    if not ok then
        log_debug("Physics cube create failed: %s", err or "unknown")
        return false
    end

    if type(physics_set_linear_velocity) == "function" then
        physics_set_linear_velocity(physics_state.cube_name, {0.0, 0.0, 0.0})
    end

    physics_state.ready = true
    log_debug("Physics demo initialized")
    return true
end

local function step_physics(time)
    if not physics_state.ready then
        return
    end
    if type(time) ~= "number" then
        return
    end
    if physics_state.last_step_time == time then
        return
    end

    local dt = 0.0
    if physics_state.last_step_time then
        dt = time - physics_state.last_step_time
    end
    physics_state.last_step_time = time

    if dt <= 0.0 then
        return
    end
    if dt > 0.1 then
        dt = 0.1
    end

    physics_step_simulation(dt, physics_state.max_sub_steps)
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

local function apply_deadzone(value, deadzone)
    local magnitude = math.abs(value)
    if magnitude < deadzone then
        return 0.0
    end
    local scaled = (magnitude - deadzone) / (1.0 - deadzone)
    if value < 0.0 then
        return -scaled
    end
    return scaled
end

local function forward_from_angles(yaw, pitch)
    local cos_pitch = math.cos(pitch)
    return {
        math.cos(yaw) * cos_pitch,
        math.sin(pitch),
        math.sin(yaw) * cos_pitch,
    }
end

local function reset_physics_cube()
    if not physics_state.ready then
        return
    end
    if type(physics_set_transform) ~= "function" then
        return
    end
    local rotation = {0.0, 0.0, 0.0, 1.0}
    local ok, err = physics_set_transform(
        physics_state.cube_name,
        physics_state.cube_spawn,
        rotation)
    if not ok then
        log_debug("Physics reset failed: %s", err or "unknown")
        return
    end
    if type(physics_set_linear_velocity) == "function" then
        physics_set_linear_velocity(physics_state.cube_name, {0.0, 0.0, 0.0})
    end
    physics_state.last_step_time = nil
end

local function kick_physics_cube()
    if not physics_state.ready then
        return
    end
    if type(physics_apply_impulse) ~= "function" then
        return
    end
    local forward = forward_from_angles(camera.yaw, camera.pitch)
    local lift = math.max(forward[2], 0.2)
    local direction = normalize({forward[1], lift, forward[3]})
    local impulse = {
        direction[1] * physics_state.kick_strength,
        direction[2] * physics_state.kick_strength,
        direction[3] * physics_state.kick_strength,
    }
    local ok, err = physics_apply_impulse(physics_state.cube_name, impulse)
    if not ok then
        log_debug("Physics impulse failed: %s", err or "unknown")
    end
end

local atan2_available = type(math.atan2) == "function"
if not atan2_available then
    log_debug("math.atan2 unavailable; using fallback for compass heading")
end

local function atan2(y, x)
    if atan2_available then
        return math.atan2(y, x)
    end
    if x == 0.0 then
        if y > 0.0 then
            return math.pi / 2.0
        elseif y < 0.0 then
            return -math.pi / 2.0
        end
        return 0.0
    end
    local angle = math.atan(y / x)
    if x < 0.0 then
        if y >= 0.0 then
            angle = angle + math.pi
        else
            angle = angle - math.pi
        end
    end
    return angle
end

local function update_camera(dt)
    if not gui_input then
        return
    end

    local look_delta_x = gui_input.mouseDeltaX * controls.mouse_sensitivity
    local look_delta_y = -gui_input.mouseDeltaY * controls.mouse_sensitivity

    local pad = gui_input.gamepad
    if pad and pad.connected then
        local stick_x = apply_deadzone(pad.rightX, controls.stick_deadzone)
        local stick_y = apply_deadzone(pad.rightY, controls.stick_deadzone)
        look_delta_x = look_delta_x + stick_x * controls.gamepad_look_speed * dt
        look_delta_y = look_delta_y - stick_y * controls.gamepad_look_speed * dt
    end

    camera.yaw = camera.yaw + look_delta_x
    camera.pitch = clamp(camera.pitch + look_delta_y, -controls.max_pitch, controls.max_pitch)

    local forward = forward_from_angles(camera.yaw, camera.pitch)
    local forward_flat = normalize({forward[1], 0.0, forward[3]})
    local right = normalize(cross(forward_flat, world_up))

    local move_x = 0.0
    local move_z = 0.0
    local move_y = 0.0

    if gui_input.keyStates["move_forward"] then
        move_z = move_z + 1.0
    end
    if gui_input.keyStates["move_back"] then
        move_z = move_z - 1.0
    end
    if gui_input.keyStates["move_right"] then
        move_x = move_x + 1.0
    end
    if gui_input.keyStates["move_left"] then
        move_x = move_x - 1.0
    end

    if player_state.noclip then
        if gui_input.keyStates["fly_up"] or ui_state.flyUpActive or ui_state.flyUpPulse then
            move_y = move_y + 1.0
        end
        if gui_input.keyStates["fly_down"] or ui_state.flyDownActive or ui_state.flyDownPulse then
            move_y = move_y - 1.0
        end
    end
    ui_state.flyUpPulse = false
    ui_state.flyDownPulse = false

    if pad and pad.connected then
        move_x = move_x + apply_deadzone(pad.leftX, controls.stick_deadzone)
        move_z = move_z - apply_deadzone(pad.leftY, controls.stick_deadzone)
    end

    local length = math.sqrt(move_x * move_x + move_z * move_z)
    if length > 1.0 then
        move_x = move_x / length
        move_z = move_z / length
    end

    local toggle_pressed = gui_input.keyStates["noclip_toggle"]
    if toggle_pressed and not player_state.noclip_toggle_pressed then
        player_state.noclip = not player_state.noclip
        player_state.vertical_velocity = 0.0
        player_state.grounded = false
        log_debug("Noclip %s", player_state.noclip and "enabled" or "disabled")
    end
    player_state.noclip_toggle_pressed = toggle_pressed and true or false

    if player_state.noclip then
        local move_forward = forward_flat
        if controls.move_forward_uses_pitch then
            move_forward = forward
        end

        if length > 0.0 then
            local speed = controls.move_speed * dt
            if lua_debug and controls.move_forward_uses_pitch and math.abs(camera.pitch) > 0.001 then
                movement_log_cooldown = movement_log_cooldown - dt
                if movement_log_cooldown <= 0.0 then
                    log_debug("Move forward uses pitch: pitch=%.3f forward=(%.2f, %.2f, %.2f)",
                        camera.pitch, move_forward[1], move_forward[2], move_forward[3])
                    movement_log_cooldown = 0.5
                end
            end
            camera.position[1] = camera.position[1] + (right[1] * move_x + move_forward[1] * move_z) * speed
            camera.position[2] = camera.position[2] + (right[2] * move_x + move_forward[2] * move_z) * speed
            camera.position[3] = camera.position[3] + (right[3] * move_x + move_forward[3] * move_z) * speed
        end

        if move_y ~= 0.0 then
            camera.position[2] = camera.position[2] + move_y * controls.fly_speed * dt
        end
        return
    end

    if length > 0.0 then
        local speed = controls.move_speed * dt
        camera.position[1] = camera.position[1] + (right[1] * move_x + forward_flat[1] * move_z) * speed
        camera.position[3] = camera.position[3] + (right[3] * move_x + forward_flat[3] * move_z) * speed
    end

    local jump_pressed = gui_input.keyStates["jump"]
    if jump_pressed and not player_state.jump_pressed and player_state.grounded then
        player_state.vertical_velocity = controls.jump_speed
        player_state.grounded = false
    end
    player_state.jump_pressed = jump_pressed and true or false

    player_state.vertical_velocity = player_state.vertical_velocity + controls.gravity * dt
    if player_state.vertical_velocity < controls.max_fall_speed then
        player_state.vertical_velocity = controls.max_fall_speed
    end
    camera.position[2] = camera.position[2] + player_state.vertical_velocity * dt

    local floor_height = room.floor_top + player_state.eye_height
    if camera.position[2] <= floor_height then
        camera.position[2] = floor_height
        player_state.vertical_velocity = 0.0
        player_state.grounded = true
    else
        player_state.grounded = false
    end

    local room_limit = room.half_size - player_state.radius
    camera.position[1] = clamp(camera.position[1], -room_limit, room_limit)
    camera.position[3] = clamp(camera.position[3], -room_limit, room_limit)
end

local function update_audio_controls()
    if not gui_input then
        return
    end

    local pad = gui_input.gamepad
    local toggle_pressed = gui_input.keyStates["music_toggle"]
    if pad and pad.connected and pad.togglePressed then
        toggle_pressed = true
    end

    if toggle_pressed and not music_state.togglePressed then
        toggle_music()
    end

    music_state.togglePressed = toggle_pressed and true or false
end

local function resolve_material_shader()
    if type(config) ~= "table" then
        error("Missing config table for MaterialX shader selection")
    end
    local materialx = config_resolver.resolve_materialx(config)
    if type(materialx) ~= "table" or not materialx.enabled then
        error("MaterialX config missing or disabled; shader selection cannot proceed")
    end
    local materials = config_resolver.resolve_materialx_materials(config)
    if type(materials) == "table" and type(materials[1]) == "table" then
        local first_key = materials[1].shader_key
        if type(first_key) == "string" and first_key ~= "" then
            log_debug("Using first materialx materials shader_key=%s", first_key)
            return first_key
        end
    end
    error("MaterialX enabled but no materials shader_key found")
end

-- Delegate to framework
local function build_static_model_matrix(position, scale)
    return scene_framework.build_static_model_matrix(position, scale)
end

-- Apply color using current cube_vertices
local function apply_color_to_vertices(color)
    return scene_framework.apply_color_to_vertices(cube_vertices, color)
end

local function create_static_cube(position, scale, color, shader_key, object_type)
    local model = build_static_model_matrix(position, scale)
    local function compute_model_matrix()
        return model
    end

    local vertices = color and apply_color_to_vertices(color) or cube_vertices
    if type(shader_key) ~= "string" or shader_key == "" then
        error("create_static_cube requires a shader_key")
    end
    local resolved_shader = shader_key

    return {
        vertices = vertices,
        indices = cube_indices,
        compute_model_matrix = compute_model_matrix,
        shader_keys = {resolved_shader},
        object_type = object_type or "cube",
    }
end


local function create_physics_cube()
    if not ensure_physics_setup() then
        return nil
    end
    -- Use "solid" shader for physics cube to distinguish from floor
    local shader_key = "solid"
    local last_matrix = math3d.identity()
    local base_rotation_offset = math.pi / 4  -- Start with 45 degree rotation so it's visible immediately

    local function compute_model_matrix(time)
        step_physics(time)
        local transform, err = physics_get_transform(physics_state.cube_name)
        if not transform then
            if lua_debug then
                log_debug("physics_get_transform failed: %s", err or "unknown")
            end
            return last_matrix
        end
        
        -- Add rotation to physics cube so it spins while falling
        local spin_angle = base_rotation_offset + (time * physics_spin_speed)
        local spin_rotation = math3d.rotation_y(spin_angle)
        local physics_matrix = math3d.from_transform(transform.position, transform.rotation)
        local scale = scale_matrix(
            physics_state.cube_scale[1],
            physics_state.cube_scale[2],
            physics_state.cube_scale[3])
        
        -- Combine: translation from physics, spin rotation, then scale
        local matrix = math3d.multiply(physics_matrix, math3d.multiply(spin_rotation, scale))
        last_matrix = matrix
        return matrix
    end

    return {
        vertices = apply_color_to_vertices(physics_state.cube_color),
        indices = (#cube_indices_double_sided > 0) and cube_indices_double_sided or cube_indices,
        compute_model_matrix = compute_model_matrix,
        shader_keys = {shader_key},
        object_type = "physics_cube",
    }
end

local function create_spinning_cube()
    if not spinning_cube_enabled then
        return nil
    end
    local shader_key = resolve_material_shader()
    log_debug("Spinning cube shader=%s", shader_key)
    local function compute_model_matrix(time)
        local rotation = math3d.rotation_y(time * spinning_cube_rotation_speed)
        local scale = scale_matrix(spinning_cube_scale, spinning_cube_scale, spinning_cube_scale)
        local position = math3d.translation(0.0, spinning_cube_height, 0.0)
        return math3d.multiply(position, math3d.multiply(rotation, scale))
    end

    return {
        vertices = apply_color_to_vertices(spinning_cube_color),
        indices = cube_indices,
        compute_model_matrix = compute_model_matrix,
        shader_keys = {shader_key},
        object_type = "spinning_cube",
    }
end

local function create_dynamic_cube()
    local physics_cube = create_physics_cube()
    if physics_cube then
        return physics_cube
    end
    return create_spinning_cube()
end

local function create_lantern(x, z)
    return create_static_cube({x, lantern_height, z},
        {lantern_size, lantern_size, lantern_size}, lantern_color, "solid", "lantern")
end

local function create_room_objects()
    local floor_center_y = room.floor_top - room.floor_half_thickness
    local wall_center_y = room.floor_top + room.wall_height
    local ceiling_y = room.floor_top + room.wall_height * 2 + room.floor_half_thickness
    local wall_offset = room.half_size + room.wall_thickness
    local wall_inner_edge = wall_offset - room.wall_thickness
    local wall_outer_edge = wall_offset + room.wall_thickness
    log_debug("Room walls: inner=%.2f outer=%.2f", wall_inner_edge, wall_outer_edge)

    local floor_color = room.floor_color
    local wall_color = room.wall_color
    local ceiling_color = room.ceiling_color

    -- Generate proper floor and ceiling planes with tessellation
    local floor_vertices, floor_indices = generate_plane_mesh(
        room.half_size * 2, room.half_size * 2, room.floor_subdivisions, floor_color)
    local ceiling_vertices, ceiling_indices = generate_plane_mesh(
        room.half_size * 2, room.half_size * 2, room.floor_subdivisions, ceiling_color)
    
    -- Flip ceiling normals to face down
    scene_framework.flip_normals(ceiling_vertices)
    
    local function create_floor()
        local function compute_model_matrix()
            return build_static_model_matrix({0.0, floor_center_y, 0.0}, {1.0, 1.0, 1.0})
        end
        return {
            vertices = floor_vertices,
            indices = floor_indices,
            compute_model_matrix = compute_model_matrix,
            shader_keys = {"floor"},
            object_type = "floor",
        }
    end
    
    local function create_ceiling()
        local function compute_model_matrix()
            return build_static_model_matrix({0.0, ceiling_y, 0.0}, {1.0, 1.0, 1.0})
        end
        return {
            vertices = ceiling_vertices,
            indices = ceiling_indices,
            compute_model_matrix = compute_model_matrix,
            shader_keys = {"ceiling"},
            object_type = "ceiling",
        }
    end

    local objects = {
        create_floor(),
        create_ceiling(),
        create_static_cube({0.0, wall_center_y, -wall_offset},
            {room.half_size, room.wall_height, room.wall_thickness}, wall_color, "wall", "wall"),
        create_static_cube({0.0, wall_center_y, wall_offset},
            {room.half_size, room.wall_height, room.wall_thickness}, wall_color, "wall", "wall"),
        create_static_cube({-wall_offset, wall_center_y, 0.0},
            {room.wall_thickness, room.wall_height, room.half_size}, wall_color, "wall", "wall"),
        create_static_cube({wall_offset, wall_center_y, 0.0},
            {room.wall_thickness, room.wall_height, room.half_size}, wall_color, "wall", "wall"),
    }

    if lantern_enabled then
        local corner_offset = room.half_size - lantern_corner_offset
        local wall_offset = room.half_size - lantern_wall_offset
        objects[#objects + 1] = create_lantern(corner_offset, corner_offset)
        objects[#objects + 1] = create_lantern(-corner_offset, corner_offset)
        objects[#objects + 1] = create_lantern(corner_offset, -corner_offset)
        objects[#objects + 1] = create_lantern(-corner_offset, -corner_offset)

        objects[#objects + 1] = create_lantern(0.0, wall_offset)
        objects[#objects + 1] = create_lantern(0.0, -wall_offset)
        objects[#objects + 1] = create_lantern(wall_offset, 0.0)
        objects[#objects + 1] = create_lantern(-wall_offset, 0.0)
    end

    return objects
end

local room_objects = create_room_objects()

local function heading_from_yaw(yaw)
    local forward = forward_from_angles(yaw, 0.0)
    local heading = math.deg(atan2(forward[1], -forward[3])) % 360
    return heading
end

local function heading_to_cardinal(degrees)
    local directions = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"}
    local index = math.floor((degrees + 22.5) / 45.0) % 8 + 1
    return directions[index]
end

local function draw_compass_widget()
    local size = compass_layout.size
    local x = ui_layout.width - size - ui_layout.margin
    local y = ui_layout.margin
    local rect = {x = x, y = y, width = size, height = size}
    gui_context:pushRect(rect, {
        color = {0.06, 0.07, 0.09, 0.88},
        borderColor = {0.35, 0.38, 0.42, 1.0},
    })

    Gui.text(gui_context, {
        x = x,
        y = y + 2,
        width = size,
        height = compass_layout.label_height,
    }, "Compass", {
        fontSize = 12,
        alignX = "center",
        color = {0.82, 0.88, 0.95, 1.0},
    })

    local ring_rect = {
        x = x + compass_layout.padding,
        y = y + compass_layout.label_height,
        width = size - compass_layout.padding * 2,
        height = size - compass_layout.label_height - compass_layout.padding,
    }
    local center_x = ring_rect.x + ring_rect.width / 2
    local center_y = ring_rect.y + ring_rect.height / 2
    local radius = math.min(ring_rect.width, ring_rect.height) / 2 - 6

    Gui.text(gui_context, {x = center_x - 8, y = ring_rect.y - 2, width = 16, height = 14}, "N", {
        fontSize = 12,
        alignX = "center",
        color = {0.78, 0.82, 0.88, 1.0},
    })
    Gui.text(gui_context, {x = ring_rect.x + ring_rect.width - 12, y = center_y - 7, width = 14, height = 14}, "E", {
        fontSize = 12,
        alignX = "center",
        color = {0.78, 0.82, 0.88, 1.0},
    })
    Gui.text(gui_context, {x = center_x - 8, y = ring_rect.y + ring_rect.height - 12, width = 16, height = 14}, "S", {
        fontSize = 12,
        alignX = "center",
        color = {0.78, 0.82, 0.88, 1.0},
    })
    Gui.text(gui_context, {x = ring_rect.x - 2, y = center_y - 7, width = 14, height = 14}, "W", {
        fontSize = 12,
        alignX = "center",
        color = {0.78, 0.82, 0.88, 1.0},
    })

    local heading = heading_from_yaw(camera.yaw)
    local heading_int = math.floor(heading + 0.5) % 360
    local direction = heading_to_cardinal(heading)
    local angle = math.rad(heading) - math.pi / 2.0
    local needle_x = center_x + math.cos(angle) * radius
    local needle_y = center_y + math.sin(angle) * radius

    gui_context:pushRect({
        x = needle_x - 3,
        y = needle_y - 3,
        width = 6,
        height = 6,
    }, {
        color = {0.98, 0.78, 0.35, 1.0},
        radius = 2,
    })

    Gui.text(gui_context, {
        x = x,
        y = y + size - 18,
        width = size,
        height = 16,
    }, string_format("%03d deg %s", heading_int, direction), {
        fontSize = 11,
        alignX = "center",
        color = {0.9, 0.92, 0.95, 1.0},
    })
end

local function draw_flight_buttons()
    local x = ui_layout.width - flight_layout.width - ui_layout.margin
    local y = ui_layout.margin * 2 + compass_layout.size

    local up_clicked = Gui.button(gui_context, "fly_up", {
        x = x,
        y = y,
        width = flight_layout.width,
        height = flight_layout.height,
    }, "Fly Up")
    local down_clicked = Gui.button(gui_context, "fly_down", {
        x = x,
        y = y + flight_layout.height + flight_layout.spacing,
        width = flight_layout.width,
        height = flight_layout.height,
    }, "Fly Down")

    ui_state.flyUpActive = gui_context.activeWidget == "fly_up"
    ui_state.flyDownActive = gui_context.activeWidget == "fly_down"
    ui_state.flyUpPulse = up_clicked
    ui_state.flyDownPulse = down_clicked
end

local function draw_physics_buttons()
    if not physics_state.enabled then
        return
    end
    local x = ui_layout.width - physics_layout.width - ui_layout.margin
    local y = ui_layout.margin * 3 + compass_layout.size
        + flight_layout.height * 2 + flight_layout.spacing

    Gui.text(gui_context, {
        x = x,
        y = y,
        width = physics_layout.width,
        height = 16,
    }, "Physics", {
        fontSize = 12,
        alignX = "center",
        color = {0.82, 0.88, 0.95, 1.0},
    })

    local kick_clicked = Gui.button(gui_context, "physics_kick", {
        x = x,
        y = y + 18,
        width = physics_layout.width,
        height = physics_layout.height,
    }, "Kick Cube")
    if kick_clicked then
        kick_physics_cube()
    end

    local reset_clicked = Gui.button(gui_context, "physics_reset", {
        x = x,
        y = y + 18 + physics_layout.height + physics_layout.spacing,
        width = physics_layout.width,
        height = physics_layout.height,
    }, "Reset Cube")
    if reset_clicked then
        reset_physics_cube()
    end
end

function get_scene_objects()
    local objects = {}
    for i = 1, #room_objects do
        objects[#objects + 1] = room_objects[i]
    end
    local dynamic_cube = create_dynamic_cube()
    if dynamic_cube then
        objects[#objects + 1] = dynamic_cube
    end
    return objects
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

    update_camera(dt)
    update_audio_controls()

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

function get_gui_commands()
    gui_context:beginFrame(gui_input)
    draw_compass_widget()
    draw_flight_buttons()
    draw_physics_buttons()
    gui_context:endFrame()
    return gui_context:getCommands()
end
