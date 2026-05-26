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
mf.help_title = "Seismic Pipeline v2";

mf.set_current_menu("Processing");
mf.add("filtered.bin", "raw.segy")
    << HELP("Apply bandpass filter")
    << "atbpfilt $< $@";

mf.set_current_menu("Processing/QC");
mf.add("report.pdf", "filtered.bin")
    << HELP("Generate QC report") << "qcplot $< $@";

// Single rule in a group — use MENU inline, no need for set_current_menu:
mf.add("backup.tar", "filtered.bin")
    << MENU("Archive") << HELP("Archive raw data") << "tar cf $@ $<";
```

Then `make help` prints:

```
Seismic Pipeline v2

Processing:
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
mf.description("Manages a family genealogy database. Generates SVG "
    "tree visualizations from a SQLite database using Graphviz.");
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
stem("dir/file.cpp")            // "file"
basename("dir/file.cpp")        // "file.cpp"
change_ext("file.cpp", ".o")    // "file.o"
join_path("obj", "file.o")      // "obj/file.o"
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

**Quick install** (Linux & macOS):

```bash
curl -fsSL https://raw.githubusercontent.com/abdullah-altheyab/makexx/main/install.sh | sh
```

**Homebrew** (macOS & Linux):

```bash
brew tap abdullah-altheyab/makexx
brew install makexx
```

**Debian/Ubuntu:**

```bash
# Download the .deb from the latest release
curl -fsSLO https://github.com/abdullah-altheyab/makexx/releases/latest/download/makexx_VERSION_amd64.deb
sudo dpkg -i makexx_*.deb
```

**From source:**

```bash
cmake -B build
cmake --build build
cmake --install build   # installs makexx to /usr/local/bin
```

---

## Optional dependencies

- [Graphviz](https://graphviz.org/) — needed only if you use `generate_with_graph()` to visualize your dependency graph as a PDF. Install with `apt install graphviz`, `brew install graphviz`, or `choco install graphviz`.

---

## Getting started

Run `makexx` in any directory. If no `makefile.cpp` exists, it creates a starter template. Edit it, then run `makexx` again.

See [`examples/`](examples/) for a full C++ project build, a portfolio analytics workflow with config separation, a genealogy workflow with AI agent context generation, and a simulation workflow.
