# makexx

**Write your pipelines and builds in C++, not YAML.**

makexx lets you describe your workflow in a `makefile.cpp` — a plain C++ program using a simple API. Run `makexx`, and it compiles your program, executes it to generate a standard GNU `makefile`, then runs `make`.

You get full dependency tracking and incremental execution. Your pipeline logic is written in the language you already know.

---

## The problem

If you write C++, your pipelines probably look like one of these today:

- A Makefile that's grown unreadable, full of string hacks and copy-pasted rules
- A shell script with no dependency tracking — reruns everything every time
- A Python workflow tool (Snakemake, Luigi) that feels foreign to your codebase

Snakemake is an elegant solution, but it's built for Python practitioners. makexx is the equivalent for C++ engineers and scientists.

---

## A quick example

A three-step seismic processing pipeline:

```cpp
#include "makefile.hpp"

int main() {
    Makefile mf;

    mf.add("filtered.bin", "raw.segy")
        << FINAL
        << "atbpfilt input=raw.segy flo=5 fhi=80 output=filtered.bin";

    mf.add("velocity.bin", "filtered.bin")
        << FINAL
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
make            # reruns only what changed
make image.bin  # run up to a specific target
make help       # list all targets with descriptions
```

---

## Where it shines

**Mode switching.** Toggle entire pipeline branches with `#define` flags at the top of `makefile.cpp`. Change `TRIAL` to a full run, switch input sources, enable optional steps — then re-run `makexx`.

```cpp
#define TRIAL   // use 2 iterations instead of 500
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

**Self-documenting pipelines.** Add descriptions and organize targets into groups:

```cpp
mf.help_title = "Seismic Pipeline v2";

mf.HELP_GROUP("Processing");
mf.add("filtered.bin", "raw.segy")
    << FINAL << HELP("Apply bandpass filter") << "atbpfilt $< $@";

mf.HELP_GROUP("Processing/QC");
mf.add("report.pdf", "filtered.bin")
    << HELP("Generate QC report") << "qcplot $< $@";

mf.HELP_GROUP("Archive", FOLDED);   // folded by default in makexx -i
mf.add("backup.tar", "filtered.bin")
    << HELP("Archive raw data") << "tar cf $@ $<";
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

- ↑↓, PgUp/PgDn, Home/End to navigate
- Tab/Shift+Tab to jump between groups
- ←→ to fold/unfold groups
- Enter to run, `d` to dry-run preview (`make -n`)
- `/` to search and filter targets by name or description
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

See [`examples/`](examples/) for a full C++ project build, a multi-stage research pipeline, and a genealogy workflow with AI agent context generation.
