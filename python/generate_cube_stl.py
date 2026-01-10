#!/usr/bin/env python3
"""Produce a cube STL with CadQuery for the Lua scene to load."""

from __future__ import annotations

import argparse
from pathlib import Path

import cadquery as cq
from cadquery import exporters


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate a simple cube STL.")
    parser.add_argument(
        "--size",
        type=float,
        default=2.0,
        help="Edge length of the cube in model units (default: 2.0 to match Lua cube bounds).",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=Path(__file__).parent / "models" / "cube.stl",
        help="Path to write the ASCII STL file.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    cube = cq.Workplane("XY").box(args.size, args.size, args.size)
    exporters.export(
        cube,
        str(args.output),
        exportType=exporters.ExportTypes.STL,
        opt={"ascii": True},
    )
    print(f"Wrote cube STL to {args.output}")


if __name__ == "__main__":
    main()
