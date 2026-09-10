#pragma once

#include "services/interfaces/workflow/quake3/q3_pmove.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::q3 {

/**
 * @brief Turns "go that way, this fast" into a Q3UserCmd.
 *
 * A port of ioq3 ai_main.c BotInputToUserCommand. The bot AI only ever
 * says which way it wants to go in world space; the movement itself is
 * whatever Pmove makes of the resulting command, which is why bots
 * collide, fall and climb stairs exactly as a player does.
 *
 * @p worldDir need not be normalised and its vertical component is
 * ignored — bots steer horizontally and leave the rest to gravity.
 * @p speedFraction is ioq3's [0, 400] rescaled to [0, 1].
 *
 * The projection onto the bot's own facing is followed by a divide by
 * the largest component, so the dominant axis gets exactly the
 * requested speed and the other stays proportional: the same saturation
 * a player gets holding W and D together.
 */
Q3UserCmd BotDirectionToUserCmd(const glm::vec3& worldDir, float speedFraction,
                                float yaw);

/// A bot's configured speed as the fraction of full running speed that
/// BotDirectionToUserCmd wants. ioq3 asks for 400 out of 400 to run
/// flat out; here that is a speed equal to g_speed.
float BotSpeedFraction(float moveSpeed);

}  // namespace sdl3cpp::q3
