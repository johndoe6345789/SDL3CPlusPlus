#ifndef SDL3CPP_LOGGING_STRING_UTILS_HPP
#define SDL3CPP_LOGGING_STRING_UTILS_HPP

#include <string>

namespace sdl3cpp::logging {

// Helper functions to convert common types to strings for logging
std::string ToString(int value);
std::string ToString(size_t value);
std::string ToString(bool value);
std::string ToString(float value);
std::string ToString(double value);
std::string ToString(const void* value);
std::string ToString(unsigned long value);

} // namespace sdl3cpp::logging

#endif // SDL3CPP_LOGGING_STRING_UTILS_HPP
