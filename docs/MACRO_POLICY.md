# Macro Policy

## Philosophy

This codebase enforces a **"no macros"** culture both socially and mechanically. Macros are banned except in strictly whitelisted headers. This is how large, sane codebases survive.

## Why Ban Macros?

1. **Type safety**: Macros don't respect C++ types and can cause subtle bugs
2. **Debuggability**: Macros are preprocessor text substitution, making debugging difficult
3. **Scope pollution**: Macros don't respect namespaces and can collide globally
4. **IDE support**: Tools struggle with macros (no refactoring, poor code navigation)
5. **Maintainability**: Modern C++ has better alternatives (constexpr, templates, inline functions)

## Enforcement Mechanisms

### 1. Compiler Warnings

The build system enables strict warnings:

- **Clang/GCC**: `-Wmacro-redefined`, `-Wreserved-id-macro`
- **MSVC**: `/we4005` (macro redefinition as error)

These catch accidental macro redefinitions and reserved identifier usage.

### 2. Static Analysis (clang-tidy)

`cppcoreguidelines-macro-usage` check is enabled with:

- Only CAPS_CASE macros trigger warnings (CheckCapsOnly)
- Whitelisted pattern: `^SDL3CPP_.*_HPP$|^SDL_MAIN_HANDLED$`

This allows include guards and necessary SDK configuration while rejecting everything else.

### 3. CI Enforcement

The `scripts/check_macros.sh` script runs in CI and rejects any `#define` outside the whitelist.

**Current whitelist (include guards and SDK config only):**
- All `.hpp` header files (for include guards matching `SDL3CPP_*_HPP`)
- `src/app/sdl_macros.hpp` (for `SDL_MAIN_HANDLED` configuration)

### 4. Pre-commit Hook (Optional)

Run the macro check locally before committing:

```bash
./scripts/check_macros.sh
```

## What to Use Instead

| ❌ Don't Use | ✅ Use Instead |
|-------------|---------------|
| `#define PI 3.14159` | `constexpr double pi = 3.14159;` |
| `#define MAX(a,b) ((a)>(b)?(a):(b))` | `template<typename T> constexpr T max(T a, T b) { return a > b ? a : b; }` or `std::max()` |
| `#define INLINE` | `inline` keyword |
| `#define DEBUG_LOG(x) ...` | `inline void debug_log(...)` with `if constexpr` |
| `#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))` | `template<typename T, size_t N> constexpr size_t array_size(T(&)[N]) { return N; }` or `std::size()` |

## Exceptions (Whitelisted)

The only allowed macros are:

1. **Include guards**: `SDL3CPP_*_HPP` pattern
2. **SDL configuration**: `SDL_MAIN_HANDLED` (required by SDL3)

If you believe you need a macro:
1. First, try `constexpr`, `inline`, or templates
2. If absolutely necessary, propose adding it to the whitelist with justification
3. Update `scripts/check_macros.sh` whitelist
4. Update `.clang-tidy` AllowedRegexp pattern

## Running Checks Locally

```bash
# Check macro policy
./scripts/check_macros.sh

# Run clang-tidy
cmake -B build -DENABLE_CLANG_TIDY=ON
cmake --build build

# Build with warnings
cmake -B build
cmake --build build
```

## CI Integration

The CI pipeline automatically:
1. Runs `scripts/check_macros.sh` on every build
2. Enables clang-tidy checks (when configured)
3. Compiles with strict macro-related warnings

**Build will fail if:**
- New `#define` appears in non-whitelisted files
- Macros are redefined
- Reserved identifier macros are used

## References

- [C++ Core Guidelines: ES.31 - Don't use macros for constants or "functions"](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#es31-dont-use-macros-for-constants-or-functions)
- [Google C++ Style Guide: Preprocessor Macros](https://google.github.io/styleguide/cppguide.html#Preprocessor_Macros)
