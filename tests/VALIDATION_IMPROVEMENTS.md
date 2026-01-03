# Validation and Error Handling Improvements

## Overview
This document describes the comprehensive validation and error handling improvements made to the SDL3CPlusPlus application to catch errors early and provide better user feedback before crashes occur.

## Problem
The application was crashing with exit code 137 (SIGKILL), showing only a see-through window with no feedback on what went wrong. This made it difficult to diagnose issues.

## Solution
Added extensive validation at every critical initialization stage with clear, actionable error messages.

---

## Changes Made

### 1. **Early Vulkan Validation** ([sdl3_app_device.cpp](src/app/sdl3_app_device.cpp))

#### Before:
- Directly attempted to create Vulkan instance without checking if Vulkan is available
- Generic error messages

#### After:
- **Version Check**: Validates Vulkan is installed and checks API version before proceeding
  ```cpp
  VkResult enumResult = vkEnumerateInstanceVersion(&apiVersion);
  if (enumResult != VK_SUCCESS) {
      // Provides OS-specific installation instructions
  }
  ```
- **Version Requirements**: Verifies Vulkan 1.2 or higher is available
- **Console Output**: Displays detected Vulkan version
  ```
  Vulkan Version: 1.3.xxx
  ```
- **Detailed Error Messages**: Includes OS-specific installation commands for Ubuntu/Debian, Fedora, Arch
- **GPU-specific Guidance**: Separate instructions for NVIDIA and AMD GPUs

### 2. **GPU Detection and Selection** ([sdl3_app_device.cpp](src/app/sdl3_app_device.cpp))

#### Before:
- Limited information about available GPUs
- Simple unsuitable device messages

#### After:
- **Comprehensive GPU Enumeration**: Lists all detected GPUs with details
  ```
  === GPU Detection ===
  GPU 0: NVIDIA GeForce RTX 3080 (VRAM: 10240 MB)
    -> SELECTED
  GPU 1: Intel UHD Graphics (VRAM: 2048 MB)
    -> UNSUITABLE (missing required features)
  ==================
  ```
- **Memory Information**: Shows VRAM for each GPU
- **Error Result Codes**: Returns Vulkan error codes for debugging
- **Clear Selection Feedback**: Shows which GPU was selected and why others were rejected
- **Detailed Requirements**: Lists specific missing features:
  - Graphics queue support
  - Present queue support  
  - Swapchain extension support
  - Adequate swapchain formats and present modes

### 3. **Swap Chain Validation** ([sdl3_app_swapchain.cpp](src/app/sdl3_app_swapchain.cpp))

#### Before:
- Assumed swap chain support was available
- No validation of window dimensions

#### After:
- **Surface Format Validation**: Checks if surface formats are available
  ```cpp
  if (support.formats.empty()) {
      throw std::runtime_error("No surface formats available...");
  }
  ```
- **Present Mode Validation**: Verifies present modes exist
- **Window Dimension Check**: Validates window is not minimized or has invalid size
  ```cpp
  if (windowWidth == 0 || windowHeight == 0) {
      throw std::runtime_error("Invalid window dimensions...");
  }
  ```
- **Capability Reporting**: Displays surface capabilities:
  ```
  Window size: 1024x768
  Surface capabilities:
    Min extent: 1x1
    Max extent: 4096x4096
    Min images: 2
    Max images: 8
  ```

### 4. **Memory Allocation Validation** ([vulkan_api.cpp](src/app/vulkan_api.cpp))

#### Before:
- Created buffers without checking available memory
- Generic "failed to allocate" errors

#### After:
- **Size Validation**: Checks buffer size is non-zero
- **Available Memory Check**: Compares requested size against total GPU memory
  ```cpp
  if (size > totalAvailable) {
      throw std::runtime_error("Requested buffer size exceeds available GPU memory");
  }
  ```
- **Error Code Reporting**: Returns specific Vulkan error codes
- **Detailed Error Messages**: Provides size information and troubleshooting steps
  - For `VK_ERROR_OUT_OF_DEVICE_MEMORY`: 
    - Close GPU-intensive applications
    - Reduce window resolution
    - Upgrade GPU or system memory
  - For `VK_ERROR_OUT_OF_HOST_MEMORY`:
    - Close other applications
    - Add more RAM
- **Cleanup on Failure**: Properly destroys partially created resources
  ```cpp
  if (allocResult != VK_SUCCESS) {
      vkDestroyBuffer(device, buffer, nullptr);
      // throw detailed error...
  }
  ```

### 5. **Buffer Creation Validation** ([sdl3_app_buffers.cpp](src/app/sdl3_app_buffers.cpp))

#### Before:
- Attempted to create buffers even if data was empty

#### After:
- **Data Validation**: Checks vertices/indices are loaded before buffer creation
  ```cpp
  if (vertices_.empty()) {
      throw std::runtime_error("Cannot create vertex buffer: no vertices loaded");
  }
  ```
- **Resource Reporting**: Displays buffer sizes being allocated
  ```
  Creating vertex buffer: 24 vertices (1 KB)
  Creating index buffer: 36 indices (0 KB)
  ```
- **Immediate Feedback**: Uses `std::cout.flush()` to ensure messages appear immediately

### 6. **Enhanced Error Dialog Support**

All errors now:
- Show in console with detailed information
- Display in SDL message boxes (when available)
- Include actionable troubleshooting steps
- Reference specific error codes for debugging

---

## Benefits

1. **Early Detection**: Problems are caught before they cause crashes
2. **Clear Feedback**: Users see exactly what went wrong and where
3. **Actionable Guidance**: Error messages include specific steps to resolve issues
4. **Better Diagnostics**: Includes error codes, sizes, and system capabilities
5. **Progress Visibility**: Console output shows what the app is doing during initialization
6. **Resource Awareness**: Validates resources before attempting allocation

---

## Testing

To test the improvements:

```bash
cd /home/rewrich/Documents/GitHub/SDL3CPlusPlus/build/Release
make -j$(nproc)
./sdl3_app --json-file-in ./config/seed_runtime.json
```

You should now see:
- Vulkan version detection
- GPU enumeration with memory information
- Window and swap chain validation
- Buffer creation progress
- Clear error messages if any step fails

---

## Example Error Messages

### Missing Vulkan Drivers
```
ERROR: Vulkan is not available on this system.

Please install Vulkan drivers:
- Ubuntu/Debian: sudo apt install vulkan-tools libvulkan1
- Fedora: sudo dnf install vulkan-tools vulkan-loader
- Arch: sudo pacman -S vulkan-tools vulkan-icd-loader

For NVIDIA GPUs, install: nvidia-vulkan-icd
For AMD GPUs, install: mesa-vulkan-drivers
```

### No Suitable GPU
```
ERROR: Failed to find a suitable GPU.

Found 2 GPU(s), but none meet the requirements.
GPU 0: Intel UHD Graphics (VRAM: 2048 MB) [UNSUITABLE]
GPU 1: AMD Radeon (VRAM: 4096 MB) [UNSUITABLE]

Required features:
- Graphics queue support
- Present queue support
- Swapchain extension support (VK_KHR_swapchain)
- Adequate swapchain formats and present modes
```

### Out of Memory
```
ERROR: Failed to allocate buffer memory.
Requested: 512 MB
Error code: -2

Out of GPU memory. Try:
- Closing other GPU-intensive applications
- Reducing window resolution
- Upgrading GPU or system memory
```

---

## Files Modified

1. [src/app/sdl3_app_device.cpp](src/app/sdl3_app_device.cpp) - Vulkan and GPU validation
2. [src/app/sdl3_app_swapchain.cpp](src/app/sdl3_app_swapchain.cpp) - Swap chain validation
3. [src/app/vulkan_api.cpp](src/app/vulkan_api.cpp) - Memory allocation validation
4. [src/app/sdl3_app_buffers.cpp](src/app/sdl3_app_buffers.cpp) - Buffer creation validation

---

## Future Improvements

Consider adding:
- Memory usage tracking throughout app lifecycle
- Swap chain recreation validation
- Shader loading validation with detailed error messages
- Configuration file validation before initialization
- Performance monitoring and warnings for low-memory conditions
