#include "services/interfaces/app/cli_env_override.hpp"

#include <cstdlib>

namespace sdl3cpp::services::app {

bool ApplyEnvOverride(const std::string& assignment) {
    const std::size_t separator = assignment.find('=');
    if (separator == std::string::npos || separator == 0) {
        return false;
    }

    const std::string name = assignment.substr(0, separator);
    const std::string value = assignment.substr(separator + 1);

#ifdef _WIN32
    return _putenv_s(name.c_str(), value.c_str()) == 0;
#else
    return setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

}  // namespace sdl3cpp::services::app
