# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

**makexx** is a C++ build system generator. Instead of writing Makefiles by hand, users write `makefile.cpp` (a C++ program using the `Makefile` DSL) and run `makexx` to compile and execute it, producing a standard GNU `makefile`.

## Building makexx itself

```bash
cmake -B build
cmake --build build
cmake --install build   # installs to /usr/local/bin by default
```

Output binary: `build/makexx`

**How the embed works:** `inc/makexxfile.hpp` and `src/makexxfile_example.cpp` are the sources of truth. During the build, `cmake/embed_as_string.cmake` reads each file and wraps its content in a C++ raw string literal, writing `makexxfile_embed.hpp` and `makexxfile_example_embed.hpp` into the build directory. `makexx.cpp` includes these generated headers. Editing either source file and re-running `cmake --build build` regenerates the embeddings and recompiles automatically. No `xxd` required.

## How makexx works at runtime

When `makexx` is invoked in a user's project directory:

1. Extracts `makefile.hpp` from the embedded hex blob (skips if already present, unless `-u`)
2. Creates `makefile.cpp` from the embedded example template (only if it doesn't exist yet)
3. Selects a C++ compiler: uses `CXX` env var if set, otherwise probes `g++`, `clang++`, `icpx`, `icpc` in order
4. Compiles `makefile.cpp` → `makefile_gen` executable
5. Runs `makefile_gen` which calls `mf.generate()` to produce `makefile`, `.makexx_menu`, and `AGENTS.md`
6. Runs `make` with any extra args passed to `makexx` (except `-u`, `-f`, `-c`, `-i`)
7. Cleans up temp files (`tmp_makexx*`, `err_makexx.txt`, `makefile_gen`)

**Makefile protection:** makexx checks whether an existing `makefile` starts with the header `# This is an automatically generated makefile via makexx.`. If not, it refuses to overwrite it unless `-f` is passed.

## CLI flags

| Flag | Effect |
|------|--------|
| `-u` | Force re-extract `makefile.hpp` (update it) |
| `-f` | Force overwrite `makefile` even if not makexx-generated |
| `-c` | Compile only — skip running `make` |
| `-v` | Verbose output |
| `-i` | Interactive target selector (TUI with arrow keys, foldable groups, search) |

All other flags are forwarded to `make`.

## The Makefile DSL (`inc/makexxfile.hpp`)

This is the API that users write in their `makefile.cpp`. Key concepts:

```cpp
Makefile mf;

// Add a rule: mf.add(targets, sources)
auto& rule = mf.add("output.o", "input.cpp");
rule << "g++ -c input.cpp -o output.o";  // shell commands via <<

// target_type enum controls 'all' target membership
rule << FINAL;    // included in 'all'
rule << OPTIONAL; // default, run-on-demand
rule << INPUT;    // source file marker

// Metadata helpers
rule << TEMP({"tmp1", "tmp2"});     // cleaned by full_clean and soft_clean
rule << BYPROD("byproduct.log");    // cleaned by full_clean and soft_clean
rule << TARGET("manual_output");    // hidden/non-reproducible target
rule << HELP("builds the thing");   // shown by 'make help'
rule << HELP("Deploy", "deploy it"); // with explicit group

mf.on_softclean_retain("expensive_output"); // exclude from soft_clean

mf.silent = true;   // prefix commands with @ in makefile
mf.echo = false;    // suppress ### GENERATING echo lines

// Help organization
mf.help_title = "My Project";       // printed at top of 'make help'
mf.HELP_GROUP("Build");             // subsequent HELP() entries belong to this group
mf.HELP_GROUP("Build/Tests");       // nested group via slash separator
mf.HELP_GROUP("Archive", FOLDED);   // folded by default in makexx -i
// HELP("group", "desc") overrides the group for a single rule

// AI agent context generation
mf.description("What this project does..."); // project description for AGENTS.md
mf.context = true;                           // enable AGENTS.md generation (default)
mf.context = false;                          // disable AGENTS.md generation
mf.context_filename = "CLAUDE.md";           // override output filename (default "AGENTS.md")

mf.generate();              // write the makefile + .makexx_menu + AGENTS.md
mf.generate_with_graph();   // write the makefile + .makexx_menu + AGENTS.md + makefile_graph.gv (Graphviz DOT)
```

### Helper functions

```cpp
stem("dir/file.cpp")            // "file" — filename without directory or extension
basename("dir/file.cpp")        // "file.cpp" — filename without directory
change_ext("file.cpp", ".o")    // "file.o" — replace file extension
join_path("obj", "file.o")      // "obj/file.o" — join directory and filename
get_extension("file.cpp")       // "cpp" — file extension without dot
replace_all(str, from, to)      // string replacement
to_upper(str) / to_lower(str)   // case conversion
```

### Generated makefile features

The generated makefile always includes `.PHONY` and the built-in targets: `all`, `full_clean`, `soft_clean`, `list`, `list_unknown`, `list_input`, and `help`. The `help` target shows user-defined targets with box-drawing brackets for multi-target rules, organized by groups, with built-in targets listed at the bottom. Long descriptions word-wrap to the terminal width.

The `### GENERATING` decoration is suppressed for phony/built-in targets.

On Windows, the `<<` operator automatically replaces `/` with `\` in shell commands.

### AI agent context file (AGENTS.md)

`mf.generate()` writes an `AGENTS.md` file alongside the makefile. This gives AI coding agents (Claude Code, Cursor, Copilot, etc.) a plain-English summary of the project: description, input files, targets with dependencies organized by group, and built-in targets.

- `mf.description("...")` sets the project description included in the file
- `mf.context = false` disables generation
- `mf.context_filename = "CLAUDE.md"` overrides the output filename (default `AGENTS.md`)

The file is generated entirely from data the `Makefile` class already holds — no AI is involved in the generation step.

### Interactive mode

`makexx -i` launches a terminal UI for selecting and running targets. It reads the `.makexx_menu` file generated alongside the makefile. POSIX only.

Controls: ↑↓ navigate, PgUp/PgDn jump one page, Home/End jump to top/bottom, Tab/Shift+Tab jump between groups, ←→ fold/unfold groups, Enter to run, `d` dry-run preview (`make -n`), `/` to search, q/Esc to quit. After running a target, a green **Done.** or red **Failed.** indicates the exit status.

**Search:** Press `/` to enter search mode. Type to filter targets by name or description (case-insensitive). Backspace removes characters. Enter locks the filter and returns to normal navigation. Esc clears the filter. Groups with no matching entries are hidden; folded groups auto-expand when a filter is active.

**Viewport scrolling:** When the menu is taller than the terminal, the view scrolls to keep the selected item visible. Long descriptions are word-wrapped to fit the terminal width. The terminal size is re-read each frame so resizing works live.

**Multi-line descriptions:** Rules with multiple `HELP()` calls or descriptions that word-wrap display with box-drawing connectors (┌│└). Combined multi-target + multi-line descriptions use ┬│├ to connect both dimensions.

## Examples

### `examples/compile/makefile.cpp` — Large C++ project build

Shows how to manage a project with many executables and shared object files. Key patterns:

- **Path macros**: `#define O "./obj/"`, `#define S "./src/"`, etc. to keep rule definitions concise
- **`CompileRule` struct**: groups target, dependencies, and extra flags; then batched into `vector<CompileRule>` by category (translators, signal processing, seismic, AI, etc.) and passed to helper functions
- **`addexec()` / `addobj()` helpers**: wrap `mf.add()` + `<<` to reduce repetition and handle platform differences
- **`$<`, `$@`, `$^`** GNU make automatic variables work normally inside command strings
- **Groups**: `mf.get_rule(group)` + `mf.add_source(rule, target)` wires multiple executables as dependencies of a named group target (e.g., `"social"`)
- **Conditional rules**: platform detection inside `makefile.cpp` itself controls which rules are added
- **`xxd -i` embed pattern**: used in user projects to compile binary resources (e.g. header files) into `_xxd.hpp` byte arrays and list them as dependencies so make reruns the embed when the source changes

### `examples/processing_workflow/makefile.cpp` — Domain-specific workflow (exploration analytics)

Shows how `makefile.cpp` can act as a full workflow orchestration script, not just a build script. Key patterns:

- **`#define` feature flags** at the top (`TRIAL`, `SHARED`, `TESTING`, etc.) toggle whole branches of the pipeline — edit the defines and re-run `makexx` to switch modes
- **Domain classes** (`Play`, `Portfolio`, `Venture`, `DrillZone`, `Basin`) hold pipeline parameters; loops over them generate many related rules from a single template
- **`_cont` macro** (`" \\\n"`) for readable multi-line shell commands in string literals
- **SQL/string helpers** return shell command fragments that are composed into rule commands — the full power of C++ string manipulation is available

### `examples/family_tree/makefile.cpp` — Genealogy workflow with AI context

Shows how `makefile.cpp` can drive a non-build workflow (database-backed genealogy visualization) and demonstrates the AI agent context generation feature. Key patterns:

- **`mf.description("...")`** provides a project summary for the generated `AGENTS.md`
- **`HELP_GROUP()`** organizes targets into logical sections (Visualize, Subtrees, Deploy, Utilities)
- **String variables** (`ssh_cmd`, `ssh_usr`, `server`) parameterize deployment commands
- **`mf.generate_with_graph()`** produces the makefile, menu, context file, and dependency graph in one call

## Architecture

```
src/makexx.cpp                — main binary: compiler detection, file bootstrapping, orchestration
include/makexxfile.hpp        — the Makefile DSL header (source of truth, embedded at build time)
src/starter.cpp               — starter makefile.cpp written to new project directories (source of truth, embedded at build time)
CMakeLists.txt                — builds makexx; drives the embed step via cmake/embed_as_string.cmake
cmake/embed_as_string.cmake   — wraps a file's content in a C++ raw string literal for embedding
examples/compile/             — example: multi-target C++ project build
examples/processing_workflow/ — example: domain-specific pipeline orchestration
examples/family_tree/         — example: genealogy workflow with AI context generation
tests/                        — test suite
.github/workflows/ci.yml      — GitHub Actions CI (Linux + macOS)
```

The two embed headers (`makexxfile_embed.hpp`, `makexxfile_example_embed.hpp`) are generated into the CMake build directory and never checked in.

