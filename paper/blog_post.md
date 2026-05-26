# Why I Wrote a Build System Where the Config Is a C++ Program

I've spent years writing Makefiles for scientific workflows — not just compiling code, but orchestrating data pipelines, Monte Carlo simulations, and report generation. Hundreds of rules, dozens of parameterized datasets, conditional branches based on whether it's a trial run or production.

GNU Make handles the dependency tracking perfectly. What it can't handle is the *configuration*. When you need to generate 50 similar rules from a list of geological plays, each with different parameters, recovery factors, and reference databases, Make's macro language breaks down. You end up with unreadable string hacks, copy-pasted blocks, and a 2000-line Makefile that nobody wants to touch.

So I built **makexx**.

## The idea

Instead of a configuration file (YAML, TOML, JSON) or a custom DSL (CMake, Meson), your build config is a **C++ program**:

```cpp
#include "makefile.hpp"
#include "config.hpp"  // your parameters

int main() {
    Makefile mf;
    for (auto& deposit : deposits) {
        mf.add(deposit.prefix + "_forecast.t", deposit.zone + "_data.t")
            << HELP("forecast " + deposit.title)
            << ("mcsim --rf=" + deposit.recovery_factor + " $< -o $@");
    }
    mf.generate();
}
```

Run `makexx`, and it compiles your program, executes it to generate a standard GNU makefile, then runs `make`. The generated makefile is a plain text file — portable, readable, no runtime dependency on makexx.

## Why C++ as config?

Because you already know it. And it's more powerful than any config format:

- **Loops and conditionals** — generate hundreds of rules from data structures
- **Classes and structs** — model your domain (plays, deposits, simulations) with types
- **`#define` flags** — toggle entire pipeline branches: `#define TRIAL` switches from 5000 iterations to 10
- **Header files** — separate parameters from rules with `#include "config.hpp"`
- **Standard library** — string manipulation, containers, algorithms — all available

No new language to learn. No YAML indentation fights. No CMake `if()` gymnastics.

## What about Snakemake?

Snakemake is excellent — for Python users. If your team writes Python and your cluster has Python, use Snakemake. But if you're a C++ shop running on bare HPC nodes where Python isn't guaranteed, you need something that compiles to a plain makefile and runs with just `make`.

## The interactive mode

This is the feature I didn't plan but now can't live without. `makexx -i` gives you a terminal UI:

- Arrow keys to browse targets, organized by groups
- `/` to search and filter
- `d` to preview what a target would do (dry run)
- `?` to see a target's dependencies
- Space to multi-select, Enter to run
- Tab/Shift+Tab to jump between groups

For a workflow with 60+ targets across data import, QC, simulation, planning, and reporting, being able to search for "benchmark" and run it without remembering the exact target name is a game-changer.

## Auto-regeneration

Edit `makefile.cpp` (or `config.hpp`), run `make`, and the makefile regenerates automatically. GNU Make natively supports remaking its own makefile — makexx wires this up with compiler-generated dependency tracking (`-MMD`). No need to remember to run `makexx` after editing.

## The `-D` flag trick

Since `makefile.cpp` is compiled C++, you can pass preprocessor defines from the command line:

```bash
makexx -DTRIAL              # quick test run
makexx -Diterations=5000    # production
```

No config file to edit, no environment variables to set.

## Who is this for?

- **C++ developers** who want their build system in the language they already know
- **Scientists and engineers** with complex parameterized workflows (simulations, data processing, report generation)
- **HPC users** who need portable makefiles that run on any cluster with `make`
- **Anyone** whose Makefile has outgrown Make's macro language

## Try it

```bash
curl -fsSL https://raw.githubusercontent.com/abdul900/makexx/main/install.sh | sh
```

Or build from source:

```bash
cmake -B build && cmake --build build && cmake --install build
```

Then run `makexx` in any directory. It creates a starter `makefile.cpp` — edit it and go.

GitHub: [github.com/abdul900/makexx](https://github.com/abdul900/makexx)
