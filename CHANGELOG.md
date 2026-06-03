# Changelog

## Unreleased

### Usage / timing data collection

- **New `mf.profile = true`** instruments each non-built-in rule's recipe to record how long it takes, appending a raw tab-separated event — `epoch · rule · target · duration_ms` — to an append-only **`.makexx_hits`** log on every successful run. This is the data-collection foundation for makefile maintenance/utilization questions ("when did I last build this", "how often", "which step is the bottleneck", "what's safe to delete"). The log keeps **raw events keyed by the literal `$@`** so aggregation (counts, last-used, hot/cold, time windows) happens at read time and every metric stays open; it's designed to be consumed by a future `makexx --stats` and by heat-coloring in the interactive graph. A millisecond clock is selected once at parse time into `$(MXX_NOW)` (GNU `date +%s%N` → `python3` → whole-second fallback) so it works on Linux and macOS. Each recipe line stays its own shell (no `.ONESHELL`), so make's semantics are unchanged; failed recipes aren't logged, and under `make -j` per-rule times are a relative ranking (they don't sum to wall time). `.makexx_hits` survives `clean` (it's history); the `.makexx_prof/` temp dir is cleaned. Off by default (adds two process spawns + a temp file per built target)

### Interactive dependency graph

- **New: standalone, interactive dependency graph in the browser.** `mf.generate_with_graph()` now also emits `.makexx_graph.json` (the dependency DAG with per-node type / group / `HELP` / `DESC` / `TOOL` metadata), and the generated makefile gains a `graph` target. `make graph` assembles a **single self-contained `makefile_graph.html`** — the Cytoscape.js + dagre stack and the data are inlined, so it needs no server and no network (works offline / in sandboxed / HPC environments) — and opens it via the existing cross-platform opener. The viewer lays the DAG out left-to-right, colours nodes by type (input / intermediate / final / phony / tool), draws each `MENU` group as a foldable box (Fold all / Unfold all, or click a group), supports `/`-search across name / HELP / DESC, and shows HELP/DESC on hover. The Graphviz `makefile_graph.pdf` path is unchanged and still available — the HTML is for exploration, the PDF for a static shareable artifact. Assembly is done by a new internal `makexx --build-graph` (reads `.makexx_graph.json`, inlines the embedded viewer). Adds ~700 KB to the `makexx` binary for the vendored Cytoscape stack

### Interactive mode

- **Targets without `HELP()` are now reachable in `makexx -i`.** They're collected into a folded **Undocumented** group (sorted just above the built-ins) so they don't clutter the default view but can be browsed (Tab to the group, → to unfold) and found via `/` search, which surfaces matches even under folded parents. Previously these rules were dropped from `.makexx_menu` entirely, so the TUI couldn't see them. Built-in targets are excluded, multi-target rules list each target individually, and a `<< DESC(...)` annotation is shown as the entry's description when present. `make help` output is unchanged — undocumented targets stay out of it

### Generated makefile

- **Fix:** `make` with no arguments now runs `all` as intended. The generated makefile now emits `.DEFAULT_GOAL := all` near the top so GNU make doesn't pick the `makefile: makefile.cpp makefile.hpp` regen rule as the default goal (which was the first non-special target and was overshadowing `all`). Auto-regen on `makefile.cpp` edits still fires before the build, as before. Users can override by setting `.DEFAULT_GOAL` in `mf.preamble`

## v0.3.2

### Packaging / install

- **Declare runtime dependencies on `make` and a C++ compiler** in every install path so users don't hit a cryptic compiler-not-found error on first run: the `.deb` control file now has `Depends: make, g++ | clang | c++-compiler`; the Homebrew formula adds `depends_on "make"` and `depends_on "gcc"`; `install.sh` runs a non-fatal check after installing and prints a per-distro install hint if `make` or any of `g++` / `clang++` / `icpx` / `icpc` is missing; README has a new "Requirements" section explaining what's needed and which install paths auto-pull what

### Makefile DSL

- **`<< DESC("file", "description")`** — describe a file (input, intermediate, or output) — what it is, its format, where it comes from. The description is rendered next to the file name in AGENTS.md so any agent reading the project gets the provenance / schema / contact in the same place as the file path. Works in either scope (`mf << DESC(...)` or `rule << DESC(...)`, colocated with the consuming/producing rule). Multiple DESC calls for the same file **accumulate** — joined with a space — so you can layer annotations across scopes (e.g. a base description at the makefile level + a contact line added later, or schema notes split across declarations). Order is mf-level first, then rule-level in command-insertion order. Three render sites: inline in `## Input files`; as a `- File: …` sub-bullet under each target row in `## Targets` so the file-level annotation is visually distinct from the rule's `HELP()`; and as a `` `name`: … `` sub-bullet in `## Intermediate targets`

### AGENTS.md

- **Fix:** corrected the upstream link from the placeholder `ab-10/makexx` to the real repository
- **Build-system overview:** added a "Build system" section explaining the makexx workflow (`makexx`, `makexx -c`, `makexx -i`) and pointing AI agents at the canonical DSL reference (`include/makexxfile.hpp` and `CLAUDE.md` raw URLs) so they can fetch context without external prompting
- **Inline DSL quick reference:** AGENTS.md now embeds a compact, self-contained DSL cheat sheet covering the common call shapes (rule + flags + menu + settings + helpers). Lets offline / sandboxed / no-web-fetch agents make routine edits (add a rule, mark phony, add to a group) without having to fetch the upstream reference. The raw-URL links to the full reference are kept as a "go here for edge cases" fallback
- **Target annotations:** phony rules now show `(phony)` and rules with `<< TOOL(...)` show `(uses ...)`, alongside the existing `(from ...)` source-deps annotation
- **Built-in target list completed:** added `list_input` and `list_unknown`; tightened wording on the rest
- **Intermediate-targets section:** AGENTS.md now lists rules without a `HELP()` description (typically internal / glue steps) so the dependency graph the workflow user reads is complete. Without this, a `(from foo.t)` annotation pointing at an un-`HELP`'d rule looked orphaned. Built-in phony targets like `help` are filtered so they don't double-list
- **Nested group headings:** changed `### Leaf` (indented for depth) to `#### Parent/Child` — heading level encodes depth, the slash-path in the heading text makes the nesting unambiguous. The leaf-only indented form had been misread as a stray indent in eval testing
- **Clearer PHONY annotation in the embedded cheat sheet:** spelled out that `PHONY` is REQUIRED for any target whose name isn't a file the recipe creates (`install`, `clean_*`, `list_*`). Without this, developer agents writing a new phony rule had to guess and were inconsistent

## v0.3.1

### Makefile DSL

- **`open_file()` ordering:** try `wslview` before `xdg-open`. On WSL both exist, but `xdg-open` routes Office formats (e.g., `.pptx`) to broken Linux handlers instead of the Windows host. The new order is wslview → xdg-open → open → start

## v0.3.0

### Makefile DSL

- **`<< TOOL("prog")` flag:** declare external executables a rule depends on. Each tool becomes a mtime-tracked prerequisite (so replacing/recompiling the tool triggers downstream rebuilds) but is NOT added to `$^` — your recipe keeps using `$<` cleanly. Bare names are resolved by make at parse time via `$(shell command -v ...)` for cross-machine portability; arguments containing `/` are used literally (best for sibling-project tools at known paths). Also accepts variadic `TOOL("a","b")` and `TOOL({"a","b"})` forms
- **`open_file(path)` helper:** returns a portable shell snippet that hands the file to whichever OS opener exists at `make` time — tries `xdg-open` (Linux), `open` (macOS), `wslview` (WSL), then `start` (generic Windows). One `makefile.cpp` works across all of them; no regen needed when moving between systems

### Interactive mode (`makexx -i`)

- **Fix:** folded parent groups with no direct entries (only nested children) now render their header so they can be unfolded. Previously the visibility check short-circuited through `is_ancestor_folded` and the parent hid itself, leaving the children effectively unreachable from the TUI
- **`r` refresh:** rerun `makexx -c` and reload the menu file in place — no need to quit, recompile, and relaunch after editing `makefile.cpp`. Cursor (by target name), fold state (by group name), multi-select (by target name), and search filter are all preserved across the reload. Compile errors are shown and the existing in-memory state is kept

- **Fix:** backspacing the search query down to empty no longer kicks you out of search mode (you came in via `/`; only Esc or Enter should leave). Lets you rebuild a query without re-entering the mode
- **Ctrl+Backspace in search:** clear the entire query while staying in search mode (Esc still both clears and exits). Shown in the search-input header so it's discoverable. Bytes 8 (Ctrl+Backspace) and 127 (plain Backspace) are now distinguished — previously both were collapsed into KEY_BACKSPACE
- **Terminal signals disabled:** `ISIG` and `IEXTEN` are now turned off in the TUI's termios, so Ctrl+\\ no longer dumps core and Ctrl+C / Ctrl+Z are silent in interactive mode. Use `q` or double-Esc to quit
- **Fix:** search now surfaces matches that live under a folded parent group. Previously the early `is_ancestor_folded` check short-circuited the recursion before the search-active override (which already lifted a group's own folded state) had a chance to apply
- A single Escape no longer exits — too easy to fat-finger when meaning to dismiss something. Esc still clears a search filter if one is active; otherwise the first Esc replaces the keyboard-hints header with a yellow "Press Esc again to exit" prompt, and a second Esc within 2 s exits. The prompt clears itself after the window expires (the loop polls stdin with the remaining time and redraws on timeout) or on any other keypress. `q` continues to quit immediately

## v0.2.0

### Makefile DSL

- **Breaking — settings renamed for consistency:** `mf.help_title` → `mf.title`, and `mf.description("...")` is now the public field `mf.description = "..."` (was the only function-style setter; matches `title`, `silent`, `echo`, `context`, `context_filename`). Update call sites accordingly
- **`<< PHONY` flag:** mark a rule's targets as `.PHONY` so `make` always runs them even if a file with the same name exists. `<< PHONY` marks all of the rule's targets; `<< PHONY("name")`, `<< PHONY("a","b")`, or `<< PHONY({"a","b"})` mark only specific targets in a multi-target rule (when one output is a real file and the other is phony). The `.PHONY:` line is emitted from the union of built-ins and user-marked targets (sorted)
- **Error diagnostics pointing back to `makefile.cpp`:** when `make` fails, `makexx` now parses its stderr for the failing target name (`No rule to make target 'X'` / `[makefile:N: X] Error N`) and prints a compiler-style annotation showing each occurrence of that target string in `makefile.cpp`, with a colored underline. Closes the loop where errors used to reference the generated makefile that users don't write
- **`mf.preamble` escape hatch:** a free-form string injected near the top of the generated makefile (after `SHELL=`, before `.PHONY:`). Lets you set GNU make variables (`CFLAGS ?= -O2` for `make CFLAGS=-g` overrides), `include` other makefiles, declare `vpath`, set `.SUFFIXES`, etc. without makexx having to model each idiom. Errors surface from `make`, not `makexx`
- **Makefile-level `<< PHONY` / `<< RETAIN`:** `mf << PHONY("install")` declares phony targets independent of which rule produces them; `mf << RETAIN("file")` excludes files from `soft_clean` (same effect as `mf.on_softclean_retain(...)`, on the `<<` idiom). Both accept the same selective forms as the per-rule version: single, variadic, or braced list
- **`<< RETAIN` consistency:** the no-paren form `<< RETAIN` (previously documented but didn't compile) now actually works — `RETAIN` is a global instance whose `operator()` accepts the previous forms `RETAIN("file")` / `RETAIN({"a","b"})` and a new variadic form `RETAIN("a","b","c")`. Existing call sites are unchanged
- **Menu group descriptions:** `mf << MENU("Build", "Compile sources")` attaches a description to a group. It's rendered next to the group header in `make help` (em-dash separator), as an italic line under the heading in `AGENTS.md`, and as dimmed text next to the group header in the interactive TUI. Compatible with `FOLDED`: `MENU("Build", "Compile sources", FOLDED)`
- **Auto-created parent groups:** nested menu groups (e.g. `Build/Tests`, `Processing/QC/Plots`) now automatically register every parent path in `make help` and `AGENTS.md`, so parent section headers appear even when no rule lives directly in the parent

### Interactive mode (`makexx -i`)

- Parent group headers are auto-created in the TUI for nested groups, matching the behavior of `make help`; folding a parent now collapses all of its nested children
- **Fix:** group order in the TUI now matches `make help` (definition order). Previously the TUI ordered groups by entry insertion, so a per-rule `<< MENU("X")` (which registers `X` later, at `dump_help` time) could appear before a `mf << MENU("Y")` that was declared earlier. `.makexx_menu` now carries explicit `!group` lines in the canonical order
- **Fix:** cursor sticks to the same target name across visible-list changes — running a target with Enter (Recent group grows by one and shifts rows below) and pressing Escape to clear a search filter (the visible list expands back to all rows). Previously the cursor was a plain row index, so both cases moved the highlight off the user's target
- Folding a group from inside (`←` on an entry) jumps the cursor to the group header as before; unfolding that same group immediately afterwards (`→` or Space) now returns the cursor to the entry it was on before the fold. Any other key in between clears the memory

## v0.1.2

### Makefile DSL

- **`<< RETAIN`** — exclude rule targets from `soft_clean` directly on the rule definition; `<< RETAIN("file")` for explicit filenames, replacing `mf.on_softclean_retain()`
- **`change_ext(file, {exts})`** — overload returning `vector<string>` for multiple extensions in one call
- **`get_ext()`** — renamed from `get_extension()` for consistency with `change_ext()`

## v0.1.1

### Makefile DSL

- **Menu groups:** `mf << MENU("name")` replaces `mf.set_current_menu("name")` and `mf.define_menu("name", FOLDED)` — same `<<` idiom as rules, supports optional `FOLDED` argument

## v0.1.0

Initial public release.

### Makefile DSL

- **Rule API:** `mf.add(targets, sources)` with `<<` for commands and metadata
- **Target types:** `FINAL`, `OPTIONAL`, `INPUT`
- **Metadata helpers:** `TEMP()`, `BYPRODUCT()`, `TARGET()`, `HELP()`
- **Menu groups:** `rule << MENU("name")` for single-rule groups, `mf << MENU("name")` for many rules in sequence, `mf << MENU("name", FOLDED)` for folded groups. Nested groups via `/` separator
- **Help system:** `make help` with box-drawing brackets, word-wrapping, and grouped output
- **Path helpers:** `stem()`, `basename()`, `change_ext()`, `join_path()`, `get_extension()`, `replace_all()`, `to_upper()`, `to_lower()`
- **AI context generation:** `mf.generate()` writes `AGENTS.md` alongside the makefile
- **Dependency graph:** `mf.generate_with_graph()` outputs Graphviz DOT file
- **Auto-regeneration:** editing `makefile.cpp` and running `make` automatically reruns `makexx -c` and restarts with the new rules. All headers included by `makefile.cpp` (e.g., `config.hpp`) are tracked via `-MMD` dependency scanning
- **Built-in targets:** `all`, `full_clean`, `soft_clean`, `list`, `list_unknown`, `list_input`, `help`

### Interactive mode (`makexx -i`)

- Terminal UI for browsing and running targets
- **Navigation:** arrow keys, PgUp/PgDn, Home/End
- **Group navigation:** Tab/Shift+Tab to jump between groups
- **Fold/unfold:** ←→ and Space on group headers
- **Search:** `/` to filter targets by name or description (case-insensitive)
- **Multi-select:** Space to toggle selection, Enter to run all selected, `x` to deselect all
- **Dry-run:** `d` to preview commands (`make -n`) without executing
- **Dependency preview:** `?` to show a target's source dependencies
- **Run history:** recently run targets appear in a "Recent" group (persisted in `.makexx_history`)
- **Viewport scrolling:** automatic scroll to keep cursor visible, `▲`/`▼` indicators
- **Word-wrapping:** long descriptions wrap to terminal width
- **Exit feedback:** green "Done." on success, red "Failed." on error

### CLI

- `-u` force update `makefile.hpp`
- `-f` force overwrite non-makexx `makefile`
- `-c` compile only (skip `make`)
- `-v` verbose output
- `-i` interactive target selector
- `-Dname=value` forward preprocessor defines to `makefile.cpp` compilation
- `-h` / `--help` show usage, `--version` show version
- Helpful compiler-not-found error with install suggestions
- Version mismatch warning when local `makefile.hpp` differs from embedded version

### Deployment

- GitHub Actions CI (Linux + macOS)
- Release workflow: builds binaries for Linux (x86_64, aarch64) and macOS (x86_64, arm64)
- `.deb` packages for Debian/Ubuntu
- Homebrew formula
- `curl | sh` install script
