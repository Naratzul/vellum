# VS Code extension

The **Vellum** extension adds language support for `.vel` / `.vellum` files in Visual Studio Code. It bundles the `vellum-lsp` language server.

**Windows x64 only** for the initial public release. Linux and macOS binaries are not bundled yet.

Latest extension version: **0.1.0**.

## Features

- Syntax highlighting and semantic tokens
- Diagnostics on edit
- Code completion
- Go to definition
- **Vellum: Compile** — compile the active file to `.pex` via the `vellum` CLI

## Installation

Not published on the VS Code Marketplace yet. Install from a VSIX:

1. Download `vellum-<version>.vsix` from a [GitHub Release](https://github.com/Naratzul/vellum/releases) (extension releases use `vscode-v*` tags) or [build one locally](https://github.com/Naratzul/vellum/tree/main/tools/editors/vscode).
2. In VS Code, run **Extensions: Install from VSIX…** and select the file.
3. Open a `.vel` or `.vellum` file.
4. Install the [Vellum compiler](download.md) and set `vellum.compilerPath` if `vellum` is not on your `PATH`.

## Compile

Run **Vellum: Compile** from the Command Palette or the editor context menu while a `.vel` file is active. The extension saves the file, then runs the compiler with your workspace settings.

Compiler output appears in the **Vellum Compiler** output channel.

## Configuration

| Setting | Description |
| --- | --- |
| `vellum.compilerPath` | Path to the `vellum` compiler executable. When empty, runs `vellum` (`vellum.exe` on Windows) from `PATH`. |
| `vellum.importPaths` | Extra import directories passed to the compiler and language server. Workspace folder(s) are always included. Supports absolute paths and `${workspaceFolder}`. |
| `vellum.outputDirectory` | Output directory passed to the compiler (`-o`). When empty, `.pex` is written next to the source file. Supports absolute paths and `${workspaceFolder}`. |
| `vellum.trace.server` | Trace LSP traffic (`off`, `messages`, `verbose`). |
| `vellum.languageServerPath` | Optional path to a `vellum-lsp` executable. When empty, the bundled server under `server/<platform>` is used. |

Example `settings.json` for a Skyrim mod project:

```json
{
  "vellum.compilerPath": "C:/path/to/vellum.exe",
  "vellum.importPaths": [
    "${workspaceFolder}/Scripts/Source",
    "C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition/Data/Source/Scripts"
  ],
  "vellum.outputDirectory": "${workspaceFolder}/Scripts"
}
```

Import path behavior matches the CLI `-i` option; see [Imports](imports.md) and [Using the compiler](compiler.md).

## More

Full packaging and development notes live in the extension [README](https://github.com/Naratzul/vellum/blob/main/tools/editors/vscode/README.md) in the repository.
