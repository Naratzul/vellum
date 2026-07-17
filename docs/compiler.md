# Using the compiler

You can download the latest compiler from the [Download](download.md) page.

To compile your Vellum script, put your script into a **.vel** file and run the compiler with the following command:

```bat
vellum.exe -f <path-to-your-script> -i <path-to-skyrim-scripts-folder>
```

`-i` / `--import` adds directories where the compiler looks for imported scripts and base types (`.vel` / `.psc`). See [Imports](imports.md).

To write the `.pex` to a different folder (for example your mod's `Scripts` directory):

```bat
vellum.exe -f Scripts\Source\MyScript.vel -o Scripts
```

To compile every `.vel` file under a directory (recursive by default):

```bat
vellum.exe -f Scripts\Source -i <path-to-skyrim-scripts-folder> -o Scripts
```

Only compile sources directly in that folder (not subdirectories):

```bat
vellum.exe -f Scripts\Source --no-recursive -i <path-to-skyrim-scripts-folder> -o Scripts
```

Batch mode prints per-file status and a summary. It fails before compiling if two source files share the same script name (same `.pex` stem).

If compilation succeeds, this produces a **.pex** file alongside your script (or in the directory given by `-o` / `--output`).

Run `vellum.exe -h` to get info about other compiler options.

For editor-integrated compile, see the [VS Code extension](vscode.md).
