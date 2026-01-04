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
#if !defined(__LP64__) || defined(_WIN64)
// Only define unsigned long overload if it's different from size_t
// On LP64 systems (Linux x64), size_t is unsigned long, causing redefinition
std::string ToString(unsigned long value);
#endif

} // namespace sdl3cpp::logging

#endif // SDL3CPP_LOGGING_STRING_UTILS_HPP
