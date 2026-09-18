#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_xml.hpp"

namespace sdl3cpp::fs2024 {

float Attr(const std::string& tag, const std::string& name, float fallback) {
    const std::string mark = name + "=\"";
    const auto at = tag.find(mark);
    if (at == std::string::npos) return fallback;
    return std::stof(tag.substr(at + mark.size()));
}

std::string OpeningTag(const std::string& xml, std::size_t at) {
    const auto end = xml.find('>', at);
    return end == std::string::npos ? std::string() : xml.substr(at, end - at);
}

}  // namespace sdl3cpp::fs2024
