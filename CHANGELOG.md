# Changelog

All notable changes to the Vellum compiler are documented in this file.

The VS Code extension has its own changelog under [`tools/editors/vscode/CHANGELOG.md`](tools/editors/vscode/CHANGELOG.md).

## 0.2.0 — 2026-07-16

### Language

- `import` with qualified and bare static calls
- Interpolated strings (`$"…"`) with `{expr}` holes and `{{` / `}}` escapes
- `match` statement with literal patterns, `|` or-patterns, and `else`
- `is` type test operator (alongside `as`)
- Array initializer lists (`[1, 2, 3]`) in addition to sized allocation (`[Int; N]`)
- `for` over FormLists, optional index binding (`for x, i in …`), and Int ranges (`0..n`)
- Implicit cast to `Bool` in conditions (`if`, `while`, ternary, `!`, `&&` / `||`)
- Derived-to-base implicit promotion for script types
- `hidden` / `conditional` modifiers on scripts, properties, and variables
- `native` (and `native static`) functions

### Compiler CLI

- Batch compile: pass a directory to `-f` (recursive by default; `--no-recursive` available)
- `-o` / `--output` for `.pex` output directory

### Related tooling

- VS Code extension 0.1.0 (first public release): diagnostics, completion, go-to-definition, **Vellum: Compile**, import path / compiler / output settings
- Syntax highlighters updated for new keywords and interpolated strings

## 0.1.0

Initial public compiler release.
