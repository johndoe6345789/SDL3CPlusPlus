#pragma once

/// Internal helper shared by bsp_shader_script_parse.cpp and
/// bsp_shader_script.cpp, which together implement LoadShaderImages()
/// from bsp_shader_script.hpp. Not part of the public workflow-step API.

#include <map>
#include <string>

namespace sdl3cpp::services::impl::bsp_shader_script_detail {

/// Parses one .shader script's `map`/`clampmap` directives into `out`,
/// keyed by shader name, mapping to the referenced image path with its
/// extension stripped. The first directive found per shader wins.
void ParseShaderScript(const std::string& text,
                       std::map<std::string, std::string>& out);

}  // namespace sdl3cpp::services::impl::bsp_shader_script_detail
