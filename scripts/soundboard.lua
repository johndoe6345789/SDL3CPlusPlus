local Gui = require("gui")
local math3d = require("math3d")

local ctx = Gui.newContext()
local input = Gui.newInputState()
local statusMessage = "Select a clip to play"

local function findScriptDirectory()
    local info = debug.getinfo(1, "S")
    local source = info.source or ""
    if source:sub(1, 1) == "@" then
        source = source:sub(2)
    end
    local dir = source:match("(.+)[/\\]")
    return dir or "."
end

local scriptDir = findScriptDirectory()
local isWindows = package.config:sub(1, 1) == "\\"

local function joinPath(...)
    local parts = { ... }
    if #parts == 0 then
        return ""
    end
    local sep = package.config:sub(1, 1)
    local path = parts[1] or ""
    for i = 2, #parts do
        local part = parts[i] or ""
        if part ~= "" then
            path = path:gsub("[/\\]+$", "")
            part = part:gsub("^[/\\]+", "")
            if part ~= "" then
                if path == "" then
                    path = part
                else
                    path = path .. sep .. part
                end
            end
        end
    end
    return path
end

local function escapeForCommand(text)
    return (text or ""):gsub('"', '\\"')
end

local function listDirectory(fullPath)
    local command
    local escaped = escapeForCommand(fullPath)
    if isWindows then
        command = string.format('dir /b /a:-d "%s"', escaped)
    else
        command = string.format('ls -1 "%s"', escaped)
    end
    local handle = io.popen(command)
    if not handle then
        return {}
    end
    local entries = {}
    for line in handle:lines() do
        local trimmed = line:gsub("^%s+", ""):gsub("%s+$", "")
        if trimmed ~= "" then
            table.insert(entries, trimmed)
        end
    end
    handle:close()
    return entries
end

local function collectOggFiles(relativeDir)
    local fullDir = joinPath(scriptDir, relativeDir)
    local entries = listDirectory(fullDir)
    local clips = {}
    for _, entry in ipairs(entries) do
        local lower = entry:lower()
        if lower:sub(-4) == ".ogg" then
            local candidate = joinPath(fullDir, entry)
            local handle = io.open(candidate, "rb")
            if handle then
                handle:close()
                table.insert(clips, entry)
            end
        end
    end
    table.sort(clips, function(a, b)
        return a:lower() < b:lower()
    end)
    return clips
end

local function prettyClipName(name)
    local base = name:gsub("%.[^%.]+$", "")
    base = base:gsub("[_%-]+", " ")
    base = base:gsub("(%S+)", function(word)
        local first = word:sub(1, 1)
        local rest = word:sub(2)
        return first:upper() .. rest:lower()
    end)
    return base
end

local function widgetIdForClip(categoryIndex, clipName)
    local sanitized = clipName:gsub("[^%w]+", "_")
    return "clip_" .. categoryIndex .. "_" .. sanitized
end

local categories = {
    {
        name = "Sound Effects",
        relativeDir = "assets/audio/sfx",
        files = collectOggFiles("assets/audio/sfx"),
    },
    {
        name = "Speech",
        relativeDir = "assets/audio/tts",
        files = collectOggFiles("assets/audio/tts"),
    },
}

local function playClip(relativeDir, clipName)
    local label = prettyClipName(clipName)
    if type(audio_play_sound) ~= "function" then
        statusMessage = "Audio playback is unavailable"
        return
    end
    local clipPath = relativeDir .. "/" .. clipName
    local ok, err = pcall(audio_play_sound, clipPath, false)
    if ok then
        statusMessage = "Playing \"" .. label .. "\""
    else
        statusMessage = "Failed to play \"" .. label .. "\": " .. tostring(err)
    end
end

local function drawSoundboardPanel()
    local panel = { x = 16, y = 16, width = 644, height = 520 }
    ctx:pushRect(panel, {
        color = { 0.06, 0.07, 0.09, 0.95 },
        borderColor = { 0.35, 0.38, 0.42, 1.0 },
    })
    Gui.text(ctx, { x = panel.x + 20, y = panel.y + 20, width = panel.width - 40, height = 24 },
        "Audio Soundboard", {
            fontSize = 24,
            color = { 0.96, 0.96, 0.97, 1.0 },
            alignX = "left",
        })
    Gui.text(ctx, { x = panel.x + 20, y = panel.y + 46, width = panel.width - 40, height = 18 },
        "Trigger generated SFX or Piper TTS clips from the assets folder.", {
            fontSize = 14,
            color = { 0.7, 0.75, 0.8, 1.0 },
            alignX = "left",
        })

    local columnY = panel.y + 80
    local columnWidth = 300
    local columnSpacing = 24
    local buttonHeight = 36
    local buttonSpacing = 12

    for index, category in ipairs(categories) do
        local columnX = panel.x + 20 + (index - 1) * (columnWidth + columnSpacing)
        Gui.text(ctx, { x = columnX, y = columnY, width = columnWidth, height = 28 }, category.name, {
            fontSize = 20,
            color = { 0.9, 0.9, 0.95, 1.0 },
            alignX = "left",
        })

        local buttonY = columnY + 32
        if #category.files == 0 then
            Gui.text(ctx, { x = columnX, y = buttonY, width = columnWidth, height = buttonHeight },
                "No clips available", {
                    color = { 0.5, 0.55, 0.6, 1.0 },
                    alignX = "left",
                    alignY = "center",
                })
        else
            for _, clipName in ipairs(category.files) do
                local label = prettyClipName(clipName)
                if Gui.button(ctx, widgetIdForClip(index, clipName), {
                    x = columnX,
                    y = buttonY,
                    width = columnWidth,
                    height = buttonHeight,
                }, label) then
                    playClip(category.relativeDir, clipName)
                end
                buttonY = buttonY + buttonHeight + buttonSpacing
            end
        end
    end

    Gui.text(ctx, {
        x = panel.x + 20,
        y = panel.y + panel.height - 34,
        width = panel.width - 40,
        height = 24,
    }, statusMessage, {
        fontSize = 14,
        color = { 0.6, 0.8, 1.0, 1.0 },
        alignX = "left",
        alignY = "center",
    })
end

local cubeVertices = {
    { position = { -1.0, -1.0, -1.0 }, color = { 1.0, 0.2, 0.4 } },
    { position = { 1.0, -1.0, -1.0 }, color = { 0.2, 0.9, 0.4 } },
    { position = { 1.0, 1.0, -1.0 }, color = { 0.3, 0.4, 0.9 } },
    { position = { -1.0, 1.0, -1.0 }, color = { 1.0, 0.8, 0.2 } },
    { position = { -1.0, -1.0, 1.0 }, color = { 0.9, 0.4, 0.5 } },
    { position = { 1.0, -1.0, 1.0 }, color = { 0.4, 0.9, 0.5 } },
    { position = { 1.0, 1.0, 1.0 }, color = { 0.6, 0.8, 1.0 } },
    { position = { -1.0, 1.0, 1.0 }, color = { 0.4, 0.4, 0.4 } },
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
        vertex = "shaders/cube.vert.spv",
        fragment = "shaders/cube.frag.spv",
    },
}

local camera = {
    eye = { 2.0, 2.0, 3.0 },
    center = { 0.0, 0.0, 0.0 },
    up = { 0.0, 1.0, 0.0 },
    fov = 0.78,
    near = 0.1,
    far = 10.0,
}

local rotationSpeeds = { x = 0.45, y = 0.65 }

local function buildModel(time)
    local yRot = math3d.rotation_y(time * rotationSpeeds.y)
    local xRot = math3d.rotation_x(time * rotationSpeeds.x)
    return math3d.multiply(yRot, xRot)
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
        shader_key = "default",
    }
end

gui_context = ctx
gui_input = input

function get_scene_objects()
    return { createCube({ 0.0, 0.0, -4.0 }) }
end

function get_shader_paths()
    return shaderVariants
end

function get_view_projection(aspect)
    local view = math3d.look_at(camera.eye, camera.center, camera.up)
    local projection = math3d.perspective(camera.fov, aspect, camera.near, camera.far)
    return math3d.multiply(projection, view)
end

function get_gui_commands()
    ctx:beginFrame(input)
    drawSoundboardPanel()
    ctx:endFrame()
    return ctx:getCommands()
end
