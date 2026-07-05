# Vellum

[Vellum](https://github.com/Naratzul/vellum) is a scripting language for Creation Kit–era games. This extension adds editor support for Vellum (`.vel`, `.vellum`) in Visual Studio Code via the bundled `vellum-lsp` language server.

**Windows x64 only** for the initial release. Linux and macOS binaries are not bundled yet.

## Features

- Syntax highlighting and semantic tokens for Vellum
- Diagnostics on edit
- Code completion
- Go to definition

## Installation

Install from a VSIX (not published on the VS Code Marketplace yet):

1. Download `vellum-<version>.vsix` from a [GitHub Release](https://github.com/Naratzul/vellum/releases) or build one locally (see below).
2. In VS Code, run **Extensions: Install from VSIX…** and select the file.
3. Open a `.vel` or `.vellum` file.

## Configuration

| Setting | Description |
| --- | --- |
| `vellum.importPaths` | Extra directories to search for Vellum imports. Supports absolute paths and `${workspaceFolder}`. |
| `vellum.outputDirectory` | Directory for compiled `.pex` files. When empty, `.pex` is written next to the source file. Supports absolute paths and `${workspaceFolder}`. |
| `vellum.trace.server` | Trace LSP traffic (`off`, `messages`, `verbose`). |
| `vellum.languageServerPath` | Optional path to a `vellum-lsp` executable. When empty, the bundled server under `server/<platform>` is used. |

Example `settings.json` for a Skyrim mod project:

```json
{
  "vellum.importPaths": [
    "${workspaceFolder}/Scripts/Source",
    "C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition/Data/Source/Scripts"
  ],
  "vellum.outputDirectory": "${workspaceFolder}/Scripts"
}
```

## Building a VSIX

From the monorepo root, build the language server in Release, then package the extension:

```bat
cmake --build build --config Release
cd tools\editors\vscode
npm install
npm run package
```

The VSIX is written to `dist/vellum-<version>.vsix`.

To point at a custom server binary:

```bat
node scripts/package.js --server-path ..\..\..\bin\vellum-lsp\vellum-lsp.exe
```

## Development

1. `npm install` in `tools/editors/vscode`
2. Build `vellum-lsp` (Release) so `bin/vellum-lsp/vellum-lsp.exe` exists
3. Set `vellum.languageServerPath` in workspace settings to that executable
4. Press **F5** to launch an Extension Development Host

Use `npm run watch` to recompile the client on change.

## Links

- [Vellum repository](https://github.com/Naratzul/vellum)
- [Documentation](https://naratzul.github.io/vellum/)

## License

MIT — see [LICENSE](LICENSE).
