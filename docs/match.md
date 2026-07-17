# Pattern matching

Vellum’s `match` statement branches on a value using literal patterns. It is a **statement**, not an expression.

## Syntax

```vellum
match scrutinee {
    pattern => body
    pat1 | pat2 => body
    else => body
}
```

Each arm’s body can be a block `{ ... }`, a jump statement (`return`, `break`, `continue`), or a single expression statement.

## Scrutinee types

The value you match on must be one of:

- `Bool`
- `Int`
- `Float`
- `String`

## Patterns

Patterns are literals (including negative numbers such as `-1`) or identifiers that name a variable or property of a compatible type.

Use `|` to combine several patterns in one arm (**or-patterns**). An optional `else =>` arm runs when nothing else matches.

```vellum
import Debug

script GreetOnActivate : ObjectReference {

    event OnActivate(akActionRef: ObjectReference) {
        var actor = akActionRef as Actor
        if !actor {
            return
        }
        Notification($"Hello, {Greet(actor)}")
    }

    fun Greet(actor: Actor) -> String {
        match actor.GetEquippedItemType(1) {
            1 | 2 => return "blade user"
            7 => return "archer"
            else => return "adventurer"
        }
    }
}
```

(Full sample: [examples/compare/greet](https://github.com/Naratzul/vellum/tree/main/examples/compare/greet).)

## Int and Float arms

If some arms use `Int` patterns and others use `Float` (or the scrutinee is the other numeric type), the compiler promotes as needed so comparisons stay consistent. Mixing Int and Float patterns in one `match` is allowed.

## Control flow

`match` does not fall through between arms. Use `return` / `break` / `continue` inside an arm when you need to leave the enclosing function or loop.
