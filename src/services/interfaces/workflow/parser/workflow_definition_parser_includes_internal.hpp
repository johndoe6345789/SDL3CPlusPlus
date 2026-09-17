#pragma once

/// Internal helper shared by workflow_definition_parser_include_path.cpp
/// and workflow_definition_parser_expand_include.cpp, which together
/// implement WorkflowDefinitionParser::ResolveIncludes() from
/// workflow_definition_parser.hpp. Not part of the public parser API.

#include <filesystem>
#include <string>

namespace sdl3cpp::services::impl::workflow_include_detail {

// Resolve the include path:
//   1. Absolute path → use directly.
//   2. Relative path → try CWD first (game assets live under
//      CWD/packages/...), then relative to the including file's directory.
std::filesystem::path ResolvePath(const std::string& rawPath,
                                  const std::filesystem::path& baseDir);

}  // namespace sdl3cpp::services::impl::workflow_include_detail
