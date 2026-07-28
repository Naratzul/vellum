# Vellum

[Vellum](https://github.com/Naratzul/vellum) is a scripting language for Creation Kit–era games. This extension adds editor support for Vellum (`.vel`) in Visual Studio Code via the bundled `vellum-lsp` language server.

**Windows x64 only** for the initial release. Linux and macOS binaries are not bundled yet.

## Features

- Syntax highlighting and semantic tokens for Vellum
- Diagnostics on edit
- Code completion
- Go to definition
- **Vellum: Compile** — compile the active file to `.pex` via the `vellum` CLI

## Installation

1. In VS Code, open **Extensions** and search for **Vellum**, or install [naratzul.vellum-lsp](https://marketplace.visualstudio.com/items?itemName=naratzul.vellum-lsp) from the Marketplace.
2. Open a `.vel` file.
3. Install the [Vellum compiler](https://naratzul.github.io/vellum/download/) and set `vellum.compilerPath` if `vellum` is not on your `PATH`.

Alternatively, download `vellum-<version>.vsix` from a [GitHub Release](https://github.com/Naratzul/vellum/releases) (`vscode-v*` tags) or [build one locally](#building-a-vsix), then run **Extensions: Install from VSIX…**.

## Compile

Run **Vellum: Compile** from the Command Palette or the editor context menu while a `.vel` file is active. The extension saves the file, then runs `vellum.exe` with your workspace settings.

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

## Tasks

You can also wire compile into VS Code tasks. Example for the active file:

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Vellum: Compile Active File",
      "type": "shell",
      "command": "${config:vellum.compilerPath}",
      "args": [
        "-f",
        "${file}",
        "-i",
        "${workspaceFolder}/Scripts/Source;C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition/Data/Source/Scripts",
        "-o",
        "${workspaceFolder}/Scripts"
      ],
      "options": {
        "cwd": "${workspaceFolder}"
      },
      "problemMatcher": [],
      "presentation": {
        "reveal": "always",
        "panel": "shared"
      }
    }
  ]
}
```

For batch compiles, call `vellum` from a script in your mod build pipeline. Multiple import paths are separated with `;` on the command line.

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
3. Set `vellum.languageServerPath` and `vellum.compilerPath` in workspace settings
4. Press **F5** to launch an Extension Development Host

Use `npm run watch` to recompile the client on change.

## Links

- [Vellum repository](https://github.com/Naratzul/vellum)
- [Documentation](https://naratzul.github.io/vellum/)

## License

MIT — see [LICENSE](LICENSE).
