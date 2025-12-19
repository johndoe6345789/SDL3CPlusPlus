#include "app/audio_player.hpp"

#include <SDL3/SDL.h>
#include <vorbis/vorbisfile.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace sdl3cpp::app {

namespace {

std::vector<int16_t> DecodeOgg(const std::filesystem::path& path, int& rate, int& channels) {
    FILE* file = std::fopen(path.string().c_str(), "rb");
    if (!file) {
        throw std::runtime_error("Failed to open audio file: " + path.string());
    }

    OggVorbis_File oggFile{};
    if (ov_open(file, &oggFile, nullptr, 0) < 0) {
        std::fclose(file);
        throw std::runtime_error("Failed to open OGG stream: " + path.string());
    }

    vorbis_info* info = ov_info(&oggFile, -1);
    if (!info) {
        ov_clear(&oggFile);
        throw std::runtime_error("Audio metadata is missing");
    }
    channels = info->channels;
    rate = static_cast<int>(info->rate);

    std::vector<int16_t> decoded;
    decoded.reserve(static_cast<size_t>(ov_pcm_total(&oggFile, -1)) * channels);
    int bitstream = 0;
    constexpr size_t kChunkBytes = 4096 * sizeof(int16_t);
    std::vector<char> chunk(kChunkBytes);

    while (true) {
        long bytesRead = ov_read(&oggFile, chunk.data(), static_cast<int>(chunk.size()), 0, 2, 1, &bitstream);
        if (bytesRead == 0) {
            break;
        }
        if (bytesRead < 0) {
            ov_clear(&oggFile);
            throw std::runtime_error("Error decoding OGG stream");
        }
        size_t samples = static_cast<size_t>(bytesRead) / sizeof(int16_t);
        size_t oldSize = decoded.size();
        decoded.resize(oldSize + samples);
        std::memcpy(decoded.data() + oldSize, chunk.data(), samples * sizeof(int16_t));
    }

    ov_clear(&oggFile);
    if (decoded.empty()) {
        throw std::runtime_error("Decoded audio is empty");
    }
    return decoded;
}

} // namespace

AudioPlayer::AudioPlayer(const std::filesystem::path& oggPath) {
    int sampleRate = 0;
    int channelCount = 0;
    buffer_ = DecodeOgg(oggPath, sampleRate, channelCount);
    bufferSizeBytes_ = buffer_.size() * sizeof(int16_t);

    SDL_AudioSpec desired{};
    desired.freq = sampleRate;
    desired.format = SDL_AUDIO_S16;
    desired.channels = channelCount;

    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, &AudioPlayer::AudioStreamCallback, this);
    if (!stream_) {
        throw std::runtime_error("Failed to open audio stream: " + std::string(SDL_GetError()));
    }
    if (!SDL_ResumeAudioStreamDevice(stream_)) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        throw std::runtime_error("Failed to resume audio stream device: " + std::string(SDL_GetError()));
    }
}

AudioPlayer::~AudioPlayer() {
    if (stream_) {
        SDL_PauseAudioStreamDevice(stream_);
        SDL_DestroyAudioStream(stream_);
    }
}

void AudioPlayer::AudioStreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount) {
    auto* self = static_cast<AudioPlayer*>(userdata);
    self->FeedStream(stream, totalAmount);
}

void AudioPlayer::FeedStream(SDL_AudioStream* stream, int totalAmount) {
    if (totalAmount <= 0 || bufferSizeBytes_ == 0) {
        return;
    }
    const auto* source = reinterpret_cast<const uint8_t*>(buffer_.data());
    int remaining = totalAmount;
    while (remaining > 0) {
        if (positionBytes_ >= bufferSizeBytes_) {
            positionBytes_ = 0;
        }
        size_t available = bufferSizeBytes_ - positionBytes_;
        if (available == 0) {
            positionBytes_ = 0;
            continue;
        }
        size_t chunk = std::min<size_t>(available, static_cast<size_t>(remaining));
        int queued = SDL_PutAudioStreamData(stream, source + positionBytes_, static_cast<int>(chunk));
        if (queued <= 0) {
            break;
        }
        positionBytes_ += static_cast<size_t>(queued);
        remaining -= queued;
    }
}

} // namespace sdl3cpp::app
