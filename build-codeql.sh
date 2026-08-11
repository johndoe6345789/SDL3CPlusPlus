#!/bin/bash
set -e
source build/build/Release/generators/conanbuild.sh
cmake --preset conan-release -DBUILD_TESTING=OFF
cmake --build build/build/Release --target sdl3_app
