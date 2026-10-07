#pragma once

#include <string>

namespace sdl3cpp::services::impl {

/// What the player's racer reacts to, out loud.
enum class RacerVoiceEvent {
    Bump, Whoop, Damage, Hit, Air, Scream, Repaired, Win, Lose, Taunt
};

/// The line for an event in a racer's set (data/wavs/22K/Voice, e.g.
/// "assp017.wav"), from transcribing them: every racer shares one
/// scheme (001 contact, 002 a whoop, 004 damage, 005-006 hits, 007
/// airborne, 009 a scream, 010 fixed, 014 the win, 015 the loss,
/// 016-025 taunts) except Anakin, whose fix, win, loss and taunts sit
/// one line later. `variety` picks among several. Pure, so testable.
std::string RacerVoiceLineFile(const std::string& voice,
                               RacerVoiceEvent event, int variety);

}  // namespace sdl3cpp::services::impl
