local Gui = require('gui')
local math3d = require('math3d')

local ctx = Gui.newContext()
local input = Gui.newInputState()

local function log_trace(fmt, ...)
    if not lua_debug or not fmt then
        return
    end
    print(string.format(fmt, ...))
end

local buttonStates = {
    button1 = false,
    button2 = false,
    button3 = false,
    button4 = false
}

local statusMessage = 'Ready'
local viewProjectionLogged = false

local shader_variants = {
    default = {
        vertex = "shaders/gui_2d.vert.spv",
        fragment = "shaders/gui_2d.frag.spv",
    },
}

local function drawTestButtons()
    -- Background panel
    ctx:pushRect({x = 50, y = 50, width = 400, height = 400}, {
        color = {0.1, 0.1, 0.1, 0.9},
        borderColor = {0.5, 0.5, 0.5, 1.0},
    })

    -- Title
    Gui.text(ctx, {x = 70, y = 70, width = 360, height = 40}, "2D Test Buttons", {
        fontSize = 28,
        alignX = "center",
        color = {1.0, 1.0, 1.0, 1.0},
    })

    -- Button 1
    if Gui.button(ctx, "btn1", {x = 100, y = 130, width = 150, height = 50}, "Button 1") then
        buttonStates.button1 = not buttonStates.button1
        statusMessage = "Button 1 " .. (buttonStates.button1 and "pressed" or "released")
        log_trace("Button 1 clicked: %s", statusMessage)
    end

    -- Button 2
    if Gui.button(ctx, "btn2", {x = 270, y = 130, width = 150, height = 50}, "Button 2") then
        buttonStates.button2 = not buttonStates.button2
        statusMessage = "Button 2 " .. (buttonStates.button2 and "pressed" or "released")
        log_trace("Button 2 clicked: %s", statusMessage)
    end

    -- Button 3
    if Gui.button(ctx, "btn3", {x = 100, y = 200, width = 150, height = 50}, "Button 3") then
        buttonStates.button3 = not buttonStates.button3
        statusMessage = "Button 3 " .. (buttonStates.button3 and "pressed" or "released")
        log_trace("Button 3 clicked: %s", statusMessage)
    end

    -- Button 4
    if Gui.button(ctx, "btn4", {x = 270, y = 200, width = 150, height = 50}, "Button 4") then
        buttonStates.button4 = not buttonStates.button4
        statusMessage = "Button 4 " .. (buttonStates.button4 and "pressed" or "released")
        log_trace("Button 4 clicked: %s", statusMessage)
    end

    -- Status display
    Gui.text(ctx, {x = 70, y = 280, width = 360, height = 30}, "Status: " .. statusMessage, {
        fontSize = 18,
        alignX = "center",
        color = {0.8, 0.8, 1.0, 1.0},
    })

    -- Button states display
    local statesText = string.format("States: 1:%s 2:%s 3:%s 4:%s",
        buttonStates.button1 and "ON" or "OFF",
        buttonStates.button2 and "ON" or "OFF",
        buttonStates.button3 and "ON" or "OFF",
        buttonStates.button4 and "ON" or "OFF")
    Gui.text(ctx, {x = 70, y = 320, width = 360, height = 30}, statesText, {
        fontSize = 16,
        alignX = "center",
        color = {1.0, 0.8, 0.8, 1.0},
    })

    -- Reset button
    if Gui.button(ctx, "reset", {x = 175, y = 370, width = 100, height = 40}, "Reset") then
        buttonStates.button1 = false
        buttonStates.button2 = false
        buttonStates.button3 = false
        buttonStates.button4 = false
        statusMessage = "All buttons reset"
        log_trace("Reset button clicked")
    end
end

gui_context = ctx
gui_input = input

function get_scene_objects()
    return {}  -- No 3D objects
end

function get_shader_paths()
    log_trace("GUI demo shader variants: default vertex=%s fragment=%s",
        shader_variants.default.vertex,
        shader_variants.default.fragment)
    return shader_variants
end

function get_view_projection(aspect)
    if not viewProjectionLogged then
        log_trace("GUI demo view projection: identity (aspect=%.2f)", aspect)
        viewProjectionLogged = true
    end
    return math3d.identity()
end

function get_gui_commands()
    ctx:beginFrame(input)
    drawTestButtons()
    Gui.cursor(ctx, input, {
        size = 16,
        thickness = 2,
        color = {1.0, 0.9, 0.2, 1.0},
        activeColor = {1.0, 0.35, 0.15, 1.0},
    })
    ctx:endFrame()
    return ctx:getCommands()
end
