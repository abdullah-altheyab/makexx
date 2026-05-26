# Roadmap

Ideas under consideration for future releases. No commitments or timelines.

## Potential features

- **Ninja backend** — `mf.generate_ninja()` as an opt-in alternative to GNU Make output. Useful for faster parallel builds on large compile-heavy projects. The DSL wouldn't change — only the output format.
- **`mf.on_softclean_retain()` rename** — shorten to `mf.retain()`.
- **`mf.context` rename** — rename to `mf.agents` / `mf.agents_filename` for clarity.
- **Tab completion** — shell completions for target names via `makexx <tab>`.
- **Progress indicator** — show build progress (e.g., `[3/12]`) during `make` execution.
