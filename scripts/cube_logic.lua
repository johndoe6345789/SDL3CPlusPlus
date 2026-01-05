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
local Gui = require("gui")
local string_format = string.format

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
local ui_layout = {
    width = 1024,
    height = 768,
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
end

local shader_variants = {
    default = {
        vertex = "shaders/cube.vert.spv",
        fragment = "shaders/cube.frag.spv",
    },
    solid = {
        vertex = "shaders/solid.vert.spv",
        fragment = "shaders/solid.frag.spv",
    },
}

local camera = {
    position = {0.0, 0.0, 5.0},
    yaw = math.pi,  -- Face toward -Z (center of room)
    pitch = 0.0,
    fov = 0.78,
    near = 0.1,
    far = 50.0,
}

local controls = {
    move_speed = 4.0,
    fly_speed = 3.0,
    jump_speed = 5.5,
    gravity = -12.0,
    max_fall_speed = -20.0,
    mouse_sensitivity = 0.0025,
    gamepad_look_speed = 2.5,
    stick_deadzone = 0.2,
    max_pitch = math.rad(85.0),
    move_forward_uses_pitch = true,
}

local last_frame_time = nil
local movement_log_cooldown = 0.0
local world_up = {0.0, 1.0, 0.0}
local room = {
    half_size = 15.0,
    wall_thickness = 0.5,
    wall_height = 4.0,
    floor_half_thickness = 0.3,
    floor_top = 0.0,
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

camera.position[1] = 0.0
camera.position[2] = room.floor_top + player_state.eye_height
camera.position[3] = 10.0

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

local rotation_speed = 0.9

local function create_spinning_cube()
    local function compute_model_matrix(time)
        local rotation = math3d.rotation_y(time * rotation_speed)
        local scale = scale_matrix(1.5, 1.5, 1.5)  -- Make cube 3x3x3 units
        local position = math3d.translation(0.0, 3.0, 0.0)
        return math3d.multiply(position, math3d.multiply(rotation, scale))
    end

    return {
        vertices = cube_vertices,
        indices = cube_indices,
        compute_model_matrix = compute_model_matrix,
        shader_key = "default",  -- Rainbow shader for spinning cube
    }
end

local function build_static_model_matrix(position, scale)
    local translation = math3d.translation(position[1], position[2], position[3])
    local scaling = scale_matrix(scale[1], scale[2], scale[3])
    return math3d.multiply(translation, scaling)
end

local function apply_color_to_vertices(color)
    local colored_vertices = {}
    for i = 1, #cube_vertices do
        local v = cube_vertices[i]
        colored_vertices[i] = {
            position = v.position,
            color = color,
        }
    end
    return colored_vertices
end

local function create_static_cube(position, scale, color)
    local model = build_static_model_matrix(position, scale)
    local function compute_model_matrix()
        return model
    end

    local vertices = color and apply_color_to_vertices(color) or cube_vertices

    return {
        vertices = vertices,
        indices = cube_indices,
        compute_model_matrix = compute_model_matrix,
        shader_key = "solid",  -- Use solid color shader for room objects
    }
end

local function create_lantern(x, z)
    local lantern_height = 4.5
    local lantern_size = 0.2
    return create_static_cube({x, lantern_height, z},
        {lantern_size, lantern_size, lantern_size}, {1.0, 0.9, 0.6})
end

local function create_room_objects()
    local floor_center_y = room.floor_top - room.floor_half_thickness
    local wall_center_y = room.floor_top + room.wall_height
    local ceiling_y = room.floor_top + room.wall_height * 2 + room.floor_half_thickness
    local wall_offset = room.half_size - room.wall_thickness / 2

    local blue_floor = {0.2, 0.3, 0.8}
    local green_wall = {0.2, 0.7, 0.3}
    local light_gray = {0.7, 0.7, 0.7}

    local objects = {
        create_static_cube({0.0, floor_center_y, 0.0},
            {room.half_size, room.floor_half_thickness, room.half_size}, blue_floor),
        create_static_cube({0.0, ceiling_y, 0.0},
            {room.half_size, room.floor_half_thickness, room.half_size}, light_gray),
        create_static_cube({0.0, wall_center_y, -wall_offset},
            {room.half_size, room.wall_height, room.wall_thickness}, green_wall),
        create_static_cube({0.0, wall_center_y, wall_offset},
            {room.half_size, room.wall_height, room.wall_thickness}, green_wall),
        create_static_cube({-wall_offset, wall_center_y, 0.0},
            {room.wall_thickness, room.wall_height, room.half_size}, green_wall),
        create_static_cube({wall_offset, wall_center_y, 0.0},
            {room.wall_thickness, room.wall_height, room.half_size}, green_wall),
    }

    -- Add lanterns in the four corners
    local lantern_offset = 4.0
    objects[#objects + 1] = create_lantern(lantern_offset, lantern_offset)
    objects[#objects + 1] = create_lantern(-lantern_offset, lantern_offset)
    objects[#objects + 1] = create_lantern(lantern_offset, -lantern_offset)
    objects[#objects + 1] = create_lantern(-lantern_offset, -lantern_offset)

    -- Add lanterns on the walls (midpoints)
    objects[#objects + 1] = create_lantern(0.0, lantern_offset)
    objects[#objects + 1] = create_lantern(0.0, -lantern_offset)
    objects[#objects + 1] = create_lantern(lantern_offset, 0.0)
    objects[#objects + 1] = create_lantern(-lantern_offset, 0.0)

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

function get_scene_objects()
    local objects = {
        create_spinning_cube(),
    }
    for i = 1, #room_objects do
        objects[#objects + 1] = room_objects[i]
    end
    return objects
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

function get_gui_commands()
    gui_context:beginFrame(gui_input)
    draw_compass_widget()
    draw_flight_buttons()
    gui_context:endFrame()
    return gui_context:getCommands()
end
