#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief What `video.record.begin` records, and for how long.
 *
 * Read from the step's parameters, each overridable from the
 * environment so a recording needs no workflow edit:
 *
 * | parameter        | environment              | default         |
 * |------------------|--------------------------|-----------------|
 * | `output_path`    | `SDL3CPP_RECORD`         | "" (off)        |
 * | `start_after`    | `SDL3CPP_RECORD_DELAY`   | 0 seconds       |
 * | `seconds`        | `SDL3CPP_RECORD_SECONDS` | 0 (until exit)  |
 * | `fps`            | `SDL3CPP_RECORD_FPS`     | 30              |
 * | `scale`          | `SDL3CPP_RECORD_SCALE`   | 1 (window size) |
 * | `quality`        | `SDL3CPP_RECORD_CRF`     | 23              |
 * | `audio`          | `SDL3CPP_RECORD_AUDIO`   | 1 (on)          |
 * | `exit_when_done` | `SDL3CPP_RECORD_EXIT`    | 0               |
 *
 * `exit_when_done` sets `running_key` (default `game_running`) false
 * once `seconds` have been recorded, ending the frame loop.
 */
struct VideoRecorderSettings {
    std::string path;
    double delay           = 0.0;
    double seconds         = 0.0;
    int fps                = 30;
    double scale           = 1.0;
    int quality            = 23;
    bool audio             = true;
    bool exitWhenDone      = false;
    std::string runningKey = "game_running";
};

VideoRecorderSettings ReadVideoRecorderSettings(
    const WorkflowStepDefinition& step);

}  // namespace sdl3cpp::services::impl
