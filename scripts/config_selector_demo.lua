-- Config Selector Demo
-- Demonstrates how to use the config selector GUI

local ConfigSelector = require('config_selector')

-- Mock input state for demo (normally provided by C++ side)
local mockInput = {
    keyPressed = nil,
    mousePressed = false,
    mouseX = 0,
    mouseY = 0,
    mouseDoubleClick = false
}

-- Mock GUI context (normally provided by C++ side)
local mockCtx = {
    -- Mock drawing functions
    drawRect = function(ctx, x, y, w, h, color) print(string.format("drawRect: %d,%d %dx%d", x, y, w, h)) end,
    drawRectOutline = function(ctx, x, y, w, h, color, thickness) print(string.format("drawRectOutline: %d,%d %dx%d", x, y, w, h)) end,
    drawText = function(ctx, text, x, y, size, color) print(string.format("drawText: '%s' at %d,%d", text, x, y)) end
}

-- Simulate keyboard input for demo
local function simulateKeyPress(key)
    mockInput.keyPressed = key
    ConfigSelector.update()
    mockInput.keyPressed = nil
end

-- Simulate mouse input for demo
local function simulateMouseClick(x, y, doubleClick)
    mockInput.mousePressed = true
    mockInput.mouseX = x
    mockInput.mouseY = y
    mockInput.mouseDoubleClick = doubleClick or false
    ConfigSelector.update()
    mockInput.mousePressed = false
    mockInput.mouseDoubleClick = false
end

print("=== Config Selector Demo ===")
print("This demo shows how the config selector works.")
print()

-- Set up mock globals (normally provided by C++ integration)
_G.Gui = {
    newContext = function() return mockCtx end,
    newInputState = function() return mockInput end,
    drawRect = mockCtx.drawRect,
    drawRectOutline = mockCtx.drawRectOutline,
    drawText = mockCtx.drawText
}

-- Mock file system functions (normally provided by C++ side)
_G.file_exists = function(path) return true end
_G.list_directory = function(dir)
    return {
        "gui_runtime.json",
        "seed_runtime.json",
        "soundboard_runtime.json",
        "vita_gui_runtime.json",
        "vita_cube_runtime.json",
        "vita_soundboard_runtime.json"
    }
end
_G.load_json_config = function(filepath)
    -- Mock config loading
    local mockConfigs = {
        ["config/gui_runtime.json"] = {launcher = {description = "Interactive GUI demonstration"}},
        ["config/seed_runtime.json"] = {launcher = {description = "3D cube room with physics"}},
        ["config/soundboard_runtime.json"] = {launcher = {description = "Audio playback demo"}},
        ["config/vita_gui_runtime.json"] = {launcher = {description = "Vita-optimized GUI demo"}},
        ["config/vita_cube_runtime.json"] = {launcher = {description = "Vita-optimized 3D demo"}},
        ["config/vita_soundboard_runtime.json"] = {launcher = {description = "Vita-optimized audio demo"}}
    }
    return mockConfigs[filepath]
end

print("Available configs:")
local configs = ConfigSelector.getSelectedConfig and {} or {}
if not ConfigSelector.getSelectedConfig then
    -- If the selector doesn't expose configs directly, just show that it initializes
    print("- Config selector initialized successfully")
end

print()
print("Demo: Navigating with keyboard...")
simulateKeyPress("down")  -- Move down
simulateKeyPress("down")  -- Move down again
simulateKeyPress("up")    -- Move up

print()
print("Demo: Selecting with Enter...")
simulateKeyPress("return")

print()
if ConfigSelector.isFinished() then
    local selected = ConfigSelector.getSelectedConfig()
    if selected then
        print(string.format("Selected config: %s (%s)", selected.name, selected.path))
        print(string.format("Description: %s", selected.description))
    else
        print("Selection cancelled")
    end
else
    print("Selection not completed")
end

print()
print("Demo: Mouse click simulation...")
-- Reset for mouse demo
ConfigSelector = require('config_selector')  -- Reload
simulateMouseClick(400, 200, false)  -- Click on second item
simulateMouseClick(400, 200, true)   -- Double-click to select

print()
if ConfigSelector.isFinished() then
    local selected = ConfigSelector.getSelectedConfig()
    if selected then
        print(string.format("Selected config via mouse: %s (%s)", selected.name, selected.path))
    else
        print("Selection cancelled")
    end
else
    print("Selection not completed")
end

print()
print("Demo: Vita mode...")
ConfigSelector = require('config_selector')  -- Reload
ConfigSelector.setVitaMode()
print("Set Vita resolution: 960x544")
print("Updated status message for Vita controls")

print()
print("=== Demo Complete ===")
print("The config selector provides:")
print("- Platform-specific resolutions (desktop: 800x600, Vita: 960x544)")
print("- Keyboard navigation (arrow keys, WASD)")
print("- Vita D-pad simulation (D-pad, X, O)")
print("- Mouse/touch selection")
print("- Automatic config file scanning")
print("- Sorted list with descriptions")
print("- Returns selected config path for loading")