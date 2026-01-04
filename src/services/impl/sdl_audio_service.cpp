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
    desiredSpec.freq = 44100;
    desiredSpec.format = SDL_AUDIO_S16;
    desiredSpec.channels = 2;
    desiredSpec.samples = 4096;
    desiredSpec.callback = AudioCallback;
    desiredSpec.userdata = this;

    // Open audio device
    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, &audioSpec_, 0);
    if (audioDevice_ == 0) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        throw std::runtime_error("Failed to open audio device: " + std::string(SDL_GetError()));
    }

    // Start audio playback
    SDL_PlayAudioDevice(audioDevice_);

    initialized_ = true;
    logger_->Info("SDL audio service initialized successfully");
}

void SdlAudioService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    // Stop and cleanup audio
    if (audioDevice_ != 0) {
        SDL_PauseAudioDevice(audioDevice_, 1); // Pause
        SDL_CloseAudioDevice(audioDevice_);
        audioDevice_ = 0;
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

void SdlAudioService::AudioCallback(void* userdata, Uint8* stream, int len) {
    auto* service = static_cast<SdlAudioService*>(userdata);
    service->MixAudio(stream, len);
}

void SdlAudioService::MixAudio(Uint8* stream, int len) {
    std::lock_guard<std::mutex> lock(audioMutex_);

    // Clear the stream
    SDL_memset(stream, 0, len);

    if (!backgroundAudio_ || !backgroundAudio_->isOpen) {
        return;
    }

    // Read audio data from vorbis file
    char buffer[4096];
    int bytesRead = 0;
    int totalBytesRead = 0;

    while (totalBytesRead < len) {
        bytesRead = ov_read(&backgroundAudio_->vorbisFile, buffer, std::min(4096, len - totalBytesRead), 0, 2, 1, nullptr);
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

        // Mix audio data (simple copy for now, could add volume mixing)
        SDL_MixAudio(stream + totalBytesRead, reinterpret_cast<Uint8*>(buffer), bytesRead, SDL_MIX_MAXVOLUME);
        totalBytesRead += bytesRead;
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
