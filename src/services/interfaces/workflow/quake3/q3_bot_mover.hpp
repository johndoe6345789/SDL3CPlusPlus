#pragma once

#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow/quake3/q3_pmove.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/**
 * @brief The bot's movement state, kept in its JSON between frames.
 *
 * A bot needs somewhere to remember velocity and whether it was stood
 * on something, for the same reason a player does: without it there is
 * nothing for gravity to accumulate into and nothing for friction to
 * bleed off. Storing it alongside "pos" keeps the bot list the one
 * description of a bot, so spawning and respawning stay unchanged.
 */
Q3PlayerState ReadBotMoveState(const nlohmann::json& bot);
void WriteBotMoveState(nlohmann::json& bot, const Q3PlayerState& ps);

/**
 * @brief Moves one bot for a frame by running it through pmove.
 *
 * ioq3 never moves a bot itself: the AI produces a usercmd and the
 * server runs the very same Pmove() on it that it runs for a human
 * (g_active.c G_RunClient -> ClientThink_real). Collision, gravity and
 * stair-stepping then come for free, which is what stops bots hovering
 * and walking through walls. `cmd` is zero for a bot that does not want
 * to go anywhere; it is still run, so it still falls.
 */
void MoveBotThroughPmove(nlohmann::json& bot, const q3::Q3UserCmd& cmd,
                         btDiscreteDynamicsWorld* world, float dt);

}  // namespace sdl3cpp::services::impl
