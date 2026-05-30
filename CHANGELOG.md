# Changelog

## Unreleased

### Makefile DSL

- **Breaking — settings renamed for consistency:** `mf.help_title` → `mf.title`, and `mf.description("...")` is now the public field `mf.description = "..."` (was the only function-style setter; matches `title`, `silent`, `echo`, `context`, `context_filename`). Update call sites accordingly
- **`<< PHONY` flag:** mark a rule's targets as `.PHONY` so `make` always runs them even if a file with the same name exists. `<< PHONY` marks all of the rule's targets; `<< PHONY("name")`, `<< PHONY("a","b")`, or `<< PHONY({"a","b"})` mark only specific targets in a multi-target rule (when one output is a real file and the other is phony). The `.PHONY:` line is emitted from the union of built-ins and user-marked targets (sorted)
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
