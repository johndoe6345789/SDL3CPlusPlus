-- Config Selector GUI
-- Allows users to select from available JSON configuration files

local Gui = require('gui')
local ctx = Gui.newContext()
local input = Gui.newInputState()

local function log_trace(fmt, ...)
    if not lua_debug or not fmt then
        return
    end
    print(string.format(fmt, ...))
end

-- Config selector state
local configSelector = {
    availableConfigs = {},
    selectedIndex = 1,
    selectedConfig = nil,
    confirmed = false,
    windowWidth = 800,  -- Default desktop resolution
    windowHeight = 600,
    title = "Select Configuration",
    description = "Choose a runtime configuration to load:",
    statusMessage = "Use arrow keys to navigate, Enter to select, Escape to cancel"
}

-- Scan for available config files
local function scanConfigFiles()
    configSelector.availableConfigs = {}

    -- Try to read config directory
    local configDir = "config"
    if not file_exists then
        log_trace("file_exists function not available, using fallback config list")
        -- Fallback list for when file_exists isn't available
        configSelector.availableConfigs = {
            {name = "GUI Demo", path = "config/gui_runtime.json", description = "Interactive GUI demonstration"},
            {name = "3D Cube Demo", path = "config/seed_runtime.json", description = "3D cube room with physics"},
            {name = "Soundboard", path = "config/soundboard_runtime.json", description = "Audio playback demo"},
            {name = "Vita GUI Demo", path = "config/vita_gui_runtime.json", description = "Vita-optimized GUI demo"},
            {name = "Vita Cube Demo", path = "config/vita_cube_runtime.json", description = "Vita-optimized 3D demo"},
            {name = "Vita Soundboard", path = "config/vita_soundboard_runtime.json", description = "Vita-optimized audio demo"}
        }
        return
    end

    -- Scan config directory for JSON files
    local files = list_directory(configDir)
    if not files then
        log_trace("Could not scan config directory: %s", configDir)
        return
    end

    for _, filename in ipairs(files) do
        if string.match(filename, "%.json$") then
            local filepath = configDir .. "/" .. filename
            local configName = string.gsub(filename, "_runtime%.json$", "")
            configName = string.gsub(configName, "_", " ")
            configName = string.gsub(configName, "^%l", string.upper) -- Capitalize first letter

            -- Try to read and parse the config for description
            local description = "Runtime configuration"
            if load_json_config then
                local success, configData = pcall(load_json_config, filepath)
                if success and configData and configData.launcher and configData.launcher.description then
                    description = configData.launcher.description
                end
            end

            table.insert(configSelector.availableConfigs, {
                name = configName,
                path = filepath,
                description = description
            })
        end
    end

    -- Sort configs alphabetically
    table.sort(configSelector.availableConfigs, function(a, b)
        return a.name < b.name
    end)

    log_trace("Found %d config files", #configSelector.availableConfigs)
end

-- Handle input
local function handleInput()
    if not input then return end

    -- Keyboard navigation
    if input.keyPressed then
        if input.keyPressed == "up" or input.keyPressed == "w" then
            configSelector.selectedIndex = math.max(1, configSelector.selectedIndex - 1)
        elseif input.keyPressed == "down" or input.keyPressed == "s" then
            configSelector.selectedIndex = math.min(#configSelector.availableConfigs, configSelector.selectedIndex + 1)
        elseif input.keyPressed == "return" or input.keyPressed == "enter" or input.keyPressed == "space" then
            -- Select config
            if #configSelector.availableConfigs > 0 then
                configSelector.selectedConfig = configSelector.availableConfigs[configSelector.selectedIndex]
                configSelector.confirmed = true
                log_trace("Selected config: %s", configSelector.selectedConfig.path)
            end
        elseif input.keyPressed == "escape" then
            -- Cancel selection
            configSelector.selectedConfig = nil
            configSelector.confirmed = true
            log_trace("Config selection cancelled")
        end
    end

    -- Mouse/touch input
    if input.mousePressed and input.mouseX and input.mouseY then
        -- Check if click is within list bounds (simplified)
        local listY = 120
        local itemHeight = 60
        local listHeight = #configSelector.availableConfigs * itemHeight

        if input.mouseY >= listY and input.mouseY <= listY + listHeight then
            local clickedIndex = math.floor((input.mouseY - listY) / itemHeight) + 1
            if clickedIndex >= 1 and clickedIndex <= #configSelector.availableConfigs then
                configSelector.selectedIndex = clickedIndex
                -- Double-click or single click selection
                if input.mouseDoubleClick then
                    configSelector.selectedConfig = configSelector.availableConfigs[configSelector.selectedIndex]
                    configSelector.confirmed = true
                    log_trace("Selected config via mouse: %s", configSelector.selectedConfig.path)
                end
            end
        end
    end
end

-- Render the config selector GUI
local function render()
    if not ctx then return end

    -- Clear background
    Gui.drawRect(ctx, 0, 0, configSelector.windowWidth, configSelector.windowHeight, {0.05, 0.05, 0.05, 1.0})

    -- Title
    Gui.drawText(ctx, configSelector.title, 20, 20, 32, {1.0, 1.0, 1.0, 1.0})

    -- Description
    Gui.drawText(ctx, configSelector.description, 20, 60, 16, {0.8, 0.8, 0.8, 1.0})

    -- Config list
    local listX = 20
    local listY = 120
    local itemWidth = configSelector.windowWidth - 40
    local itemHeight = 60
    local spacing = 4

    for i, config in ipairs(configSelector.availableConfigs) do
        local itemY = listY + (i - 1) * (itemHeight + spacing)
        local isSelected = (i == configSelector.selectedIndex)

        -- Background
        local bgColor = isSelected and {0.2, 0.4, 0.6, 1.0} or {0.1, 0.1, 0.1, 0.8}
        Gui.drawRect(ctx, listX, itemY, itemWidth, itemHeight, bgColor)

        -- Border for selected item
        if isSelected then
            Gui.drawRectOutline(ctx, listX, itemY, itemWidth, itemHeight, {0.5, 0.7, 0.9, 1.0}, 2)
        end

        -- Config name
        Gui.drawText(ctx, config.name, listX + 10, itemY + 8, 18, {1.0, 1.0, 1.0, 1.0})

        -- Config path
        Gui.drawText(ctx, config.path, listX + 10, itemY + 30, 12, {0.7, 0.7, 0.7, 1.0})

        -- Description
        if config.description then
            Gui.drawText(ctx, config.description, listX + 10, itemY + 42, 11, {0.6, 0.6, 0.6, 1.0})
        end
    end

    -- Status message
    Gui.drawText(ctx, configSelector.statusMessage, 20, configSelector.windowHeight - 40, 14, {0.8, 0.8, 0.8, 1.0})

    -- Instructions
    local instructions = "↑/↓ or W/S: Navigate  |  Enter/Space: Select  |  Escape: Cancel"
    Gui.drawText(ctx, instructions, 20, configSelector.windowHeight - 20, 12, {0.6, 0.6, 0.6, 1.0})
end

-- Main update function
local function update()
    handleInput()
    render()
end

-- Initialize
scanConfigFiles()

-- Export functions
return {
    update = update,
    isFinished = function() return configSelector.confirmed end,
    getSelectedConfig = function() return configSelector.selectedConfig end,
    setWindowSize = function(width, height)
        configSelector.windowWidth = width
        configSelector.windowHeight = height
    end,
    setVitaMode = function()
        configSelector.windowWidth = 960
        configSelector.windowHeight = 544
        configSelector.title = "Select Configuration (Vita)"
        configSelector.statusMessage = "Use D-pad to navigate, X to select, O to cancel"
    end
}