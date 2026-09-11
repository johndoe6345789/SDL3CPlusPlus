#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_radio.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <SDL3/SDL_audio.h>

#include <cstdint>
#include <future>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.radio
 *
 * Into a car and the radio comes on at a random station, part-way
 * through a random track as if it had been playing all along, its name
 * beside the clock for a few seconds (gta5.radio.text). When a track
 * ends another from the station starts; getting out turns it off.
 * Stations are the folders of WAVs under `dir` -- GTA's own, exported
 * from its RADIO_* banks -- named by `names`. With none exported,
 * stand-in stations play. Opens its own audio device, at `volume`.
 */
class WorkflowGta5RadioStep final : public IWorkflowStep {
public:
    WorkflowGta5RadioStep(std::shared_ptr<ILogger> logger,
                          std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Open(const WorkflowStepDefinition& step, WorkflowContext& context);
    void Tune();  // starts reading a random track of station_
    void Play(Gta5Clip track);
    void Stop();

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    std::vector<Gta5Station> stations_;
    SDL_AudioDeviceID device_{0};
    SDL_AudioSpec spec_{};
    SDL_AudioStream* stream_{nullptr};
    std::future<Gta5Clip> loading_;
    std::mt19937 rng_{std::random_device{}()};
    std::uint64_t shownUntilMs_{0};
    float volume_{0.5f};
    int station_{0};
    bool midway_{false};  // the next track starts part-way through
    bool seated_{false};
    bool tried_{false};
};

}  // namespace sdl3cpp::services::impl
