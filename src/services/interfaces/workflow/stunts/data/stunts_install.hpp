#pragma once

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Finds files in a Stunts directory whatever their case.
 *
 * Retail installs mix cases within one directory -- CTRACK04.TRK next
 * to ctrack01.trk -- so nothing may assume either, least of all on a
 * case-sensitive filesystem.
 */
struct StuntsInstall {
    std::string directory;
    bool valid = false;
};

/// Opens `directory`; `valid` is false when it holds no track files.
StuntsInstall OpenStuntsInstall(const std::string& directory);

/// Full path of `name` (matched case-insensitively), or "" if absent.
std::string FindStuntsFile(const StuntsInstall& install,
                           const std::string& name);

/// Every file with `extension` (".TRK"), sorted, as bare names.
std::vector<std::string> ListStuntsFiles(const StuntsInstall& install,
                                         const std::string& extension);

}  // namespace sdl3cpp::services::impl
