# Functions

To declare a function in Vellum:

- Use the `fun` keyword followed by the function's name
- Specify parameters in `()`
- Include the return type if needed

Vellum is **case-sensitive**. Prefer **PascalCase** for function and event names (`OnActivate`, `CountItems`). Parent Papyrus members still resolve case-insensitively when you override them.

```vellum
script MyScript {
    fun Foo(i: Int) -> Int {
        return i * 5
    }
}
```

## Parameters

Use `Name: Type` syntax to specify function parameters, for example:

```vellum
script MyScript {
    fun Foo(i: Int, f: Float, s: String, actor: Actor) {
        // do something
    }
}
```

## Parameters with default values

Function parameters can have default values (literal values or none for objects and arrays):

```vellum
script MyScript {
    fun Bar(s: String, i: Int = 10) {

    }

    fun Foo() {
        Bar("Hello") // second parameter will use default value 10
    }
}
```

## Return type

To return a specific value from a function, you have to specify the function's return type using `->` followed by the type name, and use a `return` statement:

```vellum
script MyScript {
    fun Concat(a: String, b: String) -> String {
        return a + b
    }

    fun Foo() {
        var result = Concat("Hello", ", world")
    }
}
```

If you omit the return type, then the function will have `None` as its return type.

## Static functions

You can mark a function as `static` to indicate that it can be called without an instance, on the script type itself. Special variable `self` (see below) is not available in static context:

```vellum
script MyUtility {
    static fun GetRandomInt() -> Int {
        return 4
    }
}

script MyScript {
    fun Foo() {
        var i = MyUtility.GetRandomInt()
    }
}
```

After `import MyUtility`, you can also call `GetRandomInt()` bare — see [Imports](imports.md).

## Native functions

Mark a function `native` when the implementation lives in the game engine (same idea as Papyrus `Native`). The body must be empty:

```vellum
script Game {
    native static fun GetPlayer() -> Actor
}
```

You can combine modifiers: `native static fun …`. `native` is only valid on **functions**, not on events. See also [Modifiers](modifiers.md) for `hidden` / `conditional` on scripts and properties.

## Self expression

You can use `self` expression to refer to the current script instance. The `self` keyword is not available inside `static` functions:

```vellum
script MyScript {

    var f: Float {get set} = 2.72

    fun Foo() {
        Bar(self)
    }

    fun Bar(obj: MyScript) {
        obj.Baz()
    }

    fun Baz() {
        var local = self.f
    }
}
```

## Events

Events are a special kind of function. You don't call them directly, it's done by the game engine. Events don't have an explicit return type (their implicit return type is `None`). Usually you override events from base scripts using the keyword `event`. For example:

```vellum
script MyScript : ObjectReference {

    event OnActivate(obj: ObjectReference) {
        // do something
    }
}
```
