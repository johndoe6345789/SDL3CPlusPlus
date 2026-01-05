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

local music_state = {
    playing = false,
    togglePressed = false,
}

local function start_music()
    if type(audio_play_background) == "function" then
        audio_play_background("modmusic.ogg", true)
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

local math3d = require("math3d")
local string_format = string.format

local InputState = {}
InputState.__index = InputState

function InputState:new()
    local instance = {
        mouseX = 0.0,
        mouseY = 0.0,
        mouseDeltaX = 0.0,
        mouseDeltaY = 0.0,
        mouseDown = false,
        wheel = 0.0,
        textInput = "",
        keyStates = {},
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

function InputState:setMouse(x, y, isDown)
    if self.lastMouseX ~= nil and self.lastMouseY ~= nil then
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

local shader_variants = {
    default = {
        vertex = "shaders/cube.vert.spv",
        fragment = "shaders/cube.frag.spv",
    },
}

local camera = {
    position = {0.0, 0.0, 5.0},
    yaw = -math.pi / 2.0,
    pitch = 0.0,
    fov = 0.78,
    near = 0.1,
    far = 50.0,
}

local controls = {
    move_speed = 4.0,
    mouse_sensitivity = 0.0025,
    gamepad_look_speed = 2.5,
    stick_deadzone = 0.2,
    max_pitch = math.rad(85.0),
}

local last_frame_time = nil
local world_up = {0.0, 1.0, 0.0}

local function clamp(value, minValue, maxValue)
    if value < minValue then
        return minValue
    end
    if value > maxValue then
        return maxValue
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

    if gui_input.keyStates["w"] then
        move_z = move_z + 1.0
    end
    if gui_input.keyStates["s"] then
        move_z = move_z - 1.0
    end
    if gui_input.keyStates["d"] then
        move_x = move_x + 1.0
    end
    if gui_input.keyStates["a"] then
        move_x = move_x - 1.0
    end

    if pad and pad.connected then
        move_x = move_x + apply_deadzone(pad.leftX, controls.stick_deadzone)
        move_z = move_z - apply_deadzone(pad.leftY, controls.stick_deadzone)
    end

    local length = math.sqrt(move_x * move_x + move_z * move_z)
    if length > 1.0 then
        move_x = move_x / length
        move_z = move_z / length
    end

    if length > 0.0 then
        local speed = controls.move_speed * dt
        camera.position[1] = camera.position[1] + (right[1] * move_x + forward_flat[1] * move_z) * speed
        camera.position[2] = camera.position[2] + (right[2] * move_x + forward_flat[2] * move_z) * speed
        camera.position[3] = camera.position[3] + (right[3] * move_x + forward_flat[3] * move_z) * speed
    end
end

local function update_audio_controls()
    if not gui_input then
        return
    end

    local pad = gui_input.gamepad
    local toggle_pressed = gui_input.keyStates["m"]
    if pad and pad.connected and pad.togglePressed then
        toggle_pressed = true
    end

    if toggle_pressed and not music_state.togglePressed then
        toggle_music()
    end

    music_state.togglePressed = toggle_pressed and true or false
end

local rotation_speed = 0.9

local function create_spinning_cube()
    local function compute_model_matrix(time)
        return math3d.rotation_y(time * rotation_speed)
    end

    return {
        vertices = cube_vertices,
        indices = cube_indices,
        compute_model_matrix = compute_model_matrix,
        shader_key = "default",
    }
end

function get_scene_objects()
    return {
        create_spinning_cube(),
    }
end

function get_shader_paths()
    return shader_variants
end

function get_view_projection(aspect)
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
    update_audio_controls()

    local forward = forward_from_angles(camera.yaw, camera.pitch)
    local center = {
        camera.position[1] + forward[1],
        camera.position[2] + forward[2],
        camera.position[3] + forward[3],
    }

    local view = math3d.look_at(camera.position, center, world_up)
    local projection = math3d.perspective(camera.fov, aspect, camera.near, camera.far)
    return math3d.multiply(projection, view)
end
