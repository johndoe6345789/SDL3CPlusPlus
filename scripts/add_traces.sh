#!/bin/bash

# Script to add logging includes to files that need them

FILES=(
    "src/script/lua_helpers.cpp"
    "src/script/mesh_loader.cpp"
    "src/app/vulkan_api.cpp"
    "src/script/physics_bridge.cpp"
    "src/script/scene_manager.cpp"
    "src/script/shader_manager.cpp"
    "src/script/audio_manager.cpp"
    "src/script/gui_manager.cpp"
    "src/gui/gui_renderer.cpp"
    "src/script/lua_bindings.cpp"
)

for file in "${FILES[@]}"; do
    # Check if file already includes logger.hpp
    if ! grep -q "#include.*logging/logger.hpp" "$file"; then
        echo "Adding logger include to $file"
        # Add include after the first #include line
        sed -i '0,/#include/s|#include.*|&\n#include "logging/logger.hpp"|' "$file"
    else
        echo "Skipping $file - already includes logger"
    fi
done

echo "Done adding logger includes"
