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

**How the embed works:** `include/makexxfile.hpp` and `src/starter.cpp` are the sources of truth. During the build, `cmake/embed_as_string.cmake` reads each file and wraps its content in a C++ raw string literal, writing `makexxfile_embed.hpp` and `starter_embed.hpp` into the build directory. `makexx.cpp` includes these generated headers. Editing either source file and re-running `cmake --build build` regenerates the embeddings and recompiles automatically. No `xxd` required. The same embed step also bakes in the interactive-graph viewer assets under `assets/` (the vendored Cytoscape/dagre/expand-collapse stack and `assets/graph_viewer.html`) so `makexx --build-graph` can assemble a single self-contained HTML graph offline.

## How makexx works at runtime

When `makexx` is invoked in a user's project directory:

1. Extracts `makefile.hpp` from the embedded hex blob (skips if already present, unless `-u`)
2. Creates `makefile.cpp` from the embedded example template (only if it doesn't exist yet)
3. Selects a C++ compiler: uses `CXX` env var if set, otherwise probes `g++`, `clang++`, `icpx`, `icpc` in order
4. Compiles `makefile.cpp` → `.makexx_gen` executable
5. Runs `.makexx_gen` which calls `mf.generate()` to produce `makefile`, `.makexx_menu`, and `AGENTS.md`
6. Runs `make` with any extra args passed to `makexx` (except `-u`, `-f`, `-c`, `-i`)
7. Cleans up scratch files (`.makexx_probe*`, `.makexx_err`, `.makexx_gen`)

**Makefile protection:** makexx checks whether an existing `makefile` starts with the header `# This is an automatically generated makefile via makexx.`. If not, it refuses to overwrite it unless `-f` is passed.

**Auto-regeneration:** The generated makefile includes a rule `makefile: makefile.cpp makefile.hpp` that reruns `makexx -c`. If the user edits `makefile.cpp` and runs `make`, GNU make detects the makefile is out of date, regenerates it, restarts, and then builds the requested targets using the new rules. makexx compiles `makefile.cpp` with `-MMD -MF .makexx_deps -MT makefile`, so any headers included by `makefile.cpp` (e.g., a `config.hpp`) are automatically tracked as dependencies via `-include .makexx_deps` in the generated makefile.

## Generated-file naming convention

Two tiers, and **all new generated files must follow this rule**:

- **Visible artifacts** (the deliverables the user edits, opens, or keeps) are named plainly, no dot prefix: `makefile`, `makefile.cpp`, `makefile.hpp`, `makefile_graph.{gv,pdf,html}`, `AGENTS.md` (name configurable via `mf.context_filename`).
- **Everything makexx writes for its own bookkeeping** — state *and* scratch — is a hidden, namespaced `.makexx_*` dotfile. State: `.makexx_menu`, `.makexx_history`, `.makexx_deps`, `.makexx_hits`, `.makexx_graph.json`, `.makexx_prof/`. Scratch (created/deleted within a run): `.makexx_gen` (compiled generator), `.makexx_probe[.cpp]` (compiler probe), `.makexx_err` (probe stderr), `.makexx_nodes` + `.makexx_lsdir` (`list_unknown` working files).

The `.makexx_*` prefix means plain `ls` hides them, so `list_unknown` ignores them for free. When adding a new generated file, pick the tier and name accordingly — don't introduce a third style.

## CLI flags

| Flag | Effect |
|------|--------|
| `-u` | Force re-extract `makefile.hpp` (update it) |
| `-f` | Force overwrite `makefile` even if not makexx-generated |
| `-c` | Compile only — skip running `make` |
| `-v` | Verbose output |
| `-i` | Interactive target selector (TUI with arrow keys, foldable groups, search) |
| `-Dname=value` | Define a C++ preprocessor macro, forwarded to the compiler when compiling `makefile.cpp` |
| `--build-graph` | Assemble the standalone `makefile_graph.html` from `.makexx_graph.json` and exit (no compile, no `make`). Invoked by the generated `makefile_graph.html` rule; normally you run `make graph` rather than this directly |
| `--stats` | Read the `.makexx_hits` usage/timing log (written when `mf.profile = true`) and print per-rule run counts, last-run, total and median time — sorted by total time (bottleneck first) — plus menu targets with zero recorded runs. Read-only; no compile, no `make` |
| `-h`, `--help` | Show usage help |
| `--version` | Show version |

All other flags are forwarded to `make`.

## The Makefile DSL (`include/makexxfile.hpp`)

This is the API that users write in their `makefile.cpp`. Key concepts:

```cpp
Makefile mf;

// Add a rule: mf.add(targets, sources)
auto& rule = mf.add("output.o", "input.cpp");
rule << "g++ -c input.cpp -o output.o";  // shell commands via <<

// Hold a rule reference and append commands across statements (incl. loops).
// Commands execute in the order they were appended.
auto& list = mf.add("list_deposits");
list << HELP("list deposits") << "echo 'Deposits:'";
for (auto& d : deposits)
    list << ("echo '  " + d.prefix + " - " + d.title + "'");

// target_type enum controls 'all' target membership
rule << FINAL;    // included in 'all'
rule << OPTIONAL; // default, run-on-demand
rule << INPUT;    // source file marker

// Metadata helpers
rule << TEMP({"tmp1", "tmp2"});     // cleaned by full_clean and soft_clean
rule << BYPRODUCT("byproduct.log"); // cleaned by full_clean and soft_clean
rule << TARGET("manual_output");    // hidden/non-reproducible target
rule << TOOL("prog1");               // external executable: mtime-tracked prereq, not in `$^`
                                     // bare name → resolved via $(shell command -v ...);
                                     // path with '/' → used literally. variadic + braced too.
rule << HELP("builds the thing");   // shown by 'make help'
rule << HELP("Deploy", "deploy it"); // with explicit group
rule << HELP("forecast #alpha play"); // #hashtags in HELP/DESC become graph filter tags
                                     //   (target gets the rule's tags; DESC tags a file).
                                     //   #-tag = `#` at a word boundary + [A-Za-z0-9_-]+;
                                     //   stays visible in help text. Used by the graph's
                                     //   seed-filtering (see "Interactive dependency graph").

rule << RETAIN;                          // exclude all of rule's targets from soft_clean
rule << RETAIN("file");                  // selective: also RETAIN("a","b") or RETAIN({"a","b"})
rule << PHONY;                           // all of rule's targets are .PHONY (no output file)
rule << PHONY("install");                // selective: also PHONY("a","b") or PHONY({"a","b"})

// Makefile-level: declare phony / retain by name, independent of any rule
mf << PHONY("install");                  // accepts single, variadic, or braced list
mf << RETAIN("artifact.bin");            // same forms as the per-rule version

mf.silent = true;   // prefix commands with @ in makefile
mf.echo = false;    // suppress ### GENERATING echo lines
mf.profile = true;  // log per-rule run time to .makexx_hits on each run (off by default)
mf.preamble = "CFLAGS ?= -O2\n";    // raw make injected near top of generated makefile

// Help organization
mf.title = "My Project";       // printed at top of 'make help'

// A rule joins a group ONLY via its own << MENU (no sticky/current group).
mf.add("forecast.bin", "data.t")
    << MENU("Forecasting") << HELP("run forecast") << "forecast $< > $@";
mf.add("a.o", "a.cpp") << MENU("Build") << HELP("compile a") << "g++ -c $< -o $@";
mf.add("b.o", "b.cpp") << MENU("Build") << HELP("compile b") << "g++ -c $< -o $@";
mf.add("t.bin", "a.o") << MENU("Build/Tests") << HELP("test"); // nested via slash; parents auto-created
// rule << HELP("group", "desc") also sets the group for that single rule.

// `mf << MENU(...)` is a DECLARATION only — it registers a group's description
// and/or folded state. It does NOT set a current group and does NOT affect any
// rule. (It used to be sticky — every later rule silently inherited it — which
// was an invisible footgun; that inheritance was removed.)
mf << MENU("Processing", "Run the data pipeline");   // group description (help/AGENTS/TUI)
mf << MENU("Processing/QC", "Quality control checks");
mf << MENU("Archive", FOLDED);                       // folded by default in makexx -i
mf << MENU("Archive", "Old runs", FOLDED);           // description + folded
// rules then join with: mf.add(...) << MENU("Processing") / << MENU("Archive") / ...

// AI agent context generation
mf.description = "What this project does..."; // project description for AGENTS.md
mf.context = true;                           // enable AGENTS.md generation (default)
mf.context = false;                          // disable AGENTS.md generation
mf.context_filename = "CLAUDE.md";           // override output filename (default "AGENTS.md")

mf.generate();              // write the makefile + .makexx_menu + AGENTS.md
mf.generate_with_graph();   // also write makefile_graph.gv (Graphviz DOT) + .makexx_graph.json,
                            //   and add a `graph` target. `make graph` assembles a standalone
                            //   interactive makefile_graph.html and opens it; `make makefile_graph.pdf`
                            //   still renders the static Graphviz PDF.
```

### Helper functions

```cpp
stem("dir/file.cpp")                    // "file" — filename without directory or extension
basename("dir/file.cpp")               // "file.cpp" — filename without directory
change_ext("file.cpp", ".o")           // "file.o" — replace file extension
change_ext("file.cpp", {".o", ".d"})   // {"file.o", "file.d"} — same file, multiple extensions
join_path("obj", "file.o")             // "obj/file.o" — join directory and filename
get_ext("file.cpp")                    // "cpp" — file extension without dot
open_file("report.pdf")                // shell snippet that hands the file to whichever
                                        //   opener is available at make time
                                        //   (wslview → xdg-open → open → start)
replace_all(str, from, to)             // string replacement
to_upper(str) / to_lower(str)          // case conversion
```

### Generated makefile features

The generated makefile always includes `.PHONY` and the built-in targets: `all`, `full_clean`, `soft_clean`, `list`, `list_unknown`, `list_input`, and `help`. The `help` target shows user-defined targets with box-drawing brackets for multi-target rules, organized by groups, with parent sections auto-created for nested group paths and built-in targets listed at the bottom. Long descriptions word-wrap to the terminal width.

The `### GENERATING` decoration is suppressed for phony/built-in targets.

On Windows, the `<<` operator automatically replaces `/` with `\` in shell commands.

### Usage / timing data (`mf.profile`)

Setting `mf.profile = true` wraps each non-built-in rule's recipe with lightweight timing instrumentation. On every successful run it appends one tab-separated event to an append-only `.makexx_hits` log:

```
<epoch_seconds>	rule	<target>	<duration_ms>
```

The target column is the literal `$@`, so the log joins cleanly to graph nodes and any future analysis. This is the raw, source-of-truth event stream — **aggregation (counts, last-used, hot/cold, time windows) happens at read time**, not at write time, so every metric stays open. Design notes:

- A millisecond clock is selected once at make parse time into `$(MXX_NOW)` — GNU `date +%s%N`, else `python3`, else whole-second `date +%s000` — so it works on Linux and macOS.
- Each recipe line stays its own shell (no `.ONESHELL`); the wrappers just bracket the recipe via a per-target temp file under `.makexx_prof/`. Make's fail-fast / per-line semantics are unchanged. A **failed** recipe doesn't reach the trailing log line, so only successful runs are recorded.
- Under `make -j`, per-rule wall times are real but **don't sum** to elapsed time — treat the data as a relative ranking, not a budget.
- `.makexx_hits` is **never** removed by `full_clean`/`soft_clean` (it's accumulated history); the `.makexx_prof/` temp dir is cleaned by `full_clean`. Both are gitignored and excluded from `list_unknown`.
- Off by default — it adds two process spawns + a temp file per built target.

`makexx --stats` reads this log and prints a per-rule table — run count, last-run (relative), total time, median — sorted by total time so the bottleneck is on top, plus a "never recorded" list of menu targets with zero runs (review/deletion candidates). It aggregates entirely at read time. Heat-coloring in the interactive graph is the other planned reader of the same raw events.

### AI agent context file (AGENTS.md)

`mf.generate()` writes an `AGENTS.md` file alongside the makefile. This gives AI coding agents (Claude Code, Cursor, Copilot, etc.) a plain-English summary of the project: description, input files, targets with dependencies organized by group, and built-in targets. Nested groups such as `Build/Tests` appear with their parent sections automatically.

- `mf.description = "..."` sets the project description included in the file
- `mf.context = false` disables generation
- `mf.context_filename = "CLAUDE.md"` overrides the output filename (default `AGENTS.md`)

The file is generated entirely from data the `Makefile` class already holds — no AI is involved in the generation step.

### Interactive mode

`makexx -i` launches a terminal UI for selecting and running targets. It reads the `.makexx_menu` file generated alongside the makefile. POSIX only.

Controls: ↑↓ navigate, PgUp/PgDn jump one page, Home/End jump to top/bottom, Tab/Shift+Tab jump between groups, ←→ fold/unfold groups, Enter to run, `d` dry-run preview (`make -n`), `?` show dependencies, `/` to search, Space to multi-select, `x` deselect all, `r` to refresh (rerun `makexx -c` and reload the menu), `q` to quit, Esc to dismiss whatever is active (or to quit when nothing is — requires a second Esc within 2 s). After running a target, a green **Done.** or red **Failed.** indicates the exit status. Ctrl+C / Ctrl+Z / Ctrl+\ are intercepted so they don't terminate the TUI.

**Nested groups:** parent group headers (e.g. `Processing` for a `Processing/QC` rule) are auto-created and rendered as section headers above their children, indented by depth. Folding a parent collapses all of its descendants. Pressing ← on an already-folded group propagates the fold up to its parent.

**Undocumented group:** rules with no `HELP()` (object files, intermediates, glue steps) are collected into a folded **Undocumented** group, listed just above the built-ins. It's folded by default so it doesn't clutter the view; Tab to it and press → to browse, or use `/` search to find a specific undocumented target by name (search surfaces matches even under folded parents). Built-in targets are excluded, each target of a multi-target rule is listed individually, and a `<< DESC(...)` annotation is shown as the description when present. These targets appear only in `makexx -i`, never in `make help`.

**Search:** Press `/` to enter search mode. Type to filter targets by name or description (case-insensitive). Backspace removes characters; **Ctrl+Backspace** clears the whole query while staying in search mode. Enter locks the filter and returns to normal navigation. Esc clears the filter. Matches under folded parent groups still surface — the parent's fold state doesn't hide search results.

**Refresh (`r`):** rerun `makexx -c` (which regenerates the menu file from the current `makefile.cpp`) and reload the menu in place. Cursor (by target name), fold state (by group name — explicit unfolds survive even when the rule declares `FOLDED`), multi-select, and search filter are all preserved across the reload. Compile errors are shown; in-memory state is kept intact so you can fix and try again.

**Viewport scrolling:** When the menu is taller than the terminal, the view scrolls to keep the selected item visible. `▲`/`▼` indicators show when content extends above or below. Long descriptions are word-wrapped to fit the terminal width. The terminal size is re-read each frame so resizing works live.

**Multi-select:** Press Space on entries to toggle selection (marked with `●`). Enter runs all selected targets in sequence. `x` clears all selections. The header shows the selection count.

**Multi-line descriptions:** Rules with multiple `HELP()` calls or descriptions that word-wrap display with box-drawing connectors (┌│└). Combined multi-target + multi-line descriptions use ┬│├ to connect both dimensions.

## Interactive dependency graph

`mf.generate_with_graph()` emits `.makexx_graph.json` (the DAG + per-node `type`/`group`/`HELP`/`DESC`/`tags`/`cmds`); `make graph` assembles a standalone, offline `makefile_graph.html` (Cytoscape.js + dagre, vendored and embedded) and opens it. The static Graphviz `make makefile_graph.pdf` remains available.

The viewer is built around **trace-seeded filtering** — the answer to "the graph is a hairball because the same pipeline is instantiated N times" (e.g. per play / per region). You pick **seeds** and the viewer renders only the connected subgraph:

- **Seeds** = union of a name/pattern box (substring, or glob with `*` — e.g. `*_alpha_*`; **applies on Enter**, not per keystroke, so large graphs don't re-filter mid-typing), `#tag` chips (collected from hashtags in `HELP`/`DESC`), and **double-clicking a node** (toggles it as a seed). Pinned (double-clicked) seeds show as a **cleanable list of chips** (click the `×` to drop one). `/` focuses the seed box. A spinner shows while a filter/layout computes.
- **Connect** = every node on a path *between* two seeds (`(seeds ∪ descendants) ∩ (seeds ∪ ancestors)`), so shared/untagged convergence nodes (e.g. a forecasting step fed by many instances) are pulled in automatically — they don't need a tag.
- **Modifiers** — `↑ inputs` (extend upstream to source files; default on) and `↓ finals` (extend downstream to final targets; default off). A single seed + `↑ inputs` = provenance; a single seed + `↓ finals` = impact/blast-radius.
- **Step-by-step expand** — **single-click a node to select it**; two **`⊕` handles** appear on its sides (left = parents, right = children). Click one to reveal just that one level of immediate predecessors / successors, accumulating, so you can walk the graph hop by hop. (Distinct from `↑inputs`/`↓finals`, which jump all the way to the inputs/finals at once.) Single-click selects (for expanding); double-click seeds — the two are separate.
- **Seeds are outlined**; pulled-in intermediates are not. Hover shows type, `HELP`, `DESC`, `#tags`, and the rule's **commands** (the `cmds` array) so a traced path explains itself.
- **`Filter` toggle** applies/suspends the filter *without* losing the seeds: off = the full graph with your seeds still highlighted (see them in context); on = the isolated connected subgraph. **`Clear`** wipes seeds and resets the toggles. Groups still fold via Fold/Unfold all.

Filtering re-runs the dagre layout on just the visible subset.

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

### `examples/portfolio_analytics/` — Domain-specific workflow with config separation

Shows how `makefile.cpp` can act as a full workflow orchestration script, not just a build script. Key patterns:

- **Config separation** — `config.hpp` holds domain classes (`Deposit`, `Region`, `MiningZone`), feature flags, and parameters; `makefile.cpp` focuses on rules
- **`#define` feature flags** (`TRIAL`, `SHARED`, `WITH_RARE`) toggle whole branches of the pipeline
- **Domain classes** hold pipeline parameters; loops over them generate many related rules from a single template
- **`_cont` macro** (`" \\\n"`) for readable multi-line shell commands in string literals
- **Helper functions** return shell command fragments composed into rule commands
- **per-rule `<< MENU()`** organizes targets into groups (Data, QC, Forecast, Reports, Benchmark, GIS, Utilities)
- **Incremental rule building**: `list_deposits`/`list_zones` hold a rule reference (`auto& list = mf.add(...)`) and append one `echo` command per deposit/zone in a loop

### `examples/family_tree/makefile.cpp` — Genealogy workflow with AI context

Shows how `makefile.cpp` can drive a non-build workflow (database-backed genealogy visualization) and demonstrates the AI agent context generation feature. Key patterns:

- **`mf.description = "..."`** provides a project summary for the generated `AGENTS.md`
- **per-rule `<< MENU()`** organizes targets into logical sections (Visualize, Subtrees, Deploy, Utilities)
- **String variables** (`ssh_cmd`, `ssh_usr`, `server`) parameterize deployment commands
- **Incremental rule building**: the `push` rule is assembled by appending one `rsync` command per `(source, subdir)` pair from a `vector<pair<...>>` — adding a new path to deploy is one line of data, not a new shell line
- **`mf.generate_with_graph()`** produces the makefile, menu, context file, and dependency graph in one call

### `examples/simulation/` — Config separation pattern

Shows how to separate configuration from rules by putting parameters in a `config.hpp` header. Key patterns:

- **`#include "config.hpp"`** keeps data (runs, solver, iterations, trial flag) separate from logic
- **`#define TRIAL`** toggles between trial and production runs
- **Struct-based config** (`vector<Run>`) drives rule generation via loops
- Editing `config.hpp` and running `make` auto-regenerates the makefile (via `-MMD` dependency tracking)

## Architecture

```
src/makexx.cpp                — main binary: compiler detection, file bootstrapping, orchestration
include/makexxfile.hpp        — the Makefile DSL header (source of truth, embedded at build time)
src/starter.cpp               — starter makefile.cpp written to new project directories (source of truth, embedded at build time)
CMakeLists.txt                — builds makexx; drives the embed step via cmake/embed_as_string.cmake
cmake/embed_as_string.cmake   — wraps a file's content in a C++ raw string literal for embedding
assets/graph_viewer.html      — interactive graph viewer template (placeholders filled by `makexx --build-graph`)
assets/vendor/                — vendored Cytoscape.js + dagre + cytoscape-dagre + expand-collapse (embedded, kept offline)
examples/compile/             — example: multi-target C++ project build
examples/portfolio_analytics/ — example: domain-specific workflow with config separation
examples/family_tree/         — example: genealogy workflow with AI context generation
examples/simulation/          — example: config separation with auto-dependency tracking
tests/                        — test suite
.github/workflows/ci.yml      — GitHub Actions CI (Linux + macOS)
```

The two embed headers (`makexxfile_embed.hpp`, `starter_embed.hpp`) are generated into the CMake build directory and never checked in.
