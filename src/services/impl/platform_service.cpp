#include "platform_service.hpp"

#include <cstdlib>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace sdl3cpp::services::impl {

PlatformService::PlatformService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("PlatformService", "PlatformService");
    }
}

std::optional<std::filesystem::path> PlatformService::GetUserConfigDirectory() const {
    if (logger_) {
        logger_->Trace("PlatformService", "GetUserConfigDirectory");
    }
#ifdef _WIN32
    if (const char* appData = std::getenv("APPDATA")) {
        return std::filesystem::path(appData) / "sdl3cpp";
    }
#else
    if (const char* xdgConfig = std::getenv("XDG_CONFIG_HOME")) {
        return std::filesystem::path(xdgConfig) / "sdl3cpp";
    }
    if (const char* home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".config" / "sdl3cpp";
    }
#endif
    return std::nullopt;
}

#ifdef _WIN32
std::string PlatformService::FormatWin32Error(unsigned long errorCode) const {
    if (logger_) {
        logger_->Trace("PlatformService", "FormatWin32Error",
                       "errorCode=" + std::to_string(errorCode));
    }
    if (errorCode == ERROR_SUCCESS) {
        return "ERROR_SUCCESS";
    }
    LPSTR buffer = nullptr;
    DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr
    );

    std::string message;
    if (length > 0 && buffer != nullptr) {
        message = std::string(buffer, length);
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }
        LocalFree(buffer);
    } else {
        message = "(FormatMessage failed)";
    }
    return message;
}
#endif

std::string PlatformService::GetPlatformError() const {
    if (logger_) {
        logger_->Trace("PlatformService", "GetPlatformError");
    }
#ifdef _WIN32
    DWORD win32Error = ::GetLastError();
    if (win32Error != ERROR_SUCCESS) {
        return "Win32 error " + std::to_string(win32Error) + ": " + FormatWin32Error(win32Error);
    }
    return "No platform error";
#else
    return "No platform error";
#endif
}

}  // namespace sdl3cpp::services::impl
