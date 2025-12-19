# SDL3CPlusPlus
A minimal SDL3 + Vulkan spinning cube demo.

## Build

1. Install or update [Conan 2.x](https://docs.conan.io/en/latest/installation.html) and run
   ```
   conan profile detect
   conan install . -of build -b missing -s compiler=msvc -s compiler.version=194 -s compiler.cppstd=17 -c tools.cmake.cmaketoolchain:generator="Visual Studio 17 2022"
   ```
   so that Conan brings in `lua`, SDL3, and the Vulkan loader + headers.
2. Configure the project with CMake using the generated Conan toolchain:
   ```
   cmake -B build -S .
   ```
3. Build the demo:
   ```
   cmake --build build
   ```

Shaders are copied into `build/shaders` during configuration, so the demo can load the precompiled `cube.{vert,frag}.spv`.

## Run

```
cmake --build build --target spinning_cube
./build/spinning_cube
```

If you need the Conan runtime environment (e.g., because dependencies set env vars), source `build/conanrun.sh` before launching the binary on Linux/macOS or run `build\\conanrun.bat` on Windows.

## Dependency automation

This project ships a `renovate.json` configuration so Renovate can open PRs that bump the Conan packages listed in `conanfile.py`. Either install Renovate locally (`npm install -g renovate` or `npx renovate`) and run it from the repo root, or enable the Renovate bot on your GitHub/GitLab install to pick up the configuration automatically.
