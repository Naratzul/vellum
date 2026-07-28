# Changelog

All notable changes to the Vellum VS Code extension are documented in this file.

## 0.1.1 — 2026-07-28

- Document Marketplace install; drop `.vellum` file association (compiler supports `.vel` only)

## 0.1.0 — 2026-07-16

First public release.

- Vellum language support for `.vel` files (syntax highlighting, semantic tokens)
- Language server features: diagnostics, completion, go-to-definition
- Bundled `vellum-lsp` binary for Windows x64
- **Vellum: Compile** command — runs the `vellum` CLI with workspace settings
- Settings: `vellum.importPaths`, `vellum.compilerPath`, `vellum.outputDirectory`, `vellum.trace.server`, `vellum.languageServerPath`
- Task examples in the extension README
