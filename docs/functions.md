# Functions

To declare a function in Vellum:

- Use the `fun` keyword followed by the function's name
- Specify parameters in `()`
- Include the return type if needed

```swift
script MyScript {
    fun foo(i: Int) -> Int {
        return i * 5
    }
}
```

## Parameters

Use `Name: Type` syntax to specify function parameters, for example:

```swift
script MyScript {
    fun foo(i: Int, f: Float, s: String, actor: Actor) {
        // do something
    }
}
```

## Parameters with default values

Function parameters can have default values (literal values or none for objects and arrays):

```swift
script MyScript {
    fun bar(s: String, i: Int = 10) {

    }

    fun foo() {
        bar("Hello") // second parameter will use default value 10
    }
}
```

## Return type

To return a specific value from a function, you have to specify the function's return type using `->` followed by the type name, and use a `return` statement:

```swift
script MyScript {
    fun concat(a: String, b: String) -> String {
        return a + b
    }

    fun foo() {
        var result = concat("Hello", ", world")
    }
}
```

If you omit the return type, then the function will have `None` as its return type.

## Static functions

You can mark a function as `static` to indicate that it can be called without an instance, on the script type itself. Special variable `self` (see below) is not available in static context:

```swift
script MyUtility {
    static fun getRandomInt() -> Int {
        return 4
    }
}

script MyScript {
    fun foo() {
        var i = MyUtility.getRandomInt()
    }
}

```

## Self expression

You can use `self` expression to refer to the current script instance. The `self` keyword is not available inside `static` functions:

```swift
script MyScript {

    var f: Float {get set} = 2.72

    fun foo() {
        bar(self)
    }

    fun bar(obj: MyScript) {
        obj.baz()
    }

    fun baz() {
        var local = self.f
    }
}
```

## Events

Events are a special kind of function. You don't call them directly, it's done by the game engine. Events don't have an explicit return type (their implicit return type is `None`). Usually you override events from base scripts using the keyword `event`. For example:

```swift
script MyScript : ObjectReference {

    event onActivate(obj: ObjectReference) {
        // do something
    }
}
```