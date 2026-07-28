# Vellum v0.2.0 — VS Code support, string interpolation, pattern matching and more

Hey guys, a few months ago I [announced](https://www.reddit.com/r/skyrimmods/comments/1sfsdrl/vellum_modern_scripting_language_for_skyrim/) the first release of Vellum — a scripting language for Skyrim that compiles to PEX and can be used alongside (or instead of) Papyrus.

**v0.2.0 is out.** Biggest additions: a VS Code extension, string interpolation, pattern matching, better `for` loops, array initializer lists, the `is` operator, and batch compilation.

## VS Code support

There's now a VS Code extension with syntax highlighting, diagnostics, completion, go-to-definition, and **Vellum: Compile** from the editor.

Download the `.vsix` from [GitHub Releases](https://github.com/Naratzul/vellum/releases) (`vscode-v*` tags) and install via **Extensions: Install from VSIX…**. Windows x64 for now.

[Demo (gif)](https://imgur.com/mgPG600)

## String interpolation

**Papyrus:**

```papyrus
Debug.Notification("Hello, " + actor.GetName() + "! Your level is " + actor.GetLevel() as String)
```

**Vellum:**

```vellum
Debug.Notification($"Hello {actor.GetName()}! Your level is {actor.GetLevel()}")
```

Prefix with `$` and put expressions in `{...}`. Non-strings are cast automatically.

## Pattern matching

**Papyrus:**

```papyrus
If (puzzleStage == 1 || puzzleStage == 2)
    doA()
ElseIf (puzzleStage == 3 )
    doB()
Else
    doOther()
EndIf
```

**Vellum:**

```vellum
match puzzleStage {
    1 | 2 => doA()
    3 => doB()
    else => doOther()
}
```



## Enhanced for loops

Iterate arrays, FormLists, and Int ranges. Bind index optionally.

**Papyrus:**

```papyrus
Int i = 0
While i < items.GetSize()
    Form item = items.GetAt(i)
    ; ...
    i += 1
EndWhile
```

**Vellum:**

```vellum
for item, i in items {
    // item is Form, i is Int
}

for i in 0..n {
    // 0, 1, ..., n-1
}
```



## Array initializer lists

**Papyrus:**

```papyrus
Int[] nums = new Int[4]
nums[0] = 1
nums[1] = 2
nums[2] = 3
nums[3] = 4
```

**Vellum:**

```vellum
var nums = [1, 2, 3, 4]
```



## is operator

Type test that returns `Bool`.

**Papyrus:**

```papyrus
If (source as Weapon) != None
    ; source is a Weapon
EndIf
```

**Vellum:**

```vellum
if source is Weapon {
    // source is a Weapon
}
```



## Batch compilation

Now you can pass a directory to -f and the compiler builds every .vel under it (recursive by default).

## Download

- **Nexus (compiler):** [https://www.nexusmods.com/skyrimspecialedition/mods/176581](https://www.nexusmods.com/skyrimspecialedition/mods/176581)
- **GitHub (compiler + VS Code VSIX):** [https://github.com/Naratzul/vellum/releases](https://github.com/Naratzul/vellum/releases)
- **Docs:** [https://naratzul.github.io/vellum/](https://naratzul.github.io/vellum/)



## Future plans

- Support other Creation Kit games (Fallout, Starfield)
- Debugger for VSCode

If you're a mod author and curious, try dropping a new Vellum script into an existing Papyrus mod. Same PEX runtime, so they interop in-game. Vellum can extend Papyrus scripts and call Papyrus APIs.

Happy to answer questions here and see your feedback and ideas.