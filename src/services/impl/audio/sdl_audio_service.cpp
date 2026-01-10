#include "sdl_audio_service.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

namespace {
constexpr int kDecodeChunkSize = 4096;
constexpr int kMixFrames = 1024;
uint32_t GetBytesPerFrame(const SDL_AudioSpec& spec) {
    const uint32_t formatMask = SDL_AUDIO_MASK_BITSIZE;
    const uint32_t bitsPerSample = static_cast<uint32_t>(spec.format) & formatMask;
    return (bitsPerSample / 8u) * static_cast<uint32_t>(spec.channels);
}
}  // namespace

SdlAudioService::SdlAudioService(std::shared_ptr<ILogger> logger)
    : logger_(logger) {
    if (logger_) {
        logger_->Trace("SdlAudioService", "SdlAudioService");
    }
}

SdlAudioService::~SdlAudioService() {
    if (logger_) {
        logger_->Trace("SdlAudioService", "~SdlAudioService");
    }
    if (initialized_) {
        Shutdown();
    }
}

void SdlAudioService::Initialize() {
    if (logger_) {
        logger_->Trace("SdlAudioService", "Initialize");
    }

    if (initialized_) {
        return;
    }

    // Initialize SDL audio
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        throw std::runtime_error("Failed to initialize SDL audio: " + std::string(SDL_GetError()));
    }

    // Set up desired audio spec
    SDL_AudioSpec desiredSpec{};
    desiredSpec.format = SDL_AUDIO_S16;
    desiredSpec.channels = 2;
    desiredSpec.freq = 44100;

    // Open audio device stream (SDL3 way)
    audioStream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desiredSpec, nullptr, nullptr);
    if (!audioStream_) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        throw std::runtime_error("Failed to open audio device stream: " + std::string(SDL_GetError()));
    }

    mixSpec_ = desiredSpec;
    SDL_AudioSpec inputSpec{};
    if (SDL_GetAudioStreamFormat(audioStream_, &inputSpec, nullptr)) {
        mixSpec_ = inputSpec;
    }

    // Start the audio stream
    if (!SDL_ResumeAudioStreamDevice(audioStream_)) {
        SDL_DestroyAudioStream(audioStream_);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        throw std::runtime_error("Failed to resume audio stream: " + std::string(SDL_GetError()));
    }

    initialized_ = true;
    if (logger_) {
        logger_->Info("SDL audio service initialized successfully");
    }
}

void SdlAudioService::Shutdown() noexcept {
    if (logger_) {
        logger_->Trace("SdlAudioService", "Shutdown");
    }

    if (!initialized_) {
        return;
    }

    // Stop and cleanup audio
    if (audioStream_) {
        SDL_PauseAudioStreamDevice(audioStream_);
        SDL_DestroyAudioStream(audioStream_);
        audioStream_ = nullptr;
    }

    std::lock_guard<std::mutex> lock(audioMutex_);
    if (backgroundAudio_) {
        CleanupAudioData(*backgroundAudio_);
        backgroundAudio_.reset();
    }
    for (auto& effect : effectAudio_) {
        CleanupAudioData(*effect);
    }
    effectAudio_.clear();
    mixBuffer_.clear();
    tempBuffer_.clear();
    outputBuffer_.clear();

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    initialized_ = false;
    if (logger_) {
        logger_->Info("SDL audio service shutdown");
    }
}

void SdlAudioService::PlayBackground(const std::filesystem::path& path, bool loop) {
    if (logger_) {
        logger_->Trace("SdlAudioService", "PlayBackground",
                       "path=" + path.string() +
                       ", loop=" + std::string(loop ? "true" : "false"));
    }

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
    backgroundAudio_->finished = false;

    if (logger_) {
        logger_->Info("Playing background audio: " + path.string() + " (loop: " + std::to_string(loop) + ")");
    }
}

void SdlAudioService::PlayEffect(const std::filesystem::path& path, bool loop) {
    if (logger_) {
        logger_->Trace("SdlAudioService", "PlayEffect",
                       "path=" + path.string() +
                       ", loop=" + std::string(loop ? "true" : "false"));
    }

    if (!initialized_) {
        throw std::runtime_error("Audio service not initialized");
    }

    std::lock_guard<std::mutex> lock(audioMutex_);
    auto effect = std::make_unique<AudioData>();
    if (!LoadAudioFile(path, *effect)) {
        throw std::runtime_error("Failed to load audio file: " + path.string());
    }
    effect->loop = loop;
    effect->finished = false;
    effectAudio_.push_back(std::move(effect));

    if (logger_) {
        logger_->Info("Playing effect audio: " + path.string() + " (loop: " + std::to_string(loop) + ")");
    }
}

void SdlAudioService::StopBackground() {
    if (logger_) {
        logger_->Trace("SdlAudioService", "StopBackground");
    }

    if (!initialized_) {
        return;
    }

    std::lock_guard<std::mutex> lock(audioMutex_);
    if (backgroundAudio_) {
        CleanupAudioData(*backgroundAudio_);
        backgroundAudio_.reset();
    }

    if (logger_) {
        logger_->Info("Stopped background audio");
    }
}

void SdlAudioService::StopAll() {
    if (logger_) {
        logger_->Trace("SdlAudioService", "StopAll");
    }

    if (!initialized_) {
        return;
    }

    std::lock_guard<std::mutex> lock(audioMutex_);
    if (backgroundAudio_) {
        CleanupAudioData(*backgroundAudio_);
        backgroundAudio_.reset();
    }
    for (auto& effect : effectAudio_) {
        CleanupAudioData(*effect);
    }
    effectAudio_.clear();

    if (logger_) {
        logger_->Info("Stopped all audio");
    }
}

void SdlAudioService::SetVolume(float volume) {
    if (logger_) {
        logger_->Trace("SdlAudioService", "SetVolume",
                       "volume=" + std::to_string(volume));
    }
    std::lock_guard<std::mutex> lock(audioMutex_);
    volume_ = std::clamp(volume, 0.0f, 1.0f);
    if (logger_) {
        logger_->TraceVariable("volume", volume_);
    }
}

float SdlAudioService::GetVolume() const {
    if (logger_) {
        logger_->Trace("SdlAudioService", "GetVolume");
    }
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(audioMutex_));
    return volume_;
}

bool SdlAudioService::IsBackgroundPlaying() const {
    if (logger_) {
        logger_->Trace("SdlAudioService", "IsBackgroundPlaying");
    }
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(audioMutex_));
    return backgroundAudio_ && backgroundAudio_->isOpen && !backgroundAudio_->finished;
}

void SdlAudioService::Update() {
    if (logger_) {
        logger_->Trace("SdlAudioService", "Update");
    }
    if (!initialized_ || !audioStream_) {
        return;
    }

    std::lock_guard<std::mutex> lock(audioMutex_);

    if (!backgroundAudio_ && effectAudio_.empty()) {
        return;
    }

    const uint32_t bytesPerFrame = GetBytesPerFrame(mixSpec_);
    if (bytesPerFrame == 0) {
        return;
    }

    int queuedBytes = SDL_GetAudioStreamQueued(audioStream_);
    if (queuedBytes < 0) {
        if (logger_) {
            logger_->Error("Failed to query audio queue: " + std::string(SDL_GetError()));
        }
        return;
    }

    const uint32_t queueLimit = bytesPerFrame * static_cast<uint32_t>(kMixFrames);
    if (static_cast<uint32_t>(queuedBytes) >= queueLimit) {
        return;
    }

    const int frames = kMixFrames;
    const int channels = mixSpec_.channels;
    const size_t sampleCount = static_cast<size_t>(frames * channels);

    mixBuffer_.assign(sampleCount, 0);
    bool hasAudio = false;

    if (backgroundAudio_ && backgroundAudio_->isOpen) {
        int bytesRead = ReadStreamSamples(*backgroundAudio_, tempBuffer_, frames);
        if (bytesRead > 0) {
            size_t samplesRead = static_cast<size_t>(bytesRead / static_cast<int>(sizeof(int16_t)));
            for (size_t i = 0; i < samplesRead; ++i) {
                mixBuffer_[i] += tempBuffer_[i];
            }
            hasAudio = true;
        }
        if (backgroundAudio_->finished &&
            backgroundAudio_->convertStream &&
            SDL_GetAudioStreamAvailable(backgroundAudio_->convertStream) == 0) {
            CleanupAudioData(*backgroundAudio_);
            backgroundAudio_.reset();
        }
    }

    for (auto it = effectAudio_.begin(); it != effectAudio_.end(); ) {
        AudioData& effect = *(*it);
        int bytesRead = ReadStreamSamples(effect, tempBuffer_, frames);
        if (bytesRead > 0) {
            size_t samplesRead = static_cast<size_t>(bytesRead / static_cast<int>(sizeof(int16_t)));
            for (size_t i = 0; i < samplesRead; ++i) {
                mixBuffer_[i] += tempBuffer_[i];
            }
            hasAudio = true;
        }
        bool drained = effect.finished && effect.convertStream &&
                       SDL_GetAudioStreamAvailable(effect.convertStream) == 0;
        if (!effect.isOpen || drained) {
            CleanupAudioData(effect);
            it = effectAudio_.erase(it);
        } else {
            ++it;
        }
    }

    if (!hasAudio) {
        return;
    }

    outputBuffer_.assign(sampleCount, 0);
    const float volume = volume_;
    for (size_t i = 0; i < sampleCount; ++i) {
        const float mixed = static_cast<float>(mixBuffer_[i]) * volume;
        int32_t value = static_cast<int32_t>(mixed);
        value = std::clamp(value, -32768, 32767);
        outputBuffer_[i] = static_cast<int16_t>(value);
    }

    int bytesToQueue = static_cast<int>(sampleCount * sizeof(int16_t));
    if (!SDL_PutAudioStreamData(audioStream_, outputBuffer_.data(), bytesToQueue)) {
        if (logger_) {
            logger_->Error("Failed to queue audio data: " + std::string(SDL_GetError()));
        }
    }
}

bool SdlAudioService::LoadAudioFile(const std::filesystem::path& path, AudioData& audioData) {
    if (logger_) {
        logger_->Trace("SdlAudioService", "LoadAudioFile",
                       "path=" + path.string() +
                       ", audioData.isOpen=" + std::string(audioData.isOpen ? "true" : "false"));
    }
    std::string pathText = path.string();
    FILE* file = fopen(pathText.c_str(), "rb");
    if (!file) {
        if (logger_) {
            logger_->Error("Failed to open audio file: " + path.string());
        }
        return false;
    }

    if (ov_open(file, &audioData.vorbisFile, nullptr, 0) < 0) {
        fclose(file);
        if (logger_) {
            logger_->Error("Failed to open vorbis file: " + path.string());
        }
        return false;
    }

    const vorbis_info* info = ov_info(&audioData.vorbisFile, -1);
    if (!info) {
        ov_clear(&audioData.vorbisFile);
        if (logger_) {
            logger_->Error("Failed to read vorbis info: " + path.string());
        }
        return false;
    }

    audioData.sourceSpec.format = SDL_AUDIO_S16;
    audioData.sourceSpec.channels = info->channels;
    audioData.sourceSpec.freq = static_cast<int>(info->rate);
    audioData.convertStream = SDL_CreateAudioStream(&audioData.sourceSpec, &mixSpec_);
    if (!audioData.convertStream) {
        ov_clear(&audioData.vorbisFile);
        if (logger_) {
            logger_->Error("Failed to create audio stream: " + std::string(SDL_GetError()));
        }
        return false;
    }

    audioData.isOpen = true;
    audioData.finished = false;
    return true;
}

void SdlAudioService::CleanupAudioData(AudioData& audioData) {
    if (logger_) {
        logger_->Trace("SdlAudioService", "CleanupAudioData",
                       "audioData.isOpen=" + std::string(audioData.isOpen ? "true" : "false"));
    }
    if (audioData.convertStream) {
        SDL_DestroyAudioStream(audioData.convertStream);
        audioData.convertStream = nullptr;
    }
    if (audioData.isOpen) {
        ov_clear(&audioData.vorbisFile);
        audioData.isOpen = false;
    }
    audioData.finished = false;
}

int SdlAudioService::ReadStreamSamples(AudioData& audioData, std::vector<int16_t>& output, int frames) {
    if (logger_) {
        logger_->Trace("SdlAudioService", "ReadStreamSamples",
                       "frames=" + std::to_string(frames) +
                       ", audioData.isOpen=" + std::string(audioData.isOpen ? "true" : "false"));
    }
    if (!audioData.isOpen || !audioData.convertStream) {
        return 0;
    }

    const uint32_t bytesPerFrame = GetBytesPerFrame(mixSpec_);
    if (bytesPerFrame == 0) {
        return 0;
    }
    const uint32_t bytesNeeded = bytesPerFrame * static_cast<uint32_t>(frames);
    if (bytesNeeded > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
        audioData.finished = true;
        return 0;
    }
    const int bytesNeededInt = static_cast<int>(bytesNeeded);
    const size_t sampleCount = static_cast<size_t>(bytesNeededInt / static_cast<int>(sizeof(int16_t)));
    output.assign(sampleCount, 0);

    while (!audioData.finished) {
        int available = SDL_GetAudioStreamAvailable(audioData.convertStream);
        if (available < 0) {
            if (logger_) {
                logger_->Error("Failed to query audio stream: " + std::string(SDL_GetError()));
            }
            audioData.finished = true;
            break;
        }
        if (available >= bytesNeededInt) {
            break;
        }

        char decodeBuffer[kDecodeChunkSize];
        long bytesReadLong = ov_read(&audioData.vorbisFile, decodeBuffer, kDecodeChunkSize, 0, 2, 1, nullptr);
        if (bytesReadLong > 0) {
            if (bytesReadLong > std::numeric_limits<int>::max()) {
                bytesReadLong = std::numeric_limits<int>::max();
            }
            const int bytesRead = static_cast<int>(bytesReadLong);
            if (!SDL_PutAudioStreamData(audioData.convertStream, decodeBuffer, bytesRead)) {
                if (logger_) {
                    logger_->Error("Failed to queue decoded audio: " + std::string(SDL_GetError()));
                }
                audioData.finished = true;
                break;
            }
        } else if (bytesReadLong == 0) {
            if (audioData.loop) {
                ov_pcm_seek(&audioData.vorbisFile, 0);
                continue;
            }
            audioData.finished = true;
            SDL_FlushAudioStream(audioData.convertStream);
            break;
        } else {
            if (logger_) {
                logger_->Error("Vorbis decode error");
            }
            audioData.finished = true;
            break;
        }
    }

    int bytesRead = SDL_GetAudioStreamData(audioData.convertStream, output.data(), bytesNeededInt);
    if (bytesRead < 0) {
        if (logger_) {
            logger_->Error("Failed to read audio stream data: " + std::string(SDL_GetError()));
        }
        audioData.finished = true;
        return 0;
    }

    if (bytesRead < bytesNeededInt) {
        const size_t samplesRead = static_cast<size_t>(bytesRead / static_cast<int>(sizeof(int16_t)));
        if (samplesRead < output.size()) {
            using Difference = std::vector<int16_t>::difference_type;
            std::fill(output.begin() + static_cast<Difference>(samplesRead), output.end(), 0);
        }
    }

    return bytesRead;
}

}  // namespace sdl3cpp::services::impl
