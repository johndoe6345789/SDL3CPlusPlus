# AGENTS.md

## Code Style
- Use Java / Spring conventions: explicit names, clear structure, and standard patterns.
- Prefer readability over cleverness.
- Avoid complex template tricks unless required for correctness.

## C++ Guidelines
- Minimise use of macros; only use them when there is no clear alternative.
- Prefer `constexpr`, `inline`, and scoped enums over macros.
- Keep APIs explicit and predictable; avoid implicit conversions.

## Logging
- When fixing a bug, add trace logging in the affected area to improve diagnostics.
