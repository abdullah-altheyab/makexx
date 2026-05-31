# Roadmap

Ideas under consideration for future releases. No commitments or timelines.

## Potential features

- **Ninja backend** — `mf.generate_ninja()` as an opt-in alternative to GNU Make output. Useful for faster parallel builds on large compile-heavy projects. The DSL wouldn't change — only the output format.
- **Precise rule-origin diagnostics (Tier 2)** — extend the existing error annotations (which currently grep `makefile.cpp` for the target string) to point at the exact `mf.add(...)` call site, including which iteration of a loop. Cleanest implementation uses C++20 `std::source_location` as a default arg on `add()` and the `operator<<` overloads — zero call-site change for users, but bumps the minimum standard from C++17 to C++20. Macro-based capture is the C++17 alternative but pollutes the global identifier `add` or forces users to wrap calls in a macro, both intrusive.
- **`mf.on_softclean_retain()` rename** — shorten to `mf.retain()`.
- **`mf.context` rename** — rename to `mf.agents` / `mf.agents_filename` for clarity.
- **Tab completion** — shell completions for target names via `makexx <tab>`.
- **Progress indicator** — show build progress (e.g., `[3/12]`) during `make` execution.
