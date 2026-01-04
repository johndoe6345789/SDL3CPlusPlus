#!/bin/bash
# Enforce "no macros" policy: reject #define outside whitelisted headers
# This script is run in CI to mechanically enforce the culture

set -euo pipefail

# Whitelisted headers that are allowed to contain macros
# These are primarily include guards and necessary SDK configuration
WHITELIST=(
    "src/logging/logger.hpp"
    "src/logging/string_utils.hpp"
    "src/core/platform.hpp"
    "src/core/vertex.hpp"
    "src/app/sdl3_app.hpp"
    "src/app/sdl_macros.hpp"
    "src/app/vulkan_api.hpp"
    "src/app/audio_player.hpp"
    "src/script/script_engine.hpp"
    "src/script/mesh_loader.hpp"
    "src/script/lua_helpers.hpp"
    "src/script/lua_bindings.hpp"
    "src/script/physics_bridge.hpp"
    "src/script/gui_types.hpp"
    "src/gui/gui_renderer.hpp"
)

# Function to check if a file is whitelisted
is_whitelisted() {
    local file="$1"
    for allowed in "${WHITELIST[@]}"; do
        if [[ "$file" == "$allowed" ]]; then
            return 0
        fi
    done
    return 1
}

# Find all files containing #define in the src directory
violations=()
while IFS= read -r file; do
    if ! is_whitelisted "$file"; then
        violations+=("$file")
    fi
done < <(grep -r --include='*.hpp' --include='*.cpp' --include='*.h' -l '^#define' src/ || true)

# Report violations
if [[ ${#violations[@]} -gt 0 ]]; then
    echo "❌ MACRO POLICY VIOLATION: Found #define in non-whitelisted files"
    echo ""
    echo "The following files contain #define but are not whitelisted:"
    for file in "${violations[@]}"; do
        echo "  - $file"
        # Show the actual macro definitions
        grep -n '^#define' "$file" | sed 's/^/      /'
    done
    echo ""
    echo "Policy: Macros are banned except in whitelisted headers."
    echo "See scripts/check_macros.sh for the whitelist."
    echo ""
    echo "To fix this:"
    echo "  1. Replace macros with constexpr, inline functions, or templates"
    echo "  2. If absolutely necessary, add the file to the whitelist in scripts/check_macros.sh"
    echo ""
    exit 1
fi

echo "✅ Macro policy check passed: No violations found"
exit 0
