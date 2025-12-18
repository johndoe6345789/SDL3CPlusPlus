#define SDL_MAIN_HANDLED

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "app/vulkan_cube_app.hpp"

std::filesystem::path FindScriptPath(const char* argv0) {
    std::filesystem::path executable;
    if (argv0 && *argv0 != '\0') {
        executable = std::filesystem::path(argv0);
        if (executable.is_relative()) {
            executable = std::filesystem::current_path() / executable;
        }
    } else {
        executable = std::filesystem::current_path();
    }
    executable = std::filesystem::weakly_canonical(executable);
    std::filesystem::path scriptPath = executable.parent_path() / "scripts" / "cube_logic.lua";
    if (!std::filesystem::exists(scriptPath)) {
        throw std::runtime_error("Could not find Lua script at " + scriptPath.string());
    }
    return scriptPath;
}

int main(int argc, char** argv) {
    try {
        auto scriptPath = FindScriptPath(argc > 0 ? argv[0] : nullptr);
        sdl3cpp::app::VulkanCubeApp app(scriptPath);
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
