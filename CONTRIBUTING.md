# Contributing to makexx

## Building from source

```bash
cmake -B build
cmake --build build
```

## Project layout

| Path | Purpose |
|------|---------|
| `src/makexx.cpp` | The makexx binary — compiler detection, bootstrapping, orchestration |
| `include/makexxfile.hpp` | The BuildGraph DSL header distributed to users |
| `src/starter.cpp` | The starter `makefile.cpp` written to new project directories |
| `cmake/embed_as_string.cmake` | CMake script that wraps a file's content in a C++ raw string literal |
| `examples/` | Example `makefile.cpp` files showing common patterns |
| `tests/` | Test suite |

## Changing the DSL (`include/makexxfile.hpp`)

Edit the file directly. `cmake --build build` will re-embed it and recompile `makexx` automatically.

## Changing the starter template (`src/starter.cpp`)

Edit the file directly. `cmake --build build` will re-embed it and recompile `makexx` automatically.

## Submitting changes

1. Fork the repository and create a branch from `main`
2. Make your changes and verify the build passes
3. If adding a feature, add or update an example in `examples/`
4. Open a pull request with a clear description of the change
