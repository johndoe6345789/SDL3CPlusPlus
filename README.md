# SDL3CPlusPlus
A minimal SDL3 + Vulkan spinning cube demo.

## Build

1. Install or update [Conan 2.x](https://docs.conan.io/en/latest/installation.html) and run
   ```
   conan install . --install-folder build --build=missing
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
