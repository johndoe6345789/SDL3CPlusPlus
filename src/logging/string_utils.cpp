#include "string_utils.hpp"
#include <sstream>
#include <string>

namespace sdl3cpp::logging {

std::string ToString(int value) {
    return std::to_string(value);
}

std::string ToString(size_t value) {
    return std::to_string(value);
}

std::string ToString(bool value) {
    return value ? "true" : "false";
}

std::string ToString(float value) {
    return std::to_string(value);
}

std::string ToString(double value) {
    return std::to_string(value);
}

std::string ToString(const void* value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string ToString(unsigned long value) {
    return std::to_string(value);
}

} // namespace sdl3cpp::logging
