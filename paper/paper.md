---
title: 'makexx: A C++ Build System Generator for Scientific Workflows'
tags:
  - C++
  - build system
  - workflow management
  - GNU Make
  - HPC
authors:
  - name: Abdullah AlTheyab
    orcid: 0000-0000-0000-0000
    affiliation: 1
affiliations:
  - name: Independent Researcher
    index: 1
date: 26 May 2026
bibliography: paper.bib
---

# Summary

`makexx` is a build system generator that lets users describe workflows in `makefile.cpp` — a standard C++ program using a lightweight API — and produces GNU makefiles with full dependency tracking and incremental execution. Rather than inventing a new domain-specific language, `makexx` uses C++ itself as the configuration language, giving users loops, conditionals, classes, templates, and the standard library to express complex build and data-processing pipelines. The generated makefile is a portable artifact that runs anywhere GNU Make is available, with no runtime dependency on `makexx` itself.

# Statement of Need

Scientific computing workflows — data processing pipelines, simulation campaigns, report generation — require dependency tracking to avoid redundant computation. Researchers working in C++ face a choice between several imperfect options:

- **GNU Make** provides robust dependency tracking but its macro language becomes unreadable for pipelines with hundreds of parameterized rules.
- **CMake** [@cmake] targets C++ compilation specifically and uses its own DSL that lacks general programming constructs.
- **Snakemake** [@snakemake] offers Python-based workflow definition but introduces a language boundary for C++ practitioners and requires a Python runtime on execution hosts.
- **Shell scripts** provide no dependency tracking, forcing full re-execution on every run.

`makexx` addresses this gap: a workflow tool where the configuration is written in the same language as the project, with zero runtime dependencies beyond `make`. This is particularly relevant in HPC environments where Python may not be available but `make` and a C++ compiler are standard.

# Design and Features

## Architecture

When invoked, `makexx` extracts a header file (`makefile.hpp`) containing the API, compiles the user's `makefile.cpp` into a temporary executable, runs it to generate a standard GNU makefile, then optionally invokes `make`. The generated makefile includes a self-regeneration rule: if `makefile.cpp` or any header it includes is modified, running `make` automatically reruns `makexx` and restarts with updated rules.

## The DSL

The API centers on a `Makefile` class with an `add()` method for declaring rules and C++ operator overloading (`<<`) for attaching commands, metadata, and menu organization:

```cpp
Makefile mf;
mf.set_current_menu("Processing");
mf.add("result.bin", "input.dat")
    << FINAL << HELP("Run simulation")
    << "simulate --input $< --output $@";
mf.generate();
```

Users can define domain classes, use loops to generate parameterized rules, toggle pipeline branches with `#define` flags, and separate configuration into header files — all within standard C++.

## Interactive Target Selection

`makexx -i` launches a terminal interface for browsing and running targets, with keyboard navigation, search and filtering, multi-select batch execution, dry-run preview, dependency inspection, and run history.

## AI Agent Context

`generate()` writes an `AGENTS.md` file summarizing the project's targets, dependencies, and structure in plain English, enabling AI coding assistants to understand and modify the workflow.

# Comparison with Existing Tools

| Feature | makexx | Make | CMake | Snakemake |
|---|---|---|---|---|
| Configuration language | C++ | Make DSL | CMake DSL | Python |
| Full programming model | Yes | No | No | Yes |
| Runtime dependency | None | None | CMake | Python |
| Runs on bare HPC | Yes | Yes | No | No |
| Interactive TUI | Yes | No | No | No |
| AI context generation | Yes | No | No | No |

# Research Applications

`makexx` has been used in production for exploration portfolio analytics workflows involving Monte Carlo simulation, multi-stage data processing, and automated reporting across dozens of parameterized geological plays. These workflows generate hundreds of interdependent rules from C++ class hierarchies, a pattern that would be impractical in Make's macro language or a static configuration format.

# AI Usage Disclosure

Claude Code (Anthropic) was used during development for code generation, refactoring, and documentation. All AI-generated code was reviewed, tested, and validated by the author. The test suite (33 tests) and CI pipeline (Linux and macOS) verify correctness.

# Acknowledgments

The author thanks the open-source communities behind GNU Make and CMake for the foundational tooling that inspired this project.

# References
