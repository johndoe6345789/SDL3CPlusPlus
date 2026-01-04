#include "sdl_audio_service.hpp"
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace sdl3cpp::services::impl {

SdlAudioService::SdlAudioService(std::shared_ptr<ILogger> logger)
    : logger_(logger) {}

SdlAudioService::~SdlAudioService() {
    if (initialized_) {
        Shutdown();
    }
}

void SdlAudioService::Initialize() {
    logger_->TraceFunction(__func__);

    if (initialized_) {
        return;
    }

    // Initialize SDL audio
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        throw std::runtime_error("Failed to initialize SDL audio: " + std::string(SDL_GetError()));
    }

    // Set up desired audio spec
    SDL_AudioSpec desiredSpec;
    SDL_zero(desiredSpec);
    desiredSpec.format = SDL_AUDIO_S16;
    desiredSpec.channels = 2;
    desiredSpec.freq = 44100;

    // Open audio device stream (SDL3 way)
    audioStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desiredSpec, nullptr, nullptr);
    if (!audioStream_) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        throw std::runtime_error("Failed to open audio device stream: " + std::string(SDL_GetError()));
    }

    // Start the audio stream
    if (!SDL_ResumeAudioStreamDevice(audioStream_)) {
        SDL_DestroyAudioStream(audioStream_);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        throw std::runtime_error("Failed to resume audio stream: " + std::string(SDL_GetError()));
    }

    initialized_ = true;
    logger_->Info("SDL audio service initialized successfully");
}

void SdlAudioService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    // Stop and cleanup audio
    if (audioStream_) {
        SDL_PauseAudioStreamDevice(audioStream_);
        SDL_DestroyAudioStream(audioStream_);
        audioStream_ = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        if (backgroundAudio_) {
            CleanupAudioData(*backgroundAudio_);
            backgroundAudio_.reset();
        }
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    initialized_ = false;
    logger_->Info("SDL audio service shutdown");
}

void SdlAudioService::PlayBackground(const std::filesystem::path& path, bool loop) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Audio service not initialized");
    }

    std::lock_guard<std::mutex> lock(audioMutex_);

    // Stop current background audio
    if (backgroundAudio_) {
        CleanupAudioData(*backgroundAudio_);
        backgroundAudio_.reset();
    }

    // Load new audio file
    backgroundAudio_ = std::make_unique<AudioData>();
    if (!LoadAudioFile(path, *backgroundAudio_)) {
        backgroundAudio_.reset();
        throw std::runtime_error("Failed to load audio file: " + path.string());
    }

    backgroundAudio_->loop = loop;
    backgroundAudio_->position = 0;

    logger_->Info("Playing background audio: " + path.string() + " (loop: " + std::to_string(loop) + ")");
}

void SdlAudioService::PlayEffect(const std::filesystem::path& path, bool loop) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Audio service not initialized");
    }

    // For now, effects are not implemented - could be added later
    logger_->Info("Playing effect audio: " + path.string() + " (loop: " + std::to_string(loop) + ") - NOT IMPLEMENTED");
}

void SdlAudioService::StopBackground() {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    std::lock_guard<std::mutex> lock(audioMutex_);
    if (backgroundAudio_) {
        CleanupAudioData(*backgroundAudio_);
        backgroundAudio_.reset();
    }

    logger_->Info("Stopped background audio");
}

void SdlAudioService::StopAll() {
    logger_->TraceFunction(__func__);

    StopBackground();
    // Effects would be stopped here too
    logger_->Info("Stopped all audio");
}

void SdlAudioService::SetVolume(float volume) {
    volume_ = std::clamp(volume, 0.0f, 1.0f);
    logger_->TraceVariable("volume", volume_);
}

float SdlAudioService::GetVolume() const {
    return volume_;
}

bool SdlAudioService::IsBackgroundPlaying() const {
    std::lock_guard<std::mutex> lock(audioMutex_);
    return backgroundAudio_ != nullptr;
}

void SdlAudioService::Update() {
    if (!initialized_ || !audioStream_) {
        return;
    }

    std::lock_guard<std::mutex> lock(audioMutex_);

    if (!backgroundAudio_ || !backgroundAudio_->isOpen) {
        return;
    }

    // Check if we need more audio data
    if (SDL_GetAudioStreamQueued(audioStream_) < 4096) {
        // Read audio data from vorbis file
        char buffer[4096];
        int bytesRead = 0;
        int totalBytesRead = 0;

        while (totalBytesRead < 4096) {
            bytesRead = ov_read(&backgroundAudio_->vorbisFile, buffer + totalBytesRead, 4096 - totalBytesRead, 0, 2, 1, nullptr);
            if (bytesRead <= 0) {
                // End of file
                if (backgroundAudio_->loop) {
                    // Loop back to beginning
                    ov_pcm_seek(&backgroundAudio_->vorbisFile, 0);
                    continue;
                } else {
                    // Stop playback
                    CleanupAudioData(*backgroundAudio_);
                    backgroundAudio_.reset();
                    break;
                }
            }
            totalBytesRead += bytesRead;
        }

        if (totalBytesRead > 0) {
            // Queue the audio data to the stream
            if (SDL_PutAudioStreamData(audioStream_, buffer, totalBytesRead) < 0) {
                logger_->Error("Failed to queue audio data: " + std::string(SDL_GetError()));
            }
        }
    }
}

bool SdlAudioService::LoadAudioFile(const std::filesystem::path& path, AudioData& audioData) {
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
        logger_->Error("Failed to open audio file: " + path.string());
        return false;
    }

    if (ov_open(file, &audioData.vorbisFile, nullptr, 0) < 0) {
        fclose(file);
        logger_->Error("Failed to open vorbis file: " + path.string());
        return false;
    }

    audioData.isOpen = true;
    return true;
}

void SdlAudioService::CleanupAudioData(AudioData& audioData) {
    if (audioData.isOpen) {
        ov_clear(&audioData.vorbisFile);
        audioData.isOpen = false;
    }
}

}  // namespace sdl3cpp::services::impl
