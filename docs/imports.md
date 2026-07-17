# Imports

Use `import` to bring another script’s **static** functions into scope so you can call them without the script name prefix.

## Syntax

Imports are top-level declarations (before or alongside your `script` block):

```vellum
import Debug
import Utility

script MyScript : ObjectReference {
    event OnActivate(who: ObjectReference) {
        Notification("Activated")
        Wait(1.0)
    }
}
```

The name after `import` is a script stem (the file name without extension), for example `Debug` for `Debug.psc` / `Debug.vel`.

## Qualified and bare calls

After `import Debug`, both forms are valid:

```vellum
import Debug

script MyScript {
    fun Foo() {
        Debug.Notification("qualified")
        Notification("bare")
    }
}
```

Bare calls only resolve to **static** functions from imported scripts. Instance methods still need a receiver.

## Import paths

The compiler searches import directories for `.vel` and `.psc` files. Pass extra roots with `-i` / `--import` (see [Using the compiler](compiler.md)). In the VS Code extension, set `vellum.importPaths` (workspace folders are always included).

If the same script name appears in more than one import path, the last match wins and the compiler may emit a duplicate-name warning.

## Shadowing and ambiguity

- A local function, parameter, or variable with the same name as an imported static **wins** over the import for bare calls.
- If two different imports export a static with the same name, a bare call is **ambiguous** — use a qualified call (`ScriptName.Fun(...)`) instead.

You cannot import built-in literal types (`Int`, `String`, and so on).
