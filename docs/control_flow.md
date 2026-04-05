# Control flow

Vellum gives you tools for controlling the flow of your scripts. Use an `if` statement, `for` and `while` loops to define your logic.

## Conditional statements

You can use the `if` statement to branch your code logic depending on the condition specified in `()`:

```swift
script MyScript {
    fun foo() {
        bar(42)
    }

    fun bar(i: Int) {
        if i < 100 {
            baz()
        } else if i == 100 {
            qux()
        } else {
            // do nothing
        }
    }

    fun baz() {

    }

    fun qux() {

    }
}
```

Vellum supports the conditional ternary operator `?:` that you can use, for example, for conditional variable initialization:

```swift
script MyScript {

    fun foo() {
        bar(true)
    }

    fun bar(condition: Bool) {
        var i = condition ? 42 : -1
    }
}
```

## Loops

Use a `while` loop for loops with arbitrary condition:

```swift
script MyScript {

    fun foo() {
        var i = 0
        while i < 10 {
            i += 1
        }
    }
}
```

Use a `for` loop to iterate over an array:

```swift
script MyScript {

    fun foo(nums: [Int]) {
        // iterate over array of numbers
        for num in nums {
            // do something with num
        }
    }
}
```

## Jump statements

You can use a `return` statement for early returning from a function:

```swift
script MyScript : ObjectReference {

    event onActivate(obj: ObjectReference) {
        if obj != Game.GetPlayer() {
            return
        }

        // do something
    }
}
```

Inside loops you can use `break` and `continue` statements.

- `break` terminates the nearest enclosing loop
- `continue` proceeds to the next step of the nearest enclosing loop

```swift
script MyScript : ObjectReference {

    fun foo() {
        var i = 0
        while true {
            if i == 10 {
                break
            }
            i += 1
        }
    }

    fun bar(actors: [Actor]) {
        // process array of actors
        for actor in actors {
            // skip none objects
            if actor == none {
                continue
            }
            // do something with valid actor
        }
    }
}
```