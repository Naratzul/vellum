# Vellum

Vellum is a scripting language for Creation Kit–era games. It compiles to PEX and stays compatible with the Papyrus ecosystem, with a focus on clearer syntax and compiler errors. It adds language features that Papyrus does not have, and more are planned.

For now, only the Skyrim PEX subset is supported and tested. Support for other Creation Engine titles is planned.

## Documentation

Full docs (language, usage, examples): https://naratzul.github.io/vellum/

## Quick start

1. Download the compiler — see the [Download](docs/download.md) page in the docs.
2. Compile a script — from the docs, run:

   ```bat
   vellum.exe -f <path-to-your-script> -i <path-to-skyrim-scripts-folder>
   ```

   On success you get a `.pex` next to your `.vel` file. Use `vellum.exe -h` for more options.

3. Example scripts — [examples on GitHub](https://github.com/Naratzul/vellum/tree/main/examples).

## Compiler requirements

- Skyrim modding setup: you need the game’s Scripts source folder (the path passed to `-i`), as described in [Using the compiler](docs/compiler.md).
- Prebuilt releases are Windows binaries (`vellum.exe`) unless the download page says otherwise.

## Building from source

- CMake 3.12+
- C++23 toolchain (MSVC on Windows matches CI)
- Git with submodules (`git clone --recursive`)
- First CMake configure may download more dependencies (network required)

## Building the documentation site

- Python 3.12+ recommended
- `pip install -r requirements-docs.txt`, then `mkdocs build` or `mkdocs serve` (the documentation stack is listed under Acknowledgments).

## Acknowledgments

Vellum bundles or builds against:

- [lsp-framework](https://github.com/leon-bckl/lsp-framework)
- [sentry-native](https://github.com/getsentry/sentry-native)
- [cxxopts](https://github.com/jarro2783/cxxopts)
- [cpptrace](https://github.com/jeremy-rifkin/cpptrace)
- [Catch2](https://github.com/catchorg/Catch2)

The documentation site uses [MkDocs](https://www.mkdocs.org/), [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/), [PyMdown Extensions](https://facelessuser.github.io/pymdown-extensions/), and [Pygments](https://pygments.org/). Pin versions via [`requirements-docs.txt`](requirements-docs.txt).

Thanks to the authors and maintainers of these projects.
