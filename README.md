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

**Mode switching.** Toggle entire pipeline branches with `#define` flags at the top of `makefile.cpp`, or pass them from the command line:

```cpp
#define TRIAL   // use 2 iterations instead of 500
```

```bash
makexx -DTRIAL          # same as #define TRIAL in the file
makexx -Diterations=50  # override a value
```

**Parameterized rules.** Use C++ loops and data structures to generate many related rules concisely:

```cpp
for (auto& survey : surveys) {
    mf.add(survey.prefix + "_result.bin", survey.prefix + "_input.bin")
        << ("atprocess --mode=" + survey.mode + " $< > $@");
}
```

**Domain modeling.** Define C++ classes that carry your pipeline parameters — no config file format to learn:

```cpp
struct Simulation { string name, grid, solver; int iterations; };

for (auto& s : runs) {
    mf.add(s.name + ".out", s.grid)
        << ("runsim --solver=" + s.solver + " --iter=" + to_string(s.iterations) + " $< > $@");
}
```

**Config separation.** Keep parameters in a `config.hpp` to keep `makefile.cpp` focused on rules. makexx automatically tracks the dependency — editing `config.hpp` and running `make` regenerates the makefile:

```cpp
// config.hpp
#define TRIAL true
int iterations = TRIAL ? 10 : 5000;
vector<Run> runs = {{"baseline", "grid.dat", "--viscosity=1.0"}, ...};
```

```cpp
// makefile.cpp
#include "makefile.hpp"
#include "config.hpp"
```

**Self-documenting pipelines.** Add descriptions and organize targets into groups:

```cpp
mf.title = "Seismic Pipeline v2";

mf << MENU("Processing", "Run the data pipeline");   // group with a description
mf.add("filtered.bin", "raw.segy")
    << HELP("Apply bandpass filter")
    << "atbpfilt $< $@";

mf << MENU("Processing/QC");           // nested via "/" — parent headers are auto-created
mf.add("report.pdf", "filtered.bin")
    << HELP("Generate QC report") << "qcplot $< $@";

// Single rule in a group — use MENU inline:
mf.add("backup.tar", "filtered.bin")
    << MENU("Archive") << HELP("Archive raw data") << "tar cf $@ $<";
```

Then `make help` prints:

```
Seismic Pipeline v2

Processing: — Run the data pipeline
  filtered.bin ─── Apply bandpass filter

  QC:
    report.pdf ─── Generate QC report

Archive:
    backup.tar ─── Archive raw data

Built-in:
           all ─── Build all final targets
    full_clean ─── Remove all generated files
          help ─── Show this help
           ...
```

**AI agent context.** `generate()` writes an `AGENTS.md` alongside the makefile — a plain-English summary of the project for AI coding agents (Claude Code, Cursor, Copilot, etc.):

```cpp
mf.description = "Manages a family genealogy database. Generates SVG "
    "tree visualizations from a SQLite database using Graphviz.";
mf.generate();  // writes makefile + .makexx_menu + AGENTS.md
```

The generated `AGENTS.md` lists the project description, input files, and all targets with dependencies organized by group — so an AI agent can understand your project and help modify `makefile.cpp` in domain terms.

```cpp
mf.context = false;                  // disable AGENTS.md generation
mf.context_filename = "CLAUDE.md";   // use a different filename
```

**Interactive mode.** Run `makexx -i` for a terminal UI:

- ↑↓, PgUp/PgDn, Home/End to navigate; Tab/Shift+Tab to jump between groups
- ←→ to fold/unfold groups; `▲`/`▼` scroll indicators
- Enter to run, `d` to dry-run preview (`make -n`), `?` to show dependencies
- `/` to search and filter targets by name or description
- Space to multi-select targets (`●`), `x` to deselect all, Enter runs all selected in sequence
- Green **Done.** / red **Failed.** exit status after each run
- Recently run targets appear in a **Recent** group at the top

**Helper functions.** Path manipulation utilities available in your `makefile.cpp`:

```cpp
stem("dir/file.cpp")                    // "file"
basename("dir/file.cpp")               // "file.cpp"
change_ext("file.cpp", ".o")           // "file.o"
change_ext("file.cpp", {".o", ".d"})   // {"file.o", "file.d"}
get_ext("file.cpp")                    // "cpp"
join_path("obj", "file.o")             // "obj/file.o"
```

---

## Why GNU make as the backend?

The generated `makefile` is a plain text file. On most HPC clusters, `make` is the only build tool guaranteed to be available. No Python environment, no container, no package manager — `makexx` generates the file once on your workstation, and `make` runs it anywhere.

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

- [Graphviz](https://graphviz.org/) — needed only if you use `generate_with_graph()` to visualize your dependency graph as a PDF. Install with `apt install graphviz`, `brew install graphviz`, or `choco install graphviz`.

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
  mf << MENU("Build");                          // group subsequent rules
  mf << MENU("Build/Tests", "unit tests");      // nested group + description
  mf.title = "My Project";
  mf.description = "project summary for AGENTS.md";

Helpers (free functions):
  stem("dir/file.cpp")                  → "file"
  basename("dir/file.cpp")              → "file.cpp"
  change_ext("file.cpp", ".o")          → "file.o"
  join_path("obj", "file.o")            → "obj/file.o"
  get_ext("file.cpp")                   → "cpp"

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
