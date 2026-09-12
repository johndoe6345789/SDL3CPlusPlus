#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/audio/gta5_engine_bank.hpp"
#include "services/interfaces/workflow/gta5/audio/gta5_sound.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"
#include "services/interfaces/workflow/gta5/world/gta5_water.hpp"

#include <memory>
#include <random>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.sound
 *
 * The world as heard. Footfalls land as the character's feet do (the
 * walk's own phase); in water, strokes, and a splash going in. The car
 * last driven runs its engine at its revs -- five gears, climbing
 * through each -- fading with distance once left. The shore laps within
 * 8 m of water_file's water, and under it the wash deepens and muffles.
 * Sounds are the WAVs under `dir` (see LoadGta5Sounds), the engine
 * dir/engine/<engine_bank> (LoadGta5EngineBank); `volume` scales them
 * all. Opens its own audio device.
 */
class WorkflowGta5SoundStep final : public IWorkflowStep {
public:
    WorkflowGta5SoundStep(std::shared_ptr<ILogger> logger,
                          std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void Open(const WorkflowStepDefinition& step);
    void Feet(WorkflowContext& context, float dt);
    void Engine(WorkflowContext& context, float dt);
    void Water(WorkflowContext& context);
    /// GTA's gunfire and explosions, as the weapon counts them.
    void Shots(WorkflowContext& context);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
    Gta5Sounds sounds_;
    std::vector<Gta5WaterQuad> water_;
    SDL_AudioDeviceID device_{0};
    SDL_AudioSpec spec_{};
    Gta5Loop engineLoop_, waterLoop_;
    Gta5EngineBank engineBank_;  // GTA's granular engine, when exported
    Gta5EngineVoice engineVoice_;
    std::vector<SDL_AudioStream*> playing_;
    std::mt19937 rng_{20260911u};
    float volume_{1.f};
    float walkPhase_{0.f};    // as the ped walk's: footfalls at pi/2, 3pi/2
    float strokePhase_{0.f};  // 0..1 through a stroke
    float revs_{0.f};         // 0 idle .. 1 the top of a gear
    int car_{-1};             // the car last driven, its engine running
    int shots_{0};
    int blasts_{0};
    bool wasSwimming_{false};
    bool tried_{false};
};

}  // namespace sdl3cpp::services::impl
