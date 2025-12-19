local Gui = require(\ gui\)
local math3d = require(\math3d\)

local ctx = Gui.newContext()
local input = Gui.newInputState()
local textState = Gui.newTextState(\\)
local listState = Gui.newListState()
local items = {
    \Dashboard Setup\,
    \Input Streams\,
    \Telemetry\,
    \Power Profile\,
    \Diagnostics\,
    \Release Notes\,
}
local statusMessage = \Idle\
local selectedItem = items[1]
local rotationSpeeds = {x = 0.45, y = 0.65}

local cubeVertices = {
    { position = {-1.0, -1.0, -1.0}, color = {1.0, 0.2, 0.4} },
    { position = {1.0, -1.0, -1.0}, color = {0.2, 0.9, 0.4} },
    { position = {1.0, 1.0, -1.0}, color = {0.3, 0.4, 0.9} },
    { position = {-1.0, 1.0, -1.0}, color = {1.0, 0.8, 0.2} },
    { position = {-1.0, -1.0, 1.0}, color = {0.9, 0.4, 0.5} },
    { position = {1.0, -1.0, 1.0}, color = {0.4, 0.9, 0.5} },
    { position = {1.0, 1.0, 1.0}, color = {0.6, 0.8, 1.0} },
    { position = {-1.0, 1.0, 1.0}, color = {0.4, 0.4, 0.4} },
}

local cubeIndices = {
    1, 2, 3, 3, 4, 1,
    5, 6, 7, 7, 8, 5,
    1, 5, 8, 8, 4, 1,
    2, 6, 7, 7, 3, 2,
    4, 3, 7, 7, 8, 4,
    1, 2, 6, 6, 5, 1,
}

local shaderVariants = {
    default = {
        vertex = \shaders/cube.vert.spv\,
        fragment = \shaders/cube.frag.spv\,
    },
}

local camera = {
    eye = {2.0, 2.0, 3.0},
    center = {0.0, 0.0, 0.0},
    up = {0.0, 1.0, 0.0},
    fov = 0.78,
    near = 0.1,
    far = 10.0,
}

local function buildModel(time)
    local y = math3d.rotation_y(time * rotationSpeeds.y)
    local x = math3d.rotation_x(time * rotationSpeeds.x)
    return math3d.multiply(y, x)
end

local function createCube(position)
    local function computeModel(time)
        local base = buildModel(time)
        local offset = math3d.translation(position[1], position[2], position[3])
        return math3d.multiply(offset, base)
    end
    return {
        vertices = cubeVertices,
        indices = cubeIndices,
        compute_model_matrix = computeModel,
        shader_key = \default\,
    }
end

gui_context = ctx
gui_input = input

function get_scene_objects()
    return { createCube({0.0, 0.0, -4.0}) }
end

function get_shader_paths()
    return shaderVariants
end

function get_view_projection(aspect)
    local view = math3d.look_at(camera.eye, camera.center, camera.up)
    local projection = math3d.perspective(camera.fov, aspect, camera.near, camera.far)
    return math3d.multiply(projection, view)
end

local function drawPanel()
    ctx:pushRect({x = 10, y = 10, width = 460, height = 520}, {
        color = {0.06, 0.07, 0.09, 0.9},
        borderColor = {0.35, 0.38, 0.42, 1.0},
    })
    Gui.text(ctx, {x = 30, y = 30, width = 420, height = 30}, \Command Console\, {
        fontSize = 24,
        alignX = \left\,
        color = {0.95, 0.95, 0.95, 1.0},
    })
    Gui.svg(ctx, {x = 320, y = 30, width = 120, height = 120}, \assets/logo.svg\)
    textState = Gui.textbox(ctx, \search_field\, {x = 30, y = 80, width = 420, height = 40}, textState, {
        placeholder = \Filter modules...\,
        onSubmit = function(text)
            statusMessage = \Searching for: \ .. (text ~= \\ and text or \anything\)
        end,
    })

    listState = Gui.listView(ctx, \menu_list\, {x = 30, y = 140, width = 420, height = 240}, items, listState, {
        onSelect = function(idx, item)
            selectedItem = item
            statusMessage = \Ready to adjust \ .. item
        end,
        scrollToSelection = true,
    })

    if Gui.button(ctx, \apply\, {x = 30, y = 400, width = 200, height = 38}, \Apply Settings\) then
        statusMessage = \Applied configuration for \ .. (selectedItem or \–\)
    end

    Gui.text(ctx, {x = 30, y = 448, width = 420, height = 24}, \Status: \ .. statusMessage, {
        color = {0.6, 0.9, 1.0, 1.0},
        alignY = \top\,
    })
end

function get_gui_commands()
    ctx:beginFrame(input)
    drawPanel()
    ctx:endFrame()
    return ctx:getCommands()
end
