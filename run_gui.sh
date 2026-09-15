#!/usr/bin/env bash
###############################################################################
# Launch the point-and-click launcher (PyQt6).
#
# Works both in a source checkout and in an unpacked release ZIP: both keep
# python/ and the packages/ tree next to this script, which is where the GUI
# looks for them.
#
# Usage:
#   ./run_gui.sh                 # start the launcher
#   PYTHON=python3.12 ./run_gui.sh
#   SDL3_GUI_NO_INSTALL=1 ./run_gui.sh   # never create a venv or pip install
###############################################################################

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAUNCHER="$ROOT/python/dev_commands.py"
REQUIREMENTS="$ROOT/python/requirements-gui.txt"
VENV="$ROOT/.venv-gui"

die() {
  echo "error: $*" >&2
  exit 1
}

[ -f "$LAUNCHER" ] || die "missing $LAUNCHER"

# Virtualenvs put the interpreter in bin/ everywhere except Windows, where
# it lands in Scripts/ - this script is also run from Git Bash and MSYS.
venv_python() {
  if [ -x "$VENV/bin/python" ]; then
    echo "$VENV/bin/python"
  elif [ -x "$VENV/Scripts/python.exe" ]; then
    echo "$VENV/Scripts/python.exe"
  fi
}

has_pyqt6() {
  "$1" -c "import PyQt6.QtWidgets" >/dev/null 2>&1
}

find_python() {
  for candidate in "${PYTHON:-}" python3 python; do
    [ -n "$candidate" ] || continue
    if command -v "$candidate" >/dev/null 2>&1; then
      echo "$candidate"
      return 0
    fi
  done
  return 1
}

PY="$(find_python)" || die "no python3 on PATH; set PYTHON=/path/to/python"

# Prefer whatever the developer already has. Only fall back to a private
# venv when the system interpreter cannot import PyQt6, which is the usual
# case on a fresh machine and on distributions with an externally managed
# Python that refuses a plain `pip install`.
if ! has_pyqt6 "$PY"; then
  existing="$(venv_python)"
  if [ -n "$existing" ] && has_pyqt6 "$existing"; then
    PY="$existing"
  elif [ "${SDL3_GUI_NO_INSTALL:-}" = "1" ]; then
    die "PyQt6 is not installed and SDL3_GUI_NO_INSTALL=1; run: $PY -m pip install PyQt6"
  else
    echo "PyQt6 not found; setting up $VENV ..."
    "$PY" -m venv "$VENV" || die "could not create $VENV (is python3-venv installed?)"
    PY="$(venv_python)"
    [ -n "$PY" ] || die "venv at $VENV has no interpreter"
    "$PY" -m pip install --upgrade pip >/dev/null
    if [ -f "$REQUIREMENTS" ]; then
      "$PY" -m pip install -r "$REQUIREMENTS"
    else
      "$PY" -m pip install PyQt6
    fi
    has_pyqt6 "$PY" || die "PyQt6 still not importable after install"
  fi
fi

exec "$PY" "$LAUNCHER" gui "$@"
