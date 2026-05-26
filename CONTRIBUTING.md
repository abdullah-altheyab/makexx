# Contributing to makexx

## Building from source

```bash
cmake -B build
cmake --build build
```

## Running tests

```bash
cd build && ctest --output-on-failure
```

Or run the test binary directly for detailed output:

```bash
./build/test_dsl
```

CI runs on every push to `main` and on pull requests (Linux + macOS).

## Project layout

| Path | Purpose |
|------|---------|
| `src/makexx.cpp` | The makexx binary — compiler detection, bootstrapping, orchestration |
| `include/makexxfile.hpp` | The Makefile DSL header distributed to users |
| `src/starter.cpp` | The starter `makefile.cpp` written to new project directories |
| `cmake/embed_as_string.cmake` | CMake script that wraps a file's content in a C++ raw string literal |
| `examples/` | Example `makefile.cpp` files showing common patterns |
| `tests/test_dsl.cpp` | Test suite for the DSL |
| `.github/workflows/ci.yml` | CI workflow (build + test on Linux and macOS) |
| `.github/workflows/release.yml` | Release workflow (builds binaries + .deb on version tags) |

## How the embed works

`include/makexxfile.hpp` and `src/starter.cpp` are the sources of truth. During the build, `cmake/embed_as_string.cmake` wraps each file's content in a C++ raw string literal. Editing either file and re-running `cmake --build build` regenerates the embeddings automatically.

## Changing the DSL (`include/makexxfile.hpp`)

Edit the file directly. Rebuild and run `./build/test_dsl` to verify. If adding a new feature, add a test case in `tests/test_dsl.cpp`.

## Changing the starter template (`src/starter.cpp`)

Edit the file directly. Rebuild, then test by running `makexx` in an empty directory to verify the generated template.

## Submitting changes

1. Fork the repository and create a branch from `main`
2. Make your changes and verify `ctest` passes
3. If adding a DSL feature, add a test and update an example in `examples/`
4. Update `CLAUDE.md` and `README.md` if the change affects the public API
5. Open a pull request with a clear description of the change
