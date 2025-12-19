#include "app/audio_player.hpp"

#include <SDL3/SDL.h>
#include <vorbis/vorbisfile.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace sdl3cpp::app {

namespace {

struct DecodedAudio {
    std::vector<int16_t> samples;
    int sampleRate = 0;
    int channels = 0;
};

DecodedAudio DecodeOgg(const std::filesystem::path& path) {
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
    int channels = info->channels;
    int rate = static_cast<int>(info->rate);

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

    return DecodedAudio{std::move(decoded), rate, channels};
}

} // namespace

AudioPlayer::AudioPlayer() = default;

AudioPlayer::~AudioPlayer() {
    if (stream_) {
        SDL_PauseAudioStreamDevice(stream_);
        SDL_DestroyAudioStream(stream_);
    }
}

void AudioPlayer::PlayBackground(const std::filesystem::path& path, bool loop) {
    DecodedAudio clip = DecodeOgg(path);
    EnsureStream(clip.sampleRate, clip.channels);
    std::scoped_lock lock(voicesMutex_);
    backgroundVoice_ = AudioVoice{std::move(clip.samples), 0, loop, true};
}

void AudioPlayer::PlayEffect(const std::filesystem::path& path, bool loop) {
    DecodedAudio clip = DecodeOgg(path);
    EnsureStream(clip.sampleRate, clip.channels);
    std::scoped_lock lock(voicesMutex_);
    effectVoices_.push_back(AudioVoice{std::move(clip.samples), 0, loop, true});
}

void AudioPlayer::AudioStreamCallback(void* userdata, SDL_AudioStream* stream, int additionalAmount, int totalAmount) {
    auto* self = static_cast<AudioPlayer*>(userdata);
    self->FeedStream(stream, totalAmount);
}

void AudioPlayer::FeedStream(SDL_AudioStream* stream, int totalAmount) {
    if (totalAmount <= 0 || !stream_) {
        return;
    }
    size_t sampleCount = static_cast<size_t>(totalAmount) / sizeof(int16_t);
    if (sampleCount == 0) {
        return;
    }
    mixBuffer_.assign(sampleCount, 0);

    std::scoped_lock lock(voicesMutex_);
    if (backgroundVoice_ && backgroundVoice_->active) {
        AddVoiceSamples(*backgroundVoice_, mixBuffer_, sampleCount);
        if (!backgroundVoice_->active) {
            backgroundVoice_.reset();
        }
    }

    for (auto it = effectVoices_.begin(); it != effectVoices_.end();) {
        AddVoiceSamples(*it, mixBuffer_, sampleCount);
        if (!it->active) {
            it = effectVoices_.erase(it);
        } else {
            ++it;
        }
    }

    outputBuffer_.resize(sampleCount);
    for (size_t i = 0; i < sampleCount; ++i) {
        int32_t value = mixBuffer_[i];
        if (value > std::numeric_limits<int16_t>::max()) {
            value = std::numeric_limits<int16_t>::max();
        } else if (value < std::numeric_limits<int16_t>::min()) {
            value = std::numeric_limits<int16_t>::min();
        }
        outputBuffer_[i] = static_cast<int16_t>(value);
    }

    SDL_PutAudioStreamData(stream, outputBuffer_.data(), static_cast<int>(sampleCount * sizeof(int16_t)));
}

void AudioPlayer::EnsureStream(int sampleRate, int channels) {
    if (sampleRate <= 0 || channels <= 0) {
        throw std::runtime_error("Audio format is invalid");
    }
    if (sampleRate_ != 0 && (sampleRate != sampleRate_ || channels != channels_)) {
        throw std::runtime_error("Requested audio format does not match initialized stream");
    }
    if (stream_) {
        return;
    }

    SDL_AudioSpec desired{};
    desired.freq = sampleRate;
    desired.format = SDL_AUDIO_S16;
    desired.channels = channels;

    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, &AudioPlayer::AudioStreamCallback, this);
    if (!stream_) {
        throw std::runtime_error("Failed to open audio stream: " + std::string(SDL_GetError()));
    }
    if (!SDL_ResumeAudioStreamDevice(stream_)) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
        throw std::runtime_error("Failed to resume audio stream device: " + std::string(SDL_GetError()));
    }

    sampleRate_ = sampleRate;
    channels_ = channels;
}

void AudioPlayer::AddVoiceSamples(AudioVoice& voice, std::vector<int32_t>& mixBuffer, size_t sampleCount) {
    if (voice.data.empty()) {
        voice.active = false;
        return;
    }
    size_t idx = voice.position;
    for (size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        if (idx >= voice.data.size()) {
            if (voice.loop) {
                idx = 0;
            } else {
                voice.active = false;
                break;
            }
        }
        mixBuffer[sampleIndex] += static_cast<int32_t>(voice.data[idx++]);
    }
    voice.position = idx;
}

} // namespace sdl3cpp::app
