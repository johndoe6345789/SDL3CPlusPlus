#!/usr/bin/env python3
"""Convert the bundled XM tracker file to an OGG so the demo can play music."""

from __future__ import annotations

import argparse
import shlex
import subprocess
from pathlib import Path

import imageio_ffmpeg


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Convert scripts/modmusic.xm into OGG.")
    parser.add_argument(
        "--input",
        type=Path,
        default=Path(__file__).parent / "modmusic.xm",
        help="Tracker file to render (default: scripts/modmusic.xm).",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).parent / "modmusic.ogg",
        help="Path for the rendered OGG (default next to scripts/modmusic.xm).",
    )
    parser.add_argument(
        "--bitrate",
        default="192k",
        help="FFmpeg audio bitrate (default: 192k).",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if not args.input.exists():
        raise SystemExit(f"Error: XM source {args.input} is missing")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    ffmpeg_path = imageio_ffmpeg.get_ffmpeg_exe()

    ffmpeg_cmd = [
        ffmpeg_path,
        "-y",
        "-i",
        str(args.input),
        "-b:a",
        args.bitrate,
        str(args.output),
    ]

    print("Executing:", " ".join(shlex.quote(arg) for arg in ffmpeg_cmd))
    subprocess.run(ffmpeg_cmd, check=True)


if __name__ == "__main__":
    main()
