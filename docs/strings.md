# Strings

Vellum has ordinary string literals and interpolated strings.

## Ordinary strings

Double-quoted strings are type `String`:

```vellum
var greeting = "Hello, World!"
```

Use the usual escape sequences inside them (for example `\"`, `\\`).

## Interpolated strings

Prefix a string with `$` to embed expressions:

```vellum
import Debug

script Hello : ObjectReference {
    event OnActivate(actor: ObjectReference) {
        for val in [1, 2, 3, 4] {
            Notification($"{val}^2 == {val * val}")
        }
    }
}
```

Rules:

- Write `{expression}` for a hole. The expression is any logical-or expression (no assignment).
- Non-`String` values in holes are cast to `String` automatically.
- Nested `$"..."` inside a hole is allowed.
- To put a literal `{` or `}` in the text, double it: `{{` and `}}`.
- A lone `}` in the string text is a compile error.

The whole interpolated string has type `String`.
