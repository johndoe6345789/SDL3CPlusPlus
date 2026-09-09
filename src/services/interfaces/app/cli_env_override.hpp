#pragma once

#include <string>

namespace sdl3cpp::services::app {

/**
 * @brief Apply a "NAME=VALUE" assignment to the process environment.
 *
 * Workflow JSON reads values such as the Quake 3 pk3 path through
 * ${env:NAME} placeholders, which are expanded from the environment at
 * load time. Letting the command line write those variables means a
 * path can be supplied per run without editing the workflow or exporting
 * anything in the shell.
 *
 * @param assignment Text of the form NAME=VALUE. VALUE may be empty and
 *                   may itself contain '=' characters; only the first
 *                   '=' separates the two. NAME may not be empty.
 * @return false when the assignment is malformed or the environment
 *         could not be written; the caller reports this to the user.
 */
bool ApplyEnvOverride(const std::string& assignment);

}  // namespace sdl3cpp::services::app
