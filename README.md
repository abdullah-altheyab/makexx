# makexx

**Write your pipelines and builds in C++.**

makexx lets you describe your workflow in a `makefile.cpp` — a plain C++ program using a simple API. Run `makexx`, and it compiles your program, executes it to generate a standard GNU `makefile`, then runs `make`.

You get full dependency tracking and incremental execution. Your pipeline logic is written in the language you already know.

---

## The problem

If you write C++, your pipelines probably look like one of these today:

- A Makefile that's grown unreadable, full of string hacks and copy-pasted rules
- A shell script with no dependency tracking — reruns everything every time
- A Python workflow tool that feels foreign to your codebase


---

## A quick example

A three-step seismic processing pipeline:

```cpp
#include "makefile.hpp"

int main() {
    Makefile mf;

    mf.add("filtered.bin", "raw.segy")
        << "atbpfilt input=raw.segy flo=5 fhi=80 output=filtered.bin";

    mf.add("velocity.bin", "filtered.bin")
        << "atnmo input=filtered.bin output=velocity.bin";

    mf.add("image.bin", {"velocity.bin", "filtered.bin"})
        << FINAL
        << "atkirchmig vel=velocity.bin input=filtered.bin output=image.bin";

    mf.generate();
}
```

```bash
makexx          # generates and runs the pipeline
makexx -i       # interactive target selector (TUI)
make            # reruns only what changed (auto-regenerates if makefile.cpp was edited)
make image.bin  # run up to a specific target
make help       # list all targets with descriptions
```

---

## Where it shines

**C++ as the configuration language.** Loops, classes, `#define` flags, headers — no new DSL. Generate many parameterized rules from data, toggle whole branches with `#define`, keep parameters in a `config.hpp` that's auto-tracked by `-MMD`:

```cpp
struct Run { string name, grid, solver; int iter; };
for (auto& r : runs)
    mf.add(r.name + ".out", r.grid)
        << ("runsim --solver=" + r.solver + " --iter=" + to_string(r.iter) + " $< > $@");
```

Override from the command line with `makexx -DTRIAL` or `makexx -Diterations=50`.

**Self-documenting pipelines.** Add `HELP()` and group with `MENU()`. `make help` renders titles, descriptions, groups, and brackets for multi-target rules. The same data feeds `makexx -i` and `AGENTS.md`.

```cpp
mf.title = "Seismic Pipeline";
mf.add("filtered.bin", "raw.segy") << MENU("Processing") << HELP("Apply bandpass filter") << "atbpfilt $< $@";
mf.add("report.pdf", "filtered.bin") << MENU("Processing/QC") << HELP("Generate QC report") << "qcplot $< $@";  // parent auto-created
mf << MENU("Processing", "Run the data pipeline");             // optional: declare a group description / FOLDED
```

**AI agent context.** `mf.generate()` writes an `AGENTS.md` summarizing the project — description, inputs, targets per group — that any AI coding agent (Claude Code, Cursor, Copilot) reads to help modify `makefile.cpp` in domain terms.

**Interactive mode (`makexx -i`).** A TUI for browsing and running targets: `/` to search and filter, Space to multi-select, `d` to dry-run, `?` to show dependencies, `r` to refresh after editing `makefile.cpp` (cursor / fold / select / search are preserved across the reload). `q` quits; Esc dismisses; double-Esc quits when nothing is active. Targets without a `HELP()` are tucked into a folded **Undocumented** group — browse or `/`-search it to find rules you haven't documented yet.

**Interactive dependency graph.** `make graph` opens a **single self-contained `makefile_graph.html`** in your browser — no server, no network, works offline (on by default; set `mf.graph = false` to opt out). The Cytoscape/dagre viewer colours nodes by kind (input / intermediate / final / phony), routes multi-input/output rules through a synthetic **rule node**, and is built around **trace-seeded filtering**: when the same pipeline is instantiated many times (per play, region, AOI…), pick **seeds** — double-click a node, browse `Targets ▾` / `Tags ▾` (hashtags from `HELP`/`DESC`) / `Tools ▾`, or a node type — and it shows just that slice, connecting the seeds through their shared steps with `↑ inputs` / `↓ finals` for provenance and impact. The **Search** box highlights matches by name / `HELP` / `DESC` / tool name (kept distinct from seeds), and you can promote matches to seeds in one click. Tidy menus (File / View / Tracing / Actions), **save/load** of a view to a portable `.state.json`, PNG/SVG export, light/dark, and — when profiling data exists — **heat-coloring** nodes by run time / count / recency. A built-in `? Help` lists it all. The static Graphviz `make makefile_graph.pdf` is still there for a shareable artifact.

**Usage & timing data.** By default makexx logs how long each rule takes to an append-only `.makexx_hits` on every run (set `mf.profile = false` to turn it off). `makexx --stats` reads it back as a per-rule table — runs, last-used, total and median time, sorted so the **bottleneck is on top** — plus the targets nobody has run (review/deletion candidates). Local, append-only, no server: build-scan insight for a plain Makefile.

**Cross-project tool tracking.** `<< TOOL("prog")` declares an executable as a prereq so downstream targets rebuild when the tool changes (variadic: `TOOL("awk", "sed")`). Bare names resolve via `command -v`; paths with `/` are literal — perfect for tools built in a sibling project. Pair a tool with an install hint via `<< TOOLDESC("prog", "brew install prog")` (declares the tool *and* the hint in one call; a braced `TOOLDESC({"a","b"}, "...")` shares one hint across several binaries from the same package), and `make check_tools` verifies every declared tool is present, printing the hint for any that are missing.

**Helpful errors.** When `make` fails, `makexx` parses the error and points back to the matching `mf.add(...)` line in your `makefile.cpp` — no more chasing the generated makefile.

**Helper functions.** `stem()`, `basename()`, `change_ext()`, `join_path()`, `get_ext()`, plus `open_file("report.pdf")` for cross-platform file opening (wslview / xdg-open / open / start, picked at make time).

---

## Comparison

| | **makexx** | Make | Snakemake | CMake |
|---|---|---|---|---|
| Description language | C++ | Make syntax | Python | CMake DSL |
| File-based dependency tracking | ✓ | ✓ | ✓ | ✓ |
| Incremental execution | ✓ | ✓ | ✓ | ✓ |
| Full programming model | ✓ | — | ✓ | — |
| Runs on bare HPC (needs only `make`) | ✓ | ✓ | — | — |
| No new language for C++ users | ✓ | — | — | — |
| Interactive target selector | ✓ | — | — | — |
| Self-documenting targets | ✓ | — | ✓ | — |
| AI agent context generation | ✓ | — | — | — |

---

## Requirements

`makexx` invokes a C++ compiler to compile your `makefile.cpp`, then runs `make` on the generated makefile. Both must be on `PATH` at runtime:

- **GNU make** (`apt install make`, ships on macOS, in any HPC environment).
- **A C++17 compiler.** `makexx` honors `$CXX` if set, otherwise probes `g++`, `clang++`, `icpx`, `icpc` in that order. Any one of these is enough (`apt install g++` or `apt install clang`; on macOS install the Xcode Command Line Tools).

These dependencies are declared by the `.deb` package below — `dpkg -i` / `apt install` will pull them in automatically. The bare-binary installs (`install.sh` / manual download) won't.

## Installation


**From source (with cmake):**

```bash
git clone https://github.com/abdullah-altheyab/makexx.git
cd makexx
cmake -B build
cmake --build build
sudo cmake --install build   # installs to /usr/local/bin
```

**From source (without cmake):**

```bash
git clone https://github.com/abdullah-altheyab/makexx.git
cd makexx
sh build.sh             # needs only g++ or clang++
sudo cp makexx /usr/local/bin/
```

---

## Optional dependencies

- [Graphviz](https://graphviz.org/) — needed only for the static PDF graph (`make makefile_graph.pdf`). The interactive HTML graph (`make graph`) needs nothing — the viewer is self-contained and offline. Install with `apt install graphviz`, `brew install graphviz`, or `choco install graphviz`.

---

## Getting started

Run `makexx` in any directory. If no `makefile.cpp` exists, it creates a starter template. Edit it, then run `makexx` again.

See [`examples/`](examples/) for a full C++ project build, a portfolio analytics workflow with config separation, a genealogy workflow with AI agent context generation, and a simulation workflow.

---

## Porting an existing Makefile

makexx doesn't ship an importer — real-world Makefiles use pattern rules, `ifeq`, `$(call ...)`, includes, and target-specific variables that a hand-written parser would handle poorly. An AI assistant (Claude Code, Cursor, Copilot Chat, ChatGPT, …) is a much better fit: it understands the semantics, collapses repeated rules into C++ loops, and produces idiomatic `makefile.cpp` code. The catch is that the assistant probably doesn't know the makexx DSL yet — so you hand it over in the prompt.

**Option A — point the assistant at the upstream DSL reference** (works with anything that can fetch URLs: Claude Code, Cursor, Claude.ai by paste, etc.):

```text
Read these two files from the makexx repository to learn the DSL:
  https://raw.githubusercontent.com/abdullah-altheyab/makexx/main/include/makexxfile.hpp
  https://raw.githubusercontent.com/abdullah-altheyab/makexx/main/CLAUDE.md

Then translate the Makefile below into a single `makefile.cpp` using the
makexx C++ DSL. Prefer C++ loops over copy-pasted rules. Output only the
C++ source.

--- Makefile ---
<paste your Makefile here>
```

**Option B — self-contained prompt** (works in any chatbot, no web fetch needed). Paste the cheat sheet inline:

```text
makexx is a build-system generator: you write a C++ program (`makefile.cpp`)
that builds rules via a DSL, and it produces a standard GNU makefile.

Program skeleton:
  #include "makefile.hpp"
  int main() {
      Makefile mf;
      // ... define rules ...
      mf.generate();   // writes makefile + AGENTS.md
      return 0;
  }

DSL cheat sheet (all `<<` operators accept `std::string`, so concatenation works):
  auto& r = mf.add("target", "source");        // or {tgt1, tgt2}, {src1, src2}
  r << "shell command";                         // append a command (multiple OK, run in order)
  r << FINAL;                                   // include in `make all` — apply to whatever
                                                //   your original `all:` depended on
  r << PHONY;                                   // all of rule's targets are .PHONY
  r << PHONY("install");                        // selective: only named targets in a multi-target rule
  r << PHONY("a", "b");                         // selective variadic; also accepts {"a","b"}
  r << HELP("description");                     // show in `make help`
  r << TEMP("scratch.tmp");                     // cleaned by full_clean / soft_clean
  r << RETAIN;                                  // exclude all of rule's outputs from soft_clean
  r << RETAIN("file.bin");                      // selective; also RETAIN("a","b") or RETAIN({"a","b"})
  r << TOOL("prog");                            // external executable: mtime-tracked prereq
                                                //   not in `$^`. Bare name → command -v lookup;
                                                //   path with '/' → literal.
  r << TOOL("awk", "sed");                      // several at once (variadic); also TOOL({"a","b"})
  r << TOOLDESC("prog", "brew install prog");   // declare a tool + its install hint in one call
                                                //   (shown by `make check_tools`); mf << TOOLDESC too
  r << TOOLDESC({"a","b"}, "brew install pkg"); // braced: same hint for each (one package, many bins)
  r  << MENU("Build");                          // put THIS rule in a group (not sticky)
  r  << MENU("Build/Tests");                    // nested group via slash separator
  mf << MENU("Build/Tests", "unit tests");      // declare a group's description / FOLDED
  mf << PHONY("install");                       // declare phony at the makefile level
  mf << RETAIN("artifact.bin");                 // protect file from soft_clean
  mf.title = "My Project";
  mf.description = "project summary for AGENTS.md";
  mf.preamble = "CFLAGS ?= -O2\n";              // raw make injected near top
                                                //   (vars, include, vpath, .SUFFIXES)

Helpers (free functions):
  stem("dir/file.cpp")                  → "file"
  basename("dir/file.cpp")              → "file.cpp"
  change_ext("file.cpp", ".o")          → "file.o"
  join_path("obj", "file.o")            → "obj/file.o"
  get_ext("file.cpp")                   → "cpp"
  open_file("report.pdf")               → shell snippet that hands the file to the OS opener
                                            available at make time (wslview, xdg-open, open, start)

Inside command strings use the standard GNU make automatics `$<`, `$@`, `$^`.

Idioms when porting:
  - GNU make pattern rules (`%.o: %.c`) become a C++ loop that calls `mf.add()`
    once per source file.
  - GNU make variables (`CC=g++`) become C++ `std::string` constants you can
    concatenate into command strings.
  - Drop user-written `clean` rules — `make full_clean` and `make soft_clean`
    are emitted automatically.
  - Mark any target that doesn't produce an output file (`install`, `deploy`,
    `test`, etc.) with `<< PHONY`.
  - Give a target ALL its prerequisites in the single `mf.add(target, sources)`
    call — build a `std::vector<std::string>` of sources first if you must
    compute them. Calling `mf.add()` again for the same target is an **error**
    (a target may be defined only once; `makexx` aborts and points at both
    lines), not a way to append a prerequisite; reach for
    `mf.add_source(rule, "dep")` only when a source isn't known until later.

Translate the Makefile below into a single `makefile.cpp`. Output only the
C++ source.

--- Makefile ---
<paste your Makefile here>
```

After the assistant produces `makefile.cpp`, sanity-check it:

```bash
makexx -c                              # regenerate the makefile
diff <(make -n all) <(make -n -f Makefile.orig all)   # compare command sequences
make -n install                        # spot-check a few targets
```

Iterate with the assistant on anything that looks off — recursive variable expansion, non-trivial pattern-rule stem manipulation, and conditional blocks (`ifeq`) are the common rough edges.

## Acknowledgments

makexx stands on the shoulders of others:

- The interactive dependency graph is rendered by **[Cytoscape.js](https://js.cytoscape.org)** with **[dagre](https://github.com/dagrejs/dagre)** layout, plus the **cytoscape-dagre**, **cytoscape-expand-collapse**, and **cytoscape-svg** extensions — all MIT-licensed and vendored offline so the generated `makefile_graph.html` needs no network. Full attribution and licenses are in [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).
- The static PDF graph uses **[Graphviz](https://graphviz.org/)** when installed (optional, not bundled).
- Inspired by the foundational tooling of **GNU Make** and **CMake**.
- Built by [Abdullah AlTheyab](https://github.com/abdullah-altheyab), with **[Claude Code](https://claude.com/claude-code)** (Anthropic) used throughout for implementation, refactoring, and documentation — all reviewed and tested by the author.

makexx is MIT-licensed — see [LICENSE](LICENSE).
