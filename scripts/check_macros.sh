#!/bin/bash
# Enforce "no macros" policy: reject #define outside whitelisted files
# This script is run in CI to mechanically enforce the culture

set -euo pipefail

# Whitelisted files that are allowed to contain macros.
# These are include guards and third-party library configuration switches
# that must be defined before the library header is included.
WHITELIST=(
    "src/core/vertex.hpp"
    "src/stb_image.cpp"
    "src/services/interfaces/workflow/rendering/grid_gpu_resources.hpp"
    "src/services/impl/workflow/rendering/frame_draw_bodies_helpers.cpp"
    "src/services/impl/workflow/workflow_generic_steps/camera_fps_update_helpers.cpp"
    "src/services/impl/workflow/workflow_generic_steps/camera_matrix_builder.cpp"
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
        grep -n '^#define' "$file" | sed 's/^/      /'
    done
    echo ""
    echo "Policy: Macros are banned except in whitelisted files."
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
