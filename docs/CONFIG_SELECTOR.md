# Config Selector Integration Guide

## Overview
The config selector provides a Lua-based GUI for selecting JSON configuration files at runtime. It automatically scans the `config/` directory for `.json` files and presents them in a navigable list.

## Files Created
- `scripts/config_selector.lua` - Main config selector implementation
- `scripts/config_selector_demo.lua` - Demo showing usage patterns

## Usage in C++ Application

### 1. Include in Build System
Add the config selector to your CMakeLists.txt or build system to ensure it's packaged with the application.

### 2. Integration Points

#### Startup Flow
```cpp
// In main.cpp or app initialization
#ifdef ENABLE_VITA
    // Vita: Always run config selector by default (no command line available)
    runConfigSelector();
#else
    // Desktop: Check command line arguments
    if (command_line_has("--select-config")) {
        runConfigSelector();
    } else if (command_line_has("--config")) {
        // Load specific config
        std::string configPath = getCommandLineArg("--config");
        loadConfig(configPath);
    } else {
        // Default behavior - load default config
        loadConfig("config/default.json");
    }
#endif
```

#### Config Selector Function
```cpp
void runConfigSelector() {
    // Initialize Lua state
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    // Load GUI bindings
    bindGuiFunctions(L);

    // Load config selector script
    if (luaL_dofile(L, "scripts/config_selector.lua") != LUA_OK) {
        std::cerr << "Failed to load config selector: " << lua_tostring(L, -1) << std::endl;
        return;
    }

    // Set Vita mode if building for Vita
    #ifdef ENABLE_VITA
    lua_getglobal(L, "setVitaMode");
    if (lua_isfunction(L, -1)) {
        lua_call(L, 0, 0);
    }
    #endif

    // Main selector loop
    bool finished = false;
    while (!finished && !windowShouldClose()) {
        // Update input state
        updateLuaInputState(L);

        // Call selector update
        lua_getglobal(L, "update");
        lua_call(L, 0, 0);

        // Check if finished
        lua_getglobal(L, "isFinished");
        lua_call(L, 0, 1);
        finished = lua_toboolean(L, -1);
        lua_pop(L, 1);

        // Render
        render();

        // Small delay to prevent busy loop
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Get selected config
    if (finished) {
        lua_getglobal(L, "getSelectedConfig");
        lua_call(L, 0, 1);

        if (lua_istable(L, -1)) {
            // Extract config path
            lua_getfield(L, -1, "path");
            const char* configPath = lua_tostring(L, -1);

            if (configPath) {
                std::cout << "Selected config: " << configPath << std::endl;
                // Restart application with selected config
                restartApplication(configPath);
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }

    lua_close(L);
}
```

#### Lua Bindings Required
The config selector expects these Lua bindings from C++:

```cpp
// File system functions
lua_register(L, "file_exists", lua_file_exists);
lua_register(L, "list_directory", lua_list_directory);

// JSON loading
lua_register(L, "load_json_config", lua_load_json_config);

// GUI functions (from existing gui.lua integration)
lua_register(L, "newContext", lua_gui_new_context);
lua_register(L, "newInputState", lua_gui_new_input_state);
lua_register(L, "drawRect", lua_gui_draw_rect);
lua_register(L, "drawRectOutline", lua_gui_draw_rect_outline);
lua_register(L, "drawText", lua_gui_draw_text);

// Logging
lua_register(L, "log_trace", lua_log_trace);
```

### 3. Command Line Options (Desktop Only)
On desktop platforms, add support for these command line options:

```bash
# Run config selector
./myapp --select-config

# Load specific config
./myapp --config config/vita_gui_runtime.json

# Default behavior (no args)
./myapp  # loads default config
```

**Note**: Vita builds do not support command line options since the Vita has no command line interface. The config selector runs automatically on Vita.

### 4. Vita-Specific Considerations
- **Default Behavior**: Config selector runs automatically on Vita (no command line available)
- Window size should be set to Vita resolution: 960x544
- Touch input is supported via mouse simulation
- Config files are automatically filtered to show both desktop and Vita variants

### 5. Error Handling
```cpp
try {
    runConfigSelector();
} catch (const std::exception& e) {
    std::cerr << "Config selector failed: " << e.what() << std::endl;
    // Fall back to default config
    loadConfig("config/default.json");
}
```

## Config File Format
The selector expects JSON files with optional launcher metadata:

```json
{
    "launcher": {
        "description": "Human-readable description of this config"
    },
    "app": {
        // Application-specific settings
    }
}
```

## Testing
Run the demo script to verify functionality:
```bash
lua scripts/config_selector_demo.lua
```

## Future Enhancements
- Add search/filter functionality
- Support for config categories/groups
- Preview screenshots for configs
- Keyboard shortcuts for quick selection