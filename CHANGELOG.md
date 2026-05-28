# Changelog

## Unreleased

### Makefile DSL

- **Auto-created parent groups:** nested menu groups (e.g. `Build/Tests`, `Processing/QC/Plots`) now automatically register every parent path in `make help` and `AGENTS.md`, so parent section headers appear even when no rule lives directly in the parent

### Interactive mode (`makexx -i`)

- Parent group headers are auto-created in the TUI for nested groups, matching the behavior of `make help`; folding a parent now collapses all of its nested children

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
