# Casts

Use the **`as`** operator for **explicit** casts: `expression as TargetType`. Only certain conversions are allowed (see below); invalid combinations are rejected by the compiler.

Use **`is`** for a type test that returns **`Bool`**: `expression is TargetType`.

## Casting objects

Object references can only be cast to another **script/object type** when the types are related by **inheritance** (parent/child in the hierarchy).

Casting from a **parent** type to a **child** type may fail at runtime: if the value is not actually an instance of that child, the result is **`none`**. It's recommended to check for **`none`** after a narrowing cast.

Syntax: **`value as DerivedType`**.

```vellum
script MyScript : ObjectReference {

    event OnActivate(obj: ObjectReference) {
        // Narrow ObjectReference to Actor; may be none if obj is not an Actor
        var who: Actor = obj as Actor
        if !who {
            return
        }
        // use who as Actor
    }
}
```

## The `is` operator

`value is Type` is a type test. It returns `Bool` and never produces a narrowed reference (use `as` when you need the value as that type).

On Skyrim, `is` compiles like `(value as Type) as Bool`: a failed narrow cast yields `none`, which becomes `false`. Arrays cannot be used as the `is` target type.

```vellum
script TrainingMannequin : ObjectReference {
    event OnHit(aggressor: ObjectReference, source: Form, p: Projectile,
                pa: Bool, sa: Bool, ba: Bool, hb: Bool) {
        if source is Weapon {
            // source is a Weapon (or subtype) at runtime
        }
    }
}
```

Prefer `is` when you only need a yes/no check; use `as` when you need the cast result.

## Casting to `String` and `Bool`

Many types can be cast **explicitly** to **`String`**. That's useful for debugging or if you want to show a message or notification to the user.

You can also cast values to **`Bool`** with `as Bool` when you need a boolean outside of a condition (for example, to store it or pass it to a function).

```vellum
script MyScript {

    fun Print(nums: [Int]) {
        Debug.Trace(nums as String)
    }

    fun Bar(item: Form) {
        var isWeapon = item is Weapon
        if isWeapon {
            // do something
        }
    }
}
```

At runtime, **`none`** converts to **`false`** when cast to **`Bool`**. So if `item as Weapon` fails (`none` because it is not a weapon), **`(item as Weapon) as Bool`** is **`false`**. Only a non-`none` reference becomes **`true`**. Other types follow Papyrus rules (non-zero numbers, non-empty strings, non-empty arrays, and so on).

## Condition coercion (implicit cast to `Bool`)

Like Papyrus, Vellum **implicitly casts to `Bool`** in condition contexts. You do **not** need `!= none` or `as Bool` for:

- `if` / `else if` conditions
- `while` conditions
- ternary (`cond ? a : b`) conditions
- unary `!` / `not`
- both sides of `&&` and `||`

```vellum
script MyScript : ObjectReference {

    event OnActivate(obj: ObjectReference) {
        var who = obj as Actor
        if !who {
            return
        }
        if who && who.IsDead() {
            return
        }
        // use who as Actor
    }
}
```

Assignments, returns, and call arguments that expect `Bool` still need an **explicit** `as Bool` (or a real `Bool` expression).

## Implicit casts

There are several cases where the compiler will insert an implicit cast for you.

An expression of type `Int` can be promoted to `Float` in contexts where a `Float` is expected. For example, `1 + 1.0` has type `Float`.

For objects related by **inheritance**, a **derived** type can be used where a **base** type is expected (assignments, arguments, returns, comparisons, and similar contexts). The compiler inserts the upcast:

```vellum
script MyScript : ObjectReference {

    event OnActivate(obj: ObjectReference) {
        var player: Actor = Game.GetPlayer()
        // Actor promotes to ObjectReference for the comparison
        if obj != player {
            return
        }
    }
}
```
