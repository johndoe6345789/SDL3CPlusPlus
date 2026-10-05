"""Resample Episode I Racer WAVs to 44.1 kHz with linear interpolation.

    python racer_resample.py <game>/data/wavs <out_dir>

Mirrors the wavs/ folder layout under <out_dir>. Stdlib only. Source
files are 16-bit PCM at 11025 or 22050 Hz, mono or stereo.
"""

import array
import os
import sys
import wave

TARGET_RATE = 44100


def resample_channel(samples, source_rate):
    """Linear interpolation of one channel to TARGET_RATE."""
    if not samples:
        return samples
    ratio = source_rate / TARGET_RATE
    out_len = int(len(samples) * TARGET_RATE / source_rate)
    out = array.array("h", bytes(2 * out_len))
    last = len(samples) - 1
    for i in range(out_len):
        pos = i * ratio
        base = int(pos)
        frac = pos - base
        nxt = min(base + 1, last)
        value = samples[base] + (samples[nxt] - samples[base]) * frac
        out[i] = max(-32768, min(32767, int(value)))
    return out


def resample_file(src, dst):
    with wave.open(src, "rb") as reader:
        channels = reader.getnchannels()
        rate = reader.getframerate()
        raw = array.array("h", reader.readframes(reader.getnframes()))
    planes = [resample_channel(raw[c::channels], rate)
              for c in range(channels)]
    frames = len(planes[0])
    interleaved = array.array("h", bytes(2 * frames * channels))
    for c, plane in enumerate(planes):
        interleaved[c::channels] = plane
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with wave.open(dst, "wb") as writer:
        writer.setnchannels(channels)
        writer.setsampwidth(2)
        writer.setframerate(TARGET_RATE)
        writer.writeframes(interleaved.tobytes())


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    src_root, out_root = argv[1], argv[2]
    done = 0
    for folder, _, files in os.walk(src_root):
        for name in files:
            if not name.lower().endswith(".wav"):
                continue
            src = os.path.join(folder, name)
            rel = os.path.relpath(src, src_root)
            resample_file(src, os.path.join(out_root, rel))
            done += 1
    print(f"resampled {done} files")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
