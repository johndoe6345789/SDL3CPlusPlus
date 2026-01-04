#ifndef SDL3CPP_CORE_PLATFORM_HPP
#define SDL3CPP_CORE_PLATFORM_HPP

#include <filesystem>
#include <optional>
#include <string>

namespace sdl3cpp::platform {

// Platform detection
#ifdef _WIN32
    constexpr bool IsWindows = true;
    constexpr bool IsUnix = false;
#else
    constexpr bool IsWindows = false;
    constexpr bool IsUnix = true;
#endif

// Get user configuration directory (platform-specific)
std::optional<std::filesystem::path> GetUserConfigDirectory();

// Format platform-specific error messages
std::string GetPlatformError();

} // namespace sdl3cpp::platform

#endif // SDL3CPP_CORE_PLATFORM_HPP
