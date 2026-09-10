#include "services/interfaces/workflow/workflow_definition_parser_includes_internal.hpp"

namespace fs = std::filesystem;

namespace sdl3cpp::services::impl::workflow_include_detail {

fs::path ResolvePath(const std::string& rawPath, const fs::path& baseDir) {
    fs::path p(rawPath);
    if (p.is_absolute()) return p;

    // CWD-relative first (matches how shader/asset paths work in workflows)
    const fs::path cwdCandidate = fs::current_path() / p;
    if (fs::exists(cwdCandidate)) return cwdCandidate;

    // Fallback: relative to the including file
    const fs::path relCandidate = baseDir / p;
    if (fs::exists(relCandidate)) return relCandidate;

    // Return CWD-relative even if it doesn't exist yet — ParseFile will throw a
    // better "file not found" message than we would.
    return cwdCandidate;
}

}  // namespace sdl3cpp::services::impl::workflow_include_detail
