# SDL3CPlusPlus
A minimal SDL3 + Vulkan spinning cube demo.

## Build

1. Install SDL3 development headers and Vulkan loader on your platform.
2. Configure with CMake:
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
