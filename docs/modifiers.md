# Modifiers

Vellum supports Papyrus-style flags on scripts, properties, and variables.

## Script modifiers

Put modifiers before `script`:

```vellum
hidden script MathEx {
    static fun Chance(percent: Int) -> Float {
        var roll = Utility.RandomInt(0, 99)
        return (roll < percent) ? 1.0 : 0.0
    }
}
```

| Modifier | Meaning |
| --- | --- |
| `hidden` | Script is hidden in the Creation Kit (PEX `hidden` flag). |
| `conditional` | Script may expose conditional properties/variables for dialogue conditions. |

You can combine them: `hidden conditional script MyScript { ... }`.

## Property and variable modifiers

| Target | Allowed modifiers |
| --- | --- |
| Auto property | `hidden`, `conditional` |
| Variable | `conditional` only |

`conditional` on a property or variable is only valid when the **script** itself is marked `conditional`. Conditional properties must be **auto** properties (`{get set}` without custom accessors).

```vellum
conditional script MyQuest : Quest {
    conditional var stageFlag: Bool { get set } = false
}
```

Modifiers are **not** allowed on functions, events, or `import` declarations. For engine-provided functions, see [`native`](functions.md#native-functions) on [Functions](functions.md).
