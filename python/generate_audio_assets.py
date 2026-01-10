#!/usr/bin/env python3
"""Create the demo's shared `.ogg` audio library via ``pedalboard`` + ``soundfile``."""

from __future__ import annotations

import argparse
import logging
import shutil
from pathlib import Path
from typing import Optional

logger = logging.getLogger(__name__)

try:
    import numpy as np
    import soundfile as sf
    from pedalboard import Chorus, Delay, Distortion, Pedalboard, Reverb
except ImportError as exc:  # pragma: no cover - requires extra dependencies
    raise SystemExit(
        "Missing pedalboard or soundfile. Install them with "
        "`python -m pip install pedalboard soundfile numpy`. "
        f"ImportError: {exc}"
    ) from exc

try:
    from piper import PiperVoice
except ImportError:
    PiperVoice = None  # type: ignore[assignment]

try:
    from piper.download_voices import download_voice
except ImportError:
    download_voice = None  # type: ignore[assignment]

SAMPLE_RATE = 44100

TTS_PHRASES = [
    "Level 1",
    "Level 2",
    "Game Over",
    "Continue",
    "Power Up",
]

DEFAULT_PIPER_VOICE_NAME = "en_US-lessac-medium"
DEFAULT_PIPER_VOICE_DIR = (
    Path(__file__).resolve().parent / "assets" / "audio" / "tts" / "voices"
)


def _num_samples(duration: float) -> int:
    return max(1, int(round(duration * SAMPLE_RATE)))


def _apply_envelope(
    signal: np.ndarray,
    attack: float,
    decay: float,
    sustain_level: float,
    release: float,
) -> np.ndarray:
    total = signal.shape[0]
    attack_samples = min(total, int(round(attack * SAMPLE_RATE)))
    decay_samples = min(total - attack_samples, int(round(decay * SAMPLE_RATE)))
    release_samples = min(
        total - attack_samples - decay_samples, int(round(release * SAMPLE_RATE))
    )
    sustain_samples = total - (attack_samples + decay_samples + release_samples)
    env = np.empty(total, dtype=np.float32)
    idx = 0
    if attack_samples > 0:
        env[idx : idx + attack_samples] = np.linspace(0.0, 1.0, attack_samples, False)
        idx += attack_samples
    if decay_samples > 0:
        env[idx : idx + decay_samples] = np.linspace(
            1.0, sustain_level, decay_samples, False
        )
        idx += decay_samples
    if sustain_samples > 0:
        env[idx : idx + sustain_samples] = sustain_level
        idx += sustain_samples
    if release_samples > 0:
        env[-release_samples:] = np.linspace(
            sustain_level, 0.0, release_samples, False
        )
        if idx + sustain_samples < total - release_samples:
            env[idx + sustain_samples : -release_samples] = sustain_level
    if attack_samples + decay_samples + sustain_samples + release_samples == 0:
        env[:] = 1.0
    return signal * env


def _download_piper_voice(voice_name: str, download_dir: Path) -> None:
    if download_voice is None:
        logger.warning(
            "Automatic voice download requires `piper.download_voices`; install piper-tts to enable it."
        )
        return

    download_dir.mkdir(parents=True, exist_ok=True)
    logger.info("Downloading Piper voice %s into %s", voice_name, download_dir)
    download_voice(voice_name, download_dir)


def _sine_glide(duration: float, start_freq: float, end_freq: float) -> np.ndarray:
    samples = _num_samples(duration)
    t = np.linspace(0.0, duration, samples, False)
    freq = np.linspace(start_freq, end_freq, samples)
    return np.sin(2 * np.pi * freq * t).astype(np.float32)


def _pink_noise(duration: float) -> np.ndarray:
    samples = _num_samples(duration)
    return np.random.normal(scale=0.15, size=samples).astype(np.float32)


def _menu_select(duration: float) -> tuple[np.ndarray, list]:
    signal = _sine_glide(duration, 520, 960)
    signal = _apply_envelope(signal, 0.01, 0.12, 0.6, 0.15)
    effects = [Chorus(rate_hz=1.1, depth=0.18, mix=0.6)]
    return signal, effects


def _power_up(duration: float) -> tuple[np.ndarray, list]:
    signal = _sine_glide(duration, 270, 940)
    signal += 0.25 * _pink_noise(duration)
    signal = _apply_envelope(signal, 0.02, 0.26, 0.45, 0.2)
    effects = [
        Distortion(drive_db=14.0),
        Reverb(room_size=0.45, wet_level=0.25, dry_level=0.9),
    ]
    return signal, effects


def _level_up(duration: float) -> tuple[np.ndarray, list]:
    base = _sine_glide(duration, 420, 660)
    harmony = 0.45 * _sine_glide(duration, 660, 840)
    signal = (base + harmony) / 1.45
    signal = _apply_envelope(signal, 0.01, 0.18, 0.55, 0.25)
    effects = [
        Chorus(rate_hz=0.95, depth=0.24, mix=0.55),
        Delay(delay_seconds=0.18, feedback=0.25, mix=0.35),
    ]
    return signal, effects


def _swish(duration: float) -> tuple[np.ndarray, list]:
    signal = _pink_noise(duration)
    signal = _apply_envelope(signal, 0.1, 0.3, 0.2, 0.3)
    effects = [
        Reverb(room_size=0.7, wet_level=0.5, dry_level=0.4),
        Chorus(rate_hz=0.4, depth=0.2, mix=0.45),
    ]
    return signal, effects


def _thud(duration: float) -> tuple[np.ndarray, list]:
    base = _sine_glide(duration, 80, 120)
    signal = 0.7 * base + 0.3 * _pink_noise(duration)
    signal = _apply_envelope(signal, 0.01, duration * 0.4, 0.0, 0.3)
    effects = [
        Distortion(drive_db=10.0),
        Reverb(room_size=0.85, wet_level=0.55, dry_level=0.3),
    ]
    return signal, effects


SFX_DEFINITIONS = [
    ("menu_select", 0.65, "short ascending ping", _menu_select),
    ("power_up", 1.1, "riser with harmonic shimmer", _power_up),
    ("level_up", 0.9, "bright burst", _level_up),
    ("swish", 0.7, "noisy transition", _swish),
    ("thud", 1.0, "low impact", _thud),
]


def _render_ogg(path: Path, duration: float, builder):
    signal, effects = builder(duration)
    processed = signal
    if effects:
        board = Pedalboard(effects)
        processed = board(processed, SAMPLE_RATE)
    processed = np.clip(processed, -1.0, 1.0).astype(np.float32)
    sf.write(
        str(path),
        processed,
        SAMPLE_RATE,
        format="OGG",
        subtype="VORBIS",
    )


def _slugify(text: str) -> str:
    return "".join(ch if ch.isalnum() else "_" for ch in text).strip("_").lower()


def _configure_logging(verbose: bool) -> None:
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(level=level, format="%(levelname)s: %(message)s")


def _generate_sfx(output_dir: Path, force: bool):
    target_dir = output_dir / "sfx"
    if force and target_dir.exists():
        logger.debug("Removing existing SFX folder %s", target_dir)
        shutil.rmtree(target_dir)
    target_dir.mkdir(parents=True, exist_ok=True)

    for name, duration, description, builder in SFX_DEFINITIONS:
        target = target_dir / f"{name}.ogg"
        if target.exists() and not force:
            logger.info(f"Skipping existing sound: {target.name} ({description})")
            continue
        logger.info(f"Rendering SFX: {name} -> {target.name}")
        _render_ogg(target, duration, builder)


def _load_piper_voice(model_path: Path, config_path: Optional[Path]) -> Optional["PiperVoice"]:
    if PiperVoice is None:
        logger.warning("piper-tts is not installed; skipping voice generation.")
        return None

    if not model_path.exists():
        logger.warning("Piper voice model not found at %s; skip TTS.", model_path)
        return None

    resolved_config = config_path or Path(f"{model_path}.json")
    if not resolved_config.exists():
        logger.warning(
            "Piper voice config not found at %s; skip TTS generation.", resolved_config
        )
        return None

    return PiperVoice.load(str(model_path), config_path=str(resolved_config))


def _synthesize_phrase_to_ogg(voice: "PiperVoice", phrase: str, path: Path) -> None:
    chunks = list(voice.synthesize(phrase))
    if not chunks:
        logger.warning("Piper generated no audio for phrase '%s'", phrase)
        return

    audio = np.concatenate([chunk.audio_float_array for chunk in chunks])
    audio = np.clip(audio, -1.0, 1.0).astype(np.float32)
    sf.write(
        str(path),
        audio,
        voice.config.sample_rate,
        format="OGG",
        subtype="VORBIS",
    )


def _generate_tts(
    output_dir: Path,
    force: bool,
    voice_model: Path,
    voice_config: Optional[Path],
) -> None:
    voice = _load_piper_voice(voice_model, voice_config)
    if voice is None:
        return

    tts_dir = output_dir / "tts"
    if force and tts_dir.exists():
        logger.debug("Removing existing TTS folder %s", tts_dir)
        shutil.rmtree(tts_dir)
    tts_dir.mkdir(parents=True, exist_ok=True)

    for phrase in TTS_PHRASES:
        slug = _slugify(phrase)
        target = tts_dir / f"{slug}.ogg"
        if target.exists() and not force:
            logger.info(f"Skipping existing voice: {target.name} ({phrase})")
            continue
        logger.info(f"Rendering voice: {phrase} -> {target.name}")
        _synthesize_phrase_to_ogg(voice, phrase, target)


def main():
    parser = argparse.ArgumentParser(
        description="Regenerate the OGG sound library for the demo."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "assets" / "audio",
        help="Where to store generated OGG files.",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Rebuild every asset even if a file already exists.",
    )
    parser.add_argument(
        "--skip-tts",
        action="store_true",
        help="Do not regenerate the text-to-speech phrases.",
    )
    parser.add_argument(
        "--skip-sfx",
        action="store_true",
        help="Do not regenerate the procedural sound effects.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable debug logging while generating audio assets.",
    )
    parser.add_argument(
        "--piper-voice",
        default=DEFAULT_PIPER_VOICE_NAME,
        help="Piper voice identifier like 'en_US-lessac-medium'.",
    )
    parser.add_argument(
        "--piper-voice-model",
        type=Path,
        help="Path to the Piper ONNX voice model (defaults to <download-dir>/<voice>.onnx).",
    )
    parser.add_argument(
        "--piper-voice-config",
        type=Path,
        help="Path to the Piper voice config JSON (defaults to <model>.json).",
    )
    parser.add_argument(
        "--download-voice",
        action="store_true",
        help="Automatically download the Piper voice before generating TTS.",
    )

    args = parser.parse_args()
    _configure_logging(args.verbose)
    logger.debug("Output directory: %s", args.output_dir)
    voice_model_directory = (
        args.piper_voice_model.parent if args.piper_voice_model else DEFAULT_PIPER_VOICE_DIR
    )
    voice_model = args.piper_voice_model or (
        voice_model_directory / f"{args.piper_voice}.onnx"
    )
    voice_config = args.piper_voice_config
    voice_model.parent.mkdir(parents=True, exist_ok=True)
    args.output_dir.mkdir(parents=True, exist_ok=True)

    if args.download_voice:
        _download_piper_voice(args.piper_voice, voice_model.parent)

    if not args.skip_sfx:
        _generate_sfx(args.output_dir, args.force)
    if not args.skip_tts:
        _generate_tts(
            args.output_dir,
            args.force,
            voice_model,
            voice_config,
        )


if __name__ == "__main__":
    main()
