#!/usr/bin/env bash
# Prints the content tag of the CI build image: a digest of everything that
# goes into it. publish-container.yml labels the image with this, build.yml
# asks for it by name, so a job can only ever run inside an image baked from
# the recipe sitting next to it - a stale image is a pull failure rather than
# a puzzling error an hour into the build.
#
# Keep this list in step with what .github/workflows/Dockerfile COPYs and
# installs; anything that changes the image's contents belongs here.
set -euo pipefail

inputs=(
    conanfile.py
    .github/workflows/Dockerfile
)

cd "$(dirname "${BASH_SOURCE[0]}")/.."

# Carriage returns are stripped so a checkout that normalised line endings
# still hashes to the tag CI published.
for input in "${inputs[@]}"; do
    printf '%s\n' "$input"
    tr -d '\r' < "$input"
done | sha256sum | cut -c1-16
