#pragma once

#include <filesystem>

namespace sdl3cpp::services {

/**
 * @brief Audio playback service interface.
 *
 * Handles background music and sound effect playback using SDL audio.
 * Wraps the AudioPlayer class with a clean service interface.
 */
class IAudioService {
public:
    virtual ~IAudioService() = default;

    /**
     * @brief Initialize the audio subsystem.
     *
     * @throws std::runtime_error if audio initialization fails
     */
    virtual void Initialize() = 0;

    /**
     * @brief Shutdown the audio subsystem and stop all playback.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Play a background music track.
     *
     * Only one background track can play at a time. Calling this
     * method will stop any currently playing background music.
     *
     * @param path Path to the audio file (OGG, WAV, etc.)
     * @param loop Whether to loop the track
     * @throws std::runtime_error if file cannot be loaded
     */
    virtual void PlayBackground(const std::filesystem::path& path, bool loop = true) = 0;

    /**
     * @brief Play a sound effect.
     *
     * Multiple sound effects can play simultaneously.
     *
     * @param path Path to the audio file
     * @param loop Whether to loop the effect (usually false)
     * @throws std::runtime_error if file cannot be loaded
     */
    virtual void PlayEffect(const std::filesystem::path& path, bool loop = false) = 0;

    /**
     * @brief Stop background music playback.
     */
    virtual void StopBackground() = 0;

    /**
     * @brief Stop all audio playback (background and effects).
     */
    virtual void StopAll() = 0;

    /**
     * @brief Set master volume.
     *
     * @param volume Volume level from 0.0 (silent) to 1.0 (full)
     */
    virtual void SetVolume(float volume) = 0;

    /**
     * @brief Get the current master volume.
     *
     * @return Volume level from 0.0 to 1.0
     */
    virtual float GetVolume() const = 0;

    /**
     * @brief Check if background music is currently playing.
     *
     * @return true if playing, false otherwise
     */
    virtual bool IsBackgroundPlaying() const = 0;

    /**
     * @brief Update streaming audio buffers.
     *
     * Call regularly from the main loop to feed audio data.
     */
    virtual void Update() = 0;
};

}  // namespace sdl3cpp::services
