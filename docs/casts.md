# Casts

Use the **`as`** operator for **explicit** casts: `expression as TargetType`. Only certain conversions are allowed (see below); invalid combinations are rejected by the compiler.

## Casting objects

Object references can only be cast to another **script/object type** when the types are related by **inheritance** (parent/child in the hierarchy).

Casting from a **parent** type to a **child** type may fail at runtime: if the value is not actually an instance of that child, the result is **`none`**. It's recommended to check for **`none`** after a narrowing cast.

Syntax: **`value as DerivedType`**.

```vellum
script MyScript : ObjectReference {

    event onActivate(obj: ObjectReference) {
        // Narrow ObjectReference to Actor; may be none if obj is not an Actor
        var who: Actor = obj as Actor
        if who == none {
            return
        }
        // use who as Actor
    }
}
```

## Casting to `String` and `Bool`

Many types can be cast **explicitly** to **`String`**. That's useful for debugging or if you want to show a message or notification to the user.

You can also cast suitable values to **`Bool`** when you want a **boolean** for **`if` / `while`** conditions (or to pass into something that expects `Bool`).

```vellum
script MyScript {

    fun print(nums: [Int]) {
        Debug.Trace(nums as String)
    }

    fun bar(item: Form) {
        var isWeapon = (item as Weapon) as Bool
        if isWeapon {
            // do something
        }
    }
}
```

At runtime, **`none`** converts to **`false`** when cast to **`Bool`**. So in the example, if `item as Weapon` fails (`none` because it is not a weapon), **`(item as Weapon) as Bool`** is **`false`**. Only a non-`none` reference becomes **`true`**.

Coercing to **`Bool`** is one way to branch when you only care about success vs failure without comparing to `none` explicitly.

## Implicit casts

There are several cases where the compiler will insert an implicit cast for you.

An expression of type `Int` can be promoted to `Float` in contexts where a `Float` is expected. For example, `1 + 1.0` has type `Float`.

Another case is objects related by **inheritance**. The compiler applies automatic upcast in comparison operators:

```vellum
script MyScript : ObjectReference {

    event onActivate(obj: ObjectReference) {
        var player: Actor = Game.GetPlayer()
        // no explicit cast required here
        if obj != player {
            return
        }
    }
}
```
