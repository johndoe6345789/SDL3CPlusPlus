#include "services/interfaces/workflow/stunts/data/stunts_install.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace sdl3cpp::services::impl {
namespace {

std::string Upper(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return text;
}

bool HasExtension(const std::filesystem::path& path,
                  const std::string& upperExtension) {
    return Upper(path.extension().string()) == upperExtension;
}

}  // namespace

StuntsInstall OpenStuntsInstall(const std::string& directory) {
    StuntsInstall install;
    install.directory = directory;
    std::error_code error;
    if (directory.empty() ||
        !std::filesystem::is_directory(directory, error)) {
        return install;
    }
    for (const auto& entry :
         std::filesystem::directory_iterator(directory, error)) {
        if (entry.is_regular_file(error) &&
            HasExtension(entry.path(), ".TRK")) {
            install.valid = true;
            break;
        }
    }
    return install;
}

std::string FindStuntsFile(const StuntsInstall& install,
                           const std::string& name) {
    if (install.directory.empty()) return {};
    const std::string wanted = Upper(name);
    std::error_code error;
    for (const auto& entry :
         std::filesystem::directory_iterator(install.directory, error)) {
        if (!entry.is_regular_file(error)) continue;
        if (Upper(entry.path().filename().string()) == wanted) {
            return entry.path().string();
        }
    }
    return {};
}

std::vector<std::string> ListStuntsFiles(const StuntsInstall& install,
                                         const std::string& extension) {
    std::vector<std::string> names;
    if (install.directory.empty()) return names;
    const std::string wanted = Upper(extension);
    std::error_code error;
    for (const auto& entry :
         std::filesystem::directory_iterator(install.directory, error)) {
        if (entry.is_regular_file(error) &&
            HasExtension(entry.path(), wanted)) {
            names.push_back(entry.path().filename().string());
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace sdl3cpp::services::impl
