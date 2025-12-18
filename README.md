# SDL3CPlusPlus
A minimal SDL3 + Vulkan spinning cube demo.

## Build

1. Install SDL3 development headers for your platform (e.g., `libsdl3-dev` on Debian/Ubuntu or the SDK from [libsdl.org](https://www.libsdl.org/download-3.html)).
2. Install or update [Conan 2.x](https://docs.conan.io/en/latest/installation.html) and run
   ```
   conan install . --install-folder build --build=missing
   ```
   so that Conan brings in `lua` and the Vulkan loader + headers.
3. Configure the project with CMake using the generated Conan toolchain:
   ```
   cmake -B build -S .
   ```
4. Build the demo:
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
