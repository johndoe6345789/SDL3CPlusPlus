#include "platform_service.hpp"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(__linux__)
#include <sys/utsname.h>
#endif

namespace sdl3cpp::services::impl {
namespace {

std::string EnvValueOrUnset(const char* name) {
    const char* value = std::getenv(name);
    if (!value || *value == '\0') {
        return "unset";
    }
    return value;
}

std::string HintValueOrUnset(const char* name) {
    const char* value = SDL_GetHint(name);
    if (!value || *value == '\0') {
        return "unset";
    }
    return value;
}

std::string SystemThemeName(SDL_SystemTheme theme) {
    switch (theme) {
        case SDL_SYSTEM_THEME_LIGHT:
            return "light";
        case SDL_SYSTEM_THEME_DARK:
            return "dark";
        case SDL_SYSTEM_THEME_UNKNOWN:
        default:
            return "unknown";
    }
}

std::string JoinStrings(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "none";
    }
    std::string result;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += values[i];
    }
    return result;
}

bool ContainsDriver(const std::vector<std::string>& drivers, const char* name) {
    if (!name || *name == '\0') {
        return false;
    }
    for (const auto& driver : drivers) {
        if (driver == name) {
            return true;
        }
    }
    return false;
}

std::string FormatBounds(const SDL_Rect& bounds) {
    return std::to_string(bounds.w) + "x" + std::to_string(bounds.h) +
           "+" + std::to_string(bounds.x) + "+" + std::to_string(bounds.y);
}

enum class FeatureKey {
    PointerBits,
    PlatformName,
    SdlVersion,
    SdlVersionMajor,
    SdlVersionMinor,
    SdlVersionMicro,
    SdlRevision,
    CpuCount,
    CpuCacheLineSize,
    SystemRamMb,
    CpuHasSse,
    CpuHasSse2,
    CpuHasSse3,
    CpuHasSse41,
    CpuHasSse42,
    CpuHasAvx,
    CpuHasAvx2,
    CpuHasAvx512f,
    CpuHasNeon,
    CpuHasArmSimd,
    CpuHasAltivec,
    CpuHasLsx,
    CpuHasLasx,
    EnvXdgSessionType,
    EnvWaylandDisplay,
    EnvX11Display,
    EnvDesktopSession,
    EnvXdgCurrentDesktop,
    EnvXdgRuntimeDir,
    EnvSdlVideoDriver,
    EnvSdlRenderDriver,
    HintVideoDriver,
    HintRenderDriver,
    HintWaylandPreferLibdecor,
    SdlVideoDriverCount,
    SdlVideoDrivers,
    SdlVideoInitialized,
    SdlVideoBackendSupportsWayland,
    SdlVideoBackendSupportsX11,
    SdlVideoBackendSupportsKmsdrm,
    SdlVideoBackendSupportsWindows,
    SdlVideoBackendSupportsCocoa,
    SdlVideoBackendIsWayland,
    SdlVideoBackendIsX11,
    SdlVideoBackendIsKmsdrm,
    SdlVideoBackendIsWindows,
    SdlVideoBackendIsCocoa,
    SdlCurrentVideoDriver,
    SdlSystemTheme,
    SdlRenderDriverCount,
    SdlRenderDrivers,
    SdlRenderSupportsOpenGL,
    SdlRenderSupportsOpenGLES2,
    SdlRenderSupportsDirect3D11,
    SdlRenderSupportsDirect3D12,
    SdlRenderSupportsMetal,
    SdlRenderSupportsSoftware,
    SdlDisplayCount,
    SdlPrimaryDisplayId,
    SdlDisplaySummary,
    SdlDisplayError,
    UnameSysname,
    UnameRelease,
    UnameVersion,
    UnameMachine
};

struct FeatureDefinition {
    FeatureKey key;
    const char* name;
};

struct FeatureValues {
    int pointerBits = 0;
    std::string platformName;
    int sdlVersion = 0;
    int sdlVersionMajor = 0;
    int sdlVersionMinor = 0;
    int sdlVersionMicro = 0;
    std::string sdlRevision;
    int cpuCount = 0;
    int cpuCacheLineSize = 0;
    int systemRamMb = 0;
    bool cpuHasSse = false;
    bool cpuHasSse2 = false;
    bool cpuHasSse3 = false;
    bool cpuHasSse41 = false;
    bool cpuHasSse42 = false;
    bool cpuHasAvx = false;
    bool cpuHasAvx2 = false;
    bool cpuHasAvx512f = false;
    bool cpuHasNeon = false;
    bool cpuHasArmSimd = false;
    bool cpuHasAltivec = false;
    bool cpuHasLsx = false;
    bool cpuHasLasx = false;
    std::string envXdgSessionType;
    std::string envWaylandDisplay;
    std::string envX11Display;
    std::string envDesktopSession;
    std::string envXdgCurrentDesktop;
    std::string envXdgRuntimeDir;
    std::string envSdlVideoDriver;
    std::string envSdlRenderDriver;
    std::string hintVideoDriver;
    std::string hintRenderDriver;
    std::string hintWaylandPreferLibdecor;
    int sdlVideoDriverCount = 0;
    std::string sdlVideoDrivers;
    bool sdlVideoInitialized = false;
    bool sdlVideoBackendSupportsWayland = false;
    bool sdlVideoBackendSupportsX11 = false;
    bool sdlVideoBackendSupportsKmsdrm = false;
    bool sdlVideoBackendSupportsWindows = false;
    bool sdlVideoBackendSupportsCocoa = false;
    bool sdlVideoBackendIsWayland = false;
    bool sdlVideoBackendIsX11 = false;
    bool sdlVideoBackendIsKmsdrm = false;
    bool sdlVideoBackendIsWindows = false;
    bool sdlVideoBackendIsCocoa = false;
    std::string sdlCurrentVideoDriver;
    std::string sdlSystemTheme;
    int sdlRenderDriverCount = 0;
    std::string sdlRenderDrivers;
    bool sdlRenderSupportsOpenGL = false;
    bool sdlRenderSupportsOpenGLES2 = false;
    bool sdlRenderSupportsDirect3D11 = false;
    bool sdlRenderSupportsDirect3D12 = false;
    bool sdlRenderSupportsMetal = false;
    bool sdlRenderSupportsSoftware = false;
    int sdlDisplayCount = 0;
    int sdlPrimaryDisplayId = 0;
    std::string sdlDisplaySummary = "none";
    std::string sdlDisplayError = "none";
    std::string unameSysname;
    std::string unameRelease;
    std::string unameVersion;
    std::string unameMachine;
};

std::string BoolToString(bool value) {
    return value ? "true" : "false";
}

std::string StringOrUnset(const std::string& value) {
    if (value.empty()) {
        return "unset";
    }
    return value;
}

std::string FeatureValue(FeatureKey key, const FeatureValues& values) {
    switch (key) {
        case FeatureKey::PointerBits:
            return std::to_string(values.pointerBits);
        case FeatureKey::PlatformName:
            return StringOrUnset(values.platformName);
        case FeatureKey::SdlVersion:
            return std::to_string(values.sdlVersion);
        case FeatureKey::SdlVersionMajor:
            return std::to_string(values.sdlVersionMajor);
        case FeatureKey::SdlVersionMinor:
            return std::to_string(values.sdlVersionMinor);
        case FeatureKey::SdlVersionMicro:
            return std::to_string(values.sdlVersionMicro);
        case FeatureKey::SdlRevision:
            return StringOrUnset(values.sdlRevision);
        case FeatureKey::CpuCount:
            return std::to_string(values.cpuCount);
        case FeatureKey::CpuCacheLineSize:
            return std::to_string(values.cpuCacheLineSize);
        case FeatureKey::SystemRamMb:
            return std::to_string(values.systemRamMb);
        case FeatureKey::CpuHasSse:
            return BoolToString(values.cpuHasSse);
        case FeatureKey::CpuHasSse2:
            return BoolToString(values.cpuHasSse2);
        case FeatureKey::CpuHasSse3:
            return BoolToString(values.cpuHasSse3);
        case FeatureKey::CpuHasSse41:
            return BoolToString(values.cpuHasSse41);
        case FeatureKey::CpuHasSse42:
            return BoolToString(values.cpuHasSse42);
        case FeatureKey::CpuHasAvx:
            return BoolToString(values.cpuHasAvx);
        case FeatureKey::CpuHasAvx2:
            return BoolToString(values.cpuHasAvx2);
        case FeatureKey::CpuHasAvx512f:
            return BoolToString(values.cpuHasAvx512f);
        case FeatureKey::CpuHasNeon:
            return BoolToString(values.cpuHasNeon);
        case FeatureKey::CpuHasArmSimd:
            return BoolToString(values.cpuHasArmSimd);
        case FeatureKey::CpuHasAltivec:
            return BoolToString(values.cpuHasAltivec);
        case FeatureKey::CpuHasLsx:
            return BoolToString(values.cpuHasLsx);
        case FeatureKey::CpuHasLasx:
            return BoolToString(values.cpuHasLasx);
        case FeatureKey::EnvXdgSessionType:
            return StringOrUnset(values.envXdgSessionType);
        case FeatureKey::EnvWaylandDisplay:
            return StringOrUnset(values.envWaylandDisplay);
        case FeatureKey::EnvX11Display:
            return StringOrUnset(values.envX11Display);
        case FeatureKey::EnvDesktopSession:
            return StringOrUnset(values.envDesktopSession);
        case FeatureKey::EnvXdgCurrentDesktop:
            return StringOrUnset(values.envXdgCurrentDesktop);
        case FeatureKey::EnvXdgRuntimeDir:
            return StringOrUnset(values.envXdgRuntimeDir);
        case FeatureKey::EnvSdlVideoDriver:
            return StringOrUnset(values.envSdlVideoDriver);
        case FeatureKey::EnvSdlRenderDriver:
            return StringOrUnset(values.envSdlRenderDriver);
        case FeatureKey::HintVideoDriver:
            return StringOrUnset(values.hintVideoDriver);
        case FeatureKey::HintRenderDriver:
            return StringOrUnset(values.hintRenderDriver);
        case FeatureKey::HintWaylandPreferLibdecor:
            return StringOrUnset(values.hintWaylandPreferLibdecor);
        case FeatureKey::SdlVideoDriverCount:
            return std::to_string(values.sdlVideoDriverCount);
        case FeatureKey::SdlVideoDrivers:
            return StringOrUnset(values.sdlVideoDrivers);
        case FeatureKey::SdlVideoInitialized:
            return BoolToString(values.sdlVideoInitialized);
        case FeatureKey::SdlVideoBackendSupportsWayland:
            return BoolToString(values.sdlVideoBackendSupportsWayland);
        case FeatureKey::SdlVideoBackendSupportsX11:
            return BoolToString(values.sdlVideoBackendSupportsX11);
        case FeatureKey::SdlVideoBackendSupportsKmsdrm:
            return BoolToString(values.sdlVideoBackendSupportsKmsdrm);
        case FeatureKey::SdlVideoBackendSupportsWindows:
            return BoolToString(values.sdlVideoBackendSupportsWindows);
        case FeatureKey::SdlVideoBackendSupportsCocoa:
            return BoolToString(values.sdlVideoBackendSupportsCocoa);
        case FeatureKey::SdlVideoBackendIsWayland:
            return BoolToString(values.sdlVideoBackendIsWayland);
        case FeatureKey::SdlVideoBackendIsX11:
            return BoolToString(values.sdlVideoBackendIsX11);
        case FeatureKey::SdlVideoBackendIsKmsdrm:
            return BoolToString(values.sdlVideoBackendIsKmsdrm);
        case FeatureKey::SdlVideoBackendIsWindows:
            return BoolToString(values.sdlVideoBackendIsWindows);
        case FeatureKey::SdlVideoBackendIsCocoa:
            return BoolToString(values.sdlVideoBackendIsCocoa);
        case FeatureKey::SdlCurrentVideoDriver:
            return StringOrUnset(values.sdlCurrentVideoDriver);
        case FeatureKey::SdlSystemTheme:
            return StringOrUnset(values.sdlSystemTheme);
        case FeatureKey::SdlRenderDriverCount:
            return std::to_string(values.sdlRenderDriverCount);
        case FeatureKey::SdlRenderDrivers:
            return StringOrUnset(values.sdlRenderDrivers);
        case FeatureKey::SdlRenderSupportsOpenGL:
            return BoolToString(values.sdlRenderSupportsOpenGL);
        case FeatureKey::SdlRenderSupportsOpenGLES2:
            return BoolToString(values.sdlRenderSupportsOpenGLES2);
        case FeatureKey::SdlRenderSupportsDirect3D11:
            return BoolToString(values.sdlRenderSupportsDirect3D11);
        case FeatureKey::SdlRenderSupportsDirect3D12:
            return BoolToString(values.sdlRenderSupportsDirect3D12);
        case FeatureKey::SdlRenderSupportsMetal:
            return BoolToString(values.sdlRenderSupportsMetal);
        case FeatureKey::SdlRenderSupportsSoftware:
            return BoolToString(values.sdlRenderSupportsSoftware);
        case FeatureKey::SdlDisplayCount:
            return std::to_string(values.sdlDisplayCount);
        case FeatureKey::SdlPrimaryDisplayId:
            return std::to_string(values.sdlPrimaryDisplayId);
        case FeatureKey::SdlDisplaySummary:
            return StringOrUnset(values.sdlDisplaySummary);
        case FeatureKey::SdlDisplayError:
            return StringOrUnset(values.sdlDisplayError);
        case FeatureKey::UnameSysname:
            return StringOrUnset(values.unameSysname);
        case FeatureKey::UnameRelease:
            return StringOrUnset(values.unameRelease);
        case FeatureKey::UnameVersion:
            return StringOrUnset(values.unameVersion);
        case FeatureKey::UnameMachine:
            return StringOrUnset(values.unameMachine);
        default:
            return "unset";
    }
}

std::string BuildFeatureTable(const FeatureValues& values) {
    static constexpr FeatureDefinition kFeatures[] = {
        {FeatureKey::PointerBits, "platform.pointerBits"},
        {FeatureKey::PlatformName, "platform.name"},
        {FeatureKey::SdlVersion, "platform.sdl.version"},
        {FeatureKey::SdlVersionMajor, "platform.sdl.version.major"},
        {FeatureKey::SdlVersionMinor, "platform.sdl.version.minor"},
        {FeatureKey::SdlVersionMicro, "platform.sdl.version.micro"},
        {FeatureKey::SdlRevision, "platform.sdl.revision"},
        {FeatureKey::CpuCount, "platform.cpu.count"},
        {FeatureKey::CpuCacheLineSize, "platform.cpu.cacheLineSize"},
        {FeatureKey::SystemRamMb, "platform.systemRamMB"},
        {FeatureKey::CpuHasSse, "platform.cpu.hasSSE"},
        {FeatureKey::CpuHasSse2, "platform.cpu.hasSSE2"},
        {FeatureKey::CpuHasSse3, "platform.cpu.hasSSE3"},
        {FeatureKey::CpuHasSse41, "platform.cpu.hasSSE41"},
        {FeatureKey::CpuHasSse42, "platform.cpu.hasSSE42"},
        {FeatureKey::CpuHasAvx, "platform.cpu.hasAVX"},
        {FeatureKey::CpuHasAvx2, "platform.cpu.hasAVX2"},
        {FeatureKey::CpuHasAvx512f, "platform.cpu.hasAVX512F"},
        {FeatureKey::CpuHasNeon, "platform.cpu.hasNEON"},
        {FeatureKey::CpuHasArmSimd, "platform.cpu.hasARMSIMD"},
        {FeatureKey::CpuHasAltivec, "platform.cpu.hasAltiVec"},
        {FeatureKey::CpuHasLsx, "platform.cpu.hasLSX"},
        {FeatureKey::CpuHasLasx, "platform.cpu.hasLASX"},
        {FeatureKey::EnvXdgSessionType, "env.xdgSessionType"},
        {FeatureKey::EnvWaylandDisplay, "env.waylandDisplay"},
        {FeatureKey::EnvX11Display, "env.x11Display"},
        {FeatureKey::EnvDesktopSession, "env.desktopSession"},
        {FeatureKey::EnvXdgCurrentDesktop, "env.xdgCurrentDesktop"},
        {FeatureKey::EnvXdgRuntimeDir, "env.xdgRuntimeDir"},
        {FeatureKey::EnvSdlVideoDriver, "env.sdlVideoDriver"},
        {FeatureKey::EnvSdlRenderDriver, "env.sdlRenderDriver"},
        {FeatureKey::HintVideoDriver, "sdl.hint.videoDriver"},
        {FeatureKey::HintRenderDriver, "sdl.hint.renderDriver"},
        {FeatureKey::HintWaylandPreferLibdecor, "sdl.hint.waylandPreferLibdecor"},
        {FeatureKey::SdlVideoDriverCount, "sdl.videoDriverCount"},
        {FeatureKey::SdlVideoDrivers, "sdl.videoDrivers"},
        {FeatureKey::SdlVideoInitialized, "sdl.videoInitialized"},
        {FeatureKey::SdlVideoBackendSupportsWayland, "sdl.videoBackend.supportsWayland"},
        {FeatureKey::SdlVideoBackendSupportsX11, "sdl.videoBackend.supportsX11"},
        {FeatureKey::SdlVideoBackendSupportsKmsdrm, "sdl.videoBackend.supportsKmsdrm"},
        {FeatureKey::SdlVideoBackendSupportsWindows, "sdl.videoBackend.supportsWindows"},
        {FeatureKey::SdlVideoBackendSupportsCocoa, "sdl.videoBackend.supportsCocoa"},
        {FeatureKey::SdlVideoBackendIsWayland, "sdl.videoBackend.isWayland"},
        {FeatureKey::SdlVideoBackendIsX11, "sdl.videoBackend.isX11"},
        {FeatureKey::SdlVideoBackendIsKmsdrm, "sdl.videoBackend.isKmsdrm"},
        {FeatureKey::SdlVideoBackendIsWindows, "sdl.videoBackend.isWindows"},
        {FeatureKey::SdlVideoBackendIsCocoa, "sdl.videoBackend.isCocoa"},
        {FeatureKey::SdlCurrentVideoDriver, "sdl.currentVideoDriver"},
        {FeatureKey::SdlSystemTheme, "sdl.systemTheme"},
        {FeatureKey::SdlRenderDriverCount, "sdl.renderDriverCount"},
        {FeatureKey::SdlRenderDrivers, "sdl.renderDrivers"},
        {FeatureKey::SdlRenderSupportsOpenGL, "sdl.render.supportsOpenGL"},
        {FeatureKey::SdlRenderSupportsOpenGLES2, "sdl.render.supportsOpenGLES2"},
        {FeatureKey::SdlRenderSupportsDirect3D11, "sdl.render.supportsDirect3D11"},
        {FeatureKey::SdlRenderSupportsDirect3D12, "sdl.render.supportsDirect3D12"},
        {FeatureKey::SdlRenderSupportsMetal, "sdl.render.supportsMetal"},
        {FeatureKey::SdlRenderSupportsSoftware, "sdl.render.supportsSoftware"},
        {FeatureKey::SdlDisplayCount, "sdl.displayCount"},
        {FeatureKey::SdlPrimaryDisplayId, "sdl.primaryDisplayId"},
        {FeatureKey::SdlDisplaySummary, "sdl.displaySummary"},
        {FeatureKey::SdlDisplayError, "sdl.displayError"},
        {FeatureKey::UnameSysname, "platform.uname.sysname"},
        {FeatureKey::UnameRelease, "platform.uname.release"},
        {FeatureKey::UnameVersion, "platform.uname.version"},
        {FeatureKey::UnameMachine, "platform.uname.machine"}
    };

    std::string table = "feature\tvalue\n";
    for (const auto& feature : kFeatures) {
        table += feature.name;
        table += "\t";
        table += FeatureValue(feature.key, values);
        table += "\n";
    }
    return table;
}

}  // namespace

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

std::string PlatformService::GetPlatformName() const {
    if (logger_) {
        logger_->Trace("PlatformService", "GetPlatformName");
    }
    const char* platformName = SDL_GetPlatform();
    if (platformName && *platformName != '\0') {
        return platformName;
    }
    return "unknown";
}

std::string PlatformService::GetCurrentVideoDriverName() const {
    if (logger_) {
        logger_->Trace("PlatformService", "GetCurrentVideoDriverName");
    }
    const char* driver = SDL_GetCurrentVideoDriver();
    if (driver && *driver != '\0') {
        return driver;
    }
    return "unknown";
}

std::vector<std::string> PlatformService::GetAvailableVideoDrivers() const {
    if (logger_) {
        logger_->Trace("PlatformService", "GetAvailableVideoDrivers");
    }
    const int count = SDL_GetNumVideoDrivers();
    std::vector<std::string> drivers;
    if (count <= 0) {
        return drivers;
    }
    drivers.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const char* driver = SDL_GetVideoDriver(i);
        if (driver && *driver != '\0') {
            drivers.emplace_back(driver);
        }
    }
    return drivers;
}

std::vector<std::string> PlatformService::GetAvailableRenderDrivers() const {
    if (logger_) {
        logger_->Trace("PlatformService", "GetAvailableRenderDrivers");
    }
    const int count = SDL_GetNumRenderDrivers();
    std::vector<std::string> drivers;
    if (count <= 0) {
        return drivers;
    }
    drivers.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        const char* driver = SDL_GetRenderDriver(i);
        if (driver && *driver != '\0') {
            drivers.emplace_back(driver);
        }
    }
    return drivers;
}

void PlatformService::LogSystemInfo() const {
    if (!logger_) {
        return;
    }

    logger_->Trace("PlatformService", "LogSystemInfo");
    FeatureValues values;
    const int pointerBits = static_cast<int>(sizeof(void*) * 8);
    logger_->TraceVariable("platform.pointerBits", pointerBits);
    values.pointerBits = pointerBits;

    const std::string platformName = GetPlatformName();
    values.platformName = platformName;
    logger_->TraceVariable("platform.name", values.platformName);

    const int sdlVersion = SDL_GetVersion();
    logger_->TraceVariable("platform.sdl.version", sdlVersion);
    values.sdlVersion = sdlVersion;
    values.sdlVersionMajor = SDL_VERSIONNUM_MAJOR(sdlVersion);
    values.sdlVersionMinor = SDL_VERSIONNUM_MINOR(sdlVersion);
    values.sdlVersionMicro = SDL_VERSIONNUM_MICRO(sdlVersion);

    const char* revision = SDL_GetRevision();
    if (revision && *revision != '\0') {
        logger_->TraceVariable("platform.sdl.revision", std::string(revision));
        values.sdlRevision = revision;
    }

    logger_->Trace("PlatformService", "Querying CPU count with SDL_GetNumLogicalCPUCores");
    const int cpuCount = SDL_GetNumLogicalCPUCores();
    const int cpuCacheLineSize = SDL_GetCPUCacheLineSize();
    const int systemRamMb = SDL_GetSystemRAM();
    logger_->TraceVariable("platform.cpu.count", cpuCount);
    logger_->TraceVariable("platform.cpu.cacheLineSize", cpuCacheLineSize);
    logger_->TraceVariable("platform.systemRamMB", systemRamMb);
    values.cpuCount = cpuCount;
    values.cpuCacheLineSize = cpuCacheLineSize;
    values.systemRamMb = systemRamMb;

    const bool hasSse = SDL_HasSSE();
    const bool hasSse2 = SDL_HasSSE2();
    const bool hasSse3 = SDL_HasSSE3();
    const bool hasSse41 = SDL_HasSSE41();
    const bool hasSse42 = SDL_HasSSE42();
    const bool hasAvx = SDL_HasAVX();
    const bool hasAvx2 = SDL_HasAVX2();
    const bool hasAvx512f = SDL_HasAVX512F();
    const bool hasNeon = SDL_HasNEON();
    const bool hasArmSimd = SDL_HasARMSIMD();
    const bool hasAltivec = SDL_HasAltiVec();
    const bool hasLsx = SDL_HasLSX();
    const bool hasLasx = SDL_HasLASX();
    logger_->TraceVariable("platform.cpu.hasSSE", hasSse);
    logger_->TraceVariable("platform.cpu.hasSSE2", hasSse2);
    logger_->TraceVariable("platform.cpu.hasSSE3", hasSse3);
    logger_->TraceVariable("platform.cpu.hasSSE41", hasSse41);
    logger_->TraceVariable("platform.cpu.hasSSE42", hasSse42);
    logger_->TraceVariable("platform.cpu.hasAVX", hasAvx);
    logger_->TraceVariable("platform.cpu.hasAVX2", hasAvx2);
    logger_->TraceVariable("platform.cpu.hasAVX512F", hasAvx512f);
    logger_->TraceVariable("platform.cpu.hasNEON", hasNeon);
    logger_->TraceVariable("platform.cpu.hasARMSIMD", hasArmSimd);
    logger_->TraceVariable("platform.cpu.hasAltiVec", hasAltivec);
    logger_->TraceVariable("platform.cpu.hasLSX", hasLsx);
    logger_->TraceVariable("platform.cpu.hasLASX", hasLasx);
    values.cpuHasSse = hasSse;
    values.cpuHasSse2 = hasSse2;
    values.cpuHasSse3 = hasSse3;
    values.cpuHasSse41 = hasSse41;
    values.cpuHasSse42 = hasSse42;
    values.cpuHasAvx = hasAvx;
    values.cpuHasAvx2 = hasAvx2;
    values.cpuHasAvx512f = hasAvx512f;
    values.cpuHasNeon = hasNeon;
    values.cpuHasArmSimd = hasArmSimd;
    values.cpuHasAltivec = hasAltivec;
    values.cpuHasLsx = hasLsx;
    values.cpuHasLasx = hasLasx;

    const std::string envSessionType = EnvValueOrUnset("XDG_SESSION_TYPE");
    const std::string envWaylandDisplay = EnvValueOrUnset("WAYLAND_DISPLAY");
    const std::string envX11Display = EnvValueOrUnset("DISPLAY");
    const std::string envDesktopSession = EnvValueOrUnset("DESKTOP_SESSION");
    const std::string envCurrentDesktop = EnvValueOrUnset("XDG_CURRENT_DESKTOP");
    const std::string envRuntimeDir = EnvValueOrUnset("XDG_RUNTIME_DIR");
    const std::string envSdlVideoDriver = EnvValueOrUnset("SDL_VIDEODRIVER");
    const std::string envSdlRenderDriver = EnvValueOrUnset("SDL_RENDER_DRIVER");
    logger_->TraceVariable("env.xdgSessionType", envSessionType);
    logger_->TraceVariable("env.waylandDisplay", envWaylandDisplay);
    logger_->TraceVariable("env.x11Display", envX11Display);
    logger_->TraceVariable("env.desktopSession", envDesktopSession);
    logger_->TraceVariable("env.xdgCurrentDesktop", envCurrentDesktop);
    logger_->TraceVariable("env.xdgRuntimeDir", envRuntimeDir);
    logger_->TraceVariable("env.sdlVideoDriver", envSdlVideoDriver);
    logger_->TraceVariable("env.sdlRenderDriver", envSdlRenderDriver);
    values.envXdgSessionType = envSessionType;
    values.envWaylandDisplay = envWaylandDisplay;
    values.envX11Display = envX11Display;
    values.envDesktopSession = envDesktopSession;
    values.envXdgCurrentDesktop = envCurrentDesktop;
    values.envXdgRuntimeDir = envRuntimeDir;
    values.envSdlVideoDriver = envSdlVideoDriver;
    values.envSdlRenderDriver = envSdlRenderDriver;

    const std::string hintVideoDriver = HintValueOrUnset(SDL_HINT_VIDEO_DRIVER);
    const std::string hintRenderDriver = HintValueOrUnset(SDL_HINT_RENDER_DRIVER);
    const std::string hintWaylandLibdecor = HintValueOrUnset(SDL_HINT_VIDEO_WAYLAND_PREFER_LIBDECOR);
    logger_->TraceVariable("sdl.hint.videoDriver", hintVideoDriver);
    logger_->TraceVariable("sdl.hint.renderDriver", hintRenderDriver);
    logger_->TraceVariable("sdl.hint.waylandPreferLibdecor", hintWaylandLibdecor);
    values.hintVideoDriver = hintVideoDriver;
    values.hintRenderDriver = hintRenderDriver;
    values.hintWaylandPreferLibdecor = hintWaylandLibdecor;

    std::vector<std::string> videoDrivers = GetAvailableVideoDrivers();
    values.sdlVideoDriverCount = static_cast<int>(videoDrivers.size());
    logger_->TraceVariable("sdl.videoDriverCount", values.sdlVideoDriverCount);
    if (!videoDrivers.empty()) {
        const std::string joinedDrivers = JoinStrings(videoDrivers);
        logger_->TraceVariable("sdl.videoDrivers", joinedDrivers);
        values.sdlVideoDrivers = joinedDrivers;
    }
    values.sdlVideoBackendSupportsWayland = ContainsDriver(videoDrivers, "wayland");
    values.sdlVideoBackendSupportsX11 = ContainsDriver(videoDrivers, "x11");
    values.sdlVideoBackendSupportsKmsdrm = ContainsDriver(videoDrivers, "kmsdrm");
    values.sdlVideoBackendSupportsWindows = ContainsDriver(videoDrivers, "windows");
    values.sdlVideoBackendSupportsCocoa = ContainsDriver(videoDrivers, "cocoa");
    logger_->TraceVariable("sdl.videoBackend.supportsWayland", values.sdlVideoBackendSupportsWayland);
    logger_->TraceVariable("sdl.videoBackend.supportsX11", values.sdlVideoBackendSupportsX11);
    logger_->TraceVariable("sdl.videoBackend.supportsKmsdrm", values.sdlVideoBackendSupportsKmsdrm);
    logger_->TraceVariable("sdl.videoBackend.supportsWindows", values.sdlVideoBackendSupportsWindows);
    logger_->TraceVariable("sdl.videoBackend.supportsCocoa", values.sdlVideoBackendSupportsCocoa);

    const bool videoInitialized = (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0;
    logger_->TraceVariable("sdl.videoInitialized", videoInitialized);
    values.sdlVideoInitialized = videoInitialized;

    const std::string currentDriverName = GetCurrentVideoDriverName();
    logger_->TraceVariable("sdl.currentVideoDriver", currentDriverName);
    values.sdlCurrentVideoDriver = currentDriverName;
    values.sdlVideoBackendIsWayland = currentDriverName == "wayland";
    values.sdlVideoBackendIsX11 = currentDriverName == "x11";
    values.sdlVideoBackendIsKmsdrm = currentDriverName == "kmsdrm";
    values.sdlVideoBackendIsWindows = currentDriverName == "windows";
    values.sdlVideoBackendIsCocoa = currentDriverName == "cocoa";
    logger_->TraceVariable("sdl.videoBackend.isWayland", values.sdlVideoBackendIsWayland);
    logger_->TraceVariable("sdl.videoBackend.isX11", values.sdlVideoBackendIsX11);
    logger_->TraceVariable("sdl.videoBackend.isKmsdrm", values.sdlVideoBackendIsKmsdrm);
    logger_->TraceVariable("sdl.videoBackend.isWindows", values.sdlVideoBackendIsWindows);
    logger_->TraceVariable("sdl.videoBackend.isCocoa", values.sdlVideoBackendIsCocoa);
    const std::string systemTheme = SystemThemeName(SDL_GetSystemTheme());
    logger_->TraceVariable("sdl.systemTheme", systemTheme);
    values.sdlSystemTheme = systemTheme;

    std::vector<std::string> renderDrivers = GetAvailableRenderDrivers();
    values.sdlRenderDriverCount = static_cast<int>(renderDrivers.size());
    logger_->TraceVariable("sdl.renderDriverCount", values.sdlRenderDriverCount);
    if (!renderDrivers.empty()) {
        const std::string joinedDrivers = JoinStrings(renderDrivers);
        logger_->TraceVariable("sdl.renderDrivers", joinedDrivers);
        values.sdlRenderDrivers = joinedDrivers;
    }
    values.sdlRenderSupportsOpenGL = ContainsDriver(renderDrivers, "opengl");
    values.sdlRenderSupportsOpenGLES2 = ContainsDriver(renderDrivers, "opengles2");
    values.sdlRenderSupportsDirect3D11 = ContainsDriver(renderDrivers, "direct3d11");
    values.sdlRenderSupportsDirect3D12 = ContainsDriver(renderDrivers, "direct3d12");
    values.sdlRenderSupportsMetal = ContainsDriver(renderDrivers, "metal");
    values.sdlRenderSupportsSoftware = ContainsDriver(renderDrivers, "software");
    logger_->TraceVariable("sdl.render.supportsOpenGL", values.sdlRenderSupportsOpenGL);
    logger_->TraceVariable("sdl.render.supportsOpenGLES2", values.sdlRenderSupportsOpenGLES2);
    logger_->TraceVariable("sdl.render.supportsDirect3D11", values.sdlRenderSupportsDirect3D11);
    logger_->TraceVariable("sdl.render.supportsDirect3D12", values.sdlRenderSupportsDirect3D12);
    logger_->TraceVariable("sdl.render.supportsMetal", values.sdlRenderSupportsMetal);
    logger_->TraceVariable("sdl.render.supportsSoftware", values.sdlRenderSupportsSoftware);

    if (!videoInitialized) {
        logger_->Info("PlatformService::FeatureTable\n" + BuildFeatureTable(values));
        return;
    }

    int displayCount = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);
    logger_->TraceVariable("sdl.displayCount", displayCount);
    values.sdlDisplayCount = displayCount;
    if (displays) {
        SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();
        logger_->TraceVariable("sdl.primaryDisplayId", static_cast<int>(primaryDisplay));
        values.sdlPrimaryDisplayId = static_cast<int>(primaryDisplay);
        std::string displaySummary;
        for (int i = 0; i < displayCount; ++i) {
            SDL_DisplayID displayId = displays[i];
            const char* name = SDL_GetDisplayName(displayId);
            SDL_Rect bounds{};
            const bool hasBounds = SDL_GetDisplayBounds(displayId, &bounds);
            const std::string index = std::to_string(i);
            logger_->TraceVariable("sdl.display." + index + ".id",
                                   static_cast<int>(displayId));
            logger_->TraceVariable("sdl.display." + index + ".isPrimary",
                                   displayId == primaryDisplay);
            logger_->TraceVariable("sdl.display." + index + ".name",
                                   name && *name != '\0' ? std::string(name) : "unknown");
            logger_->TraceVariable("sdl.display." + index + ".bounds",
                                   hasBounds ? FormatBounds(bounds) : "unavailable");
            if (i > 0) {
                displaySummary += " | ";
            }
            displaySummary += index;
            displaySummary += ":";
            displaySummary += name && *name != '\0' ? name : "unknown";
            displaySummary += "@";
            displaySummary += hasBounds ? FormatBounds(bounds) : "unavailable";
        }
        values.sdlDisplaySummary = displaySummary.empty() ? "none" : displaySummary;
        SDL_free(displays);
    } else {
        const char* error = SDL_GetError();
        const std::string displayError = error && *error != '\0' ? std::string(error) : "unknown";
        logger_->TraceVariable("sdl.displayError", displayError);
        values.sdlDisplayError = displayError;
    }

#if defined(__linux__)
    struct utsname systemInfo {};
    if (uname(&systemInfo) == 0) {
        logger_->TraceVariable("platform.uname.sysname", std::string(systemInfo.sysname));
        logger_->TraceVariable("platform.uname.release", std::string(systemInfo.release));
        logger_->TraceVariable("platform.uname.version", std::string(systemInfo.version));
        logger_->TraceVariable("platform.uname.machine", std::string(systemInfo.machine));
        values.unameSysname = systemInfo.sysname;
        values.unameRelease = systemInfo.release;
        values.unameVersion = systemInfo.version;
        values.unameMachine = systemInfo.machine;
    }
#endif

    logger_->Info("PlatformService::FeatureTable\n" + BuildFeatureTable(values));
}

}  // namespace sdl3cpp::services::impl
