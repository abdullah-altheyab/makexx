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
make            # reruns only what changed
make image.bin  # run up to a specific target
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

---

## Installation

```bash
cmake -B build
cmake --build build
cmake --install build   # installs makexx to /usr/local/bin
```

---

## Getting started

Run `makexx` in any directory. If no `makefile.cpp` exists, it creates a starter template. Edit it, then run `makexx` again.

See [`examples/`](examples/) for a full C++ project build and a multi-stage research pipeline.
