# Control flow

Vellum gives you tools for controlling the flow of your scripts. Use an `if` statement, `for` and `while` loops to define your logic. For multi-way literal branches, see [Pattern matching](match.md).

## Conditional statements

You can use the `if` statement to branch your code logic depending on the condition specified in `()`:

```vellum
script MyScript {
    fun Foo() {
        Bar(42)
    }

    fun Bar(i: Int) {
        if i < 100 {
            Baz()
        } else if i == 100 {
            Qux()
        } else {
            // do nothing
        }
    }

    fun Baz() {

    }

    fun Qux() {

    }
}
```

Objects, numbers, strings, and arrays are implicitly cast to `Bool` (see [Casts](casts.md)):

```vellum
script MyScript : ObjectReference {
    event OnActivate(obj: ObjectReference) {
        var actor = obj as Actor
        if !actor {
            return
        }
        if actor {
            // actor is non-none
        }
    }
}
```

Vellum supports the conditional ternary operator `?:` that you can use, for example, for conditional variable initialization:

```vellum
script MyScript {

    fun Foo() {
        Bar(true)
    }

    fun Bar(condition: Bool) {
        var i = condition ? 42 : -1
    }
}
```

## Loops

Use a `while` loop for loops with arbitrary condition:

```vellum
script MyScript {

    fun Foo() {
        var i = 0
        while i < 10 {
            i += 1
        }
    }
}
```

Use a `for` loop to iterate over an array or a `FormList`:

```vellum
script MyScript {

    fun Foo(nums: [Int]) {
        // iterate over array of numbers
        for num in nums {
            // do something with num
        }
    }

    fun Bar(items: FormList) {
        // FormList elements are Form
        for item in items {
            // do something with item
        }
    }
}
```

Optionally bind the zero-based index after the value name:

```vellum
script MyScript {

    fun SumIndexed(nums: [Int]) -> Int {
        var total = 0
        for num, i in nums {
            total += num
            // i is Int (0, 1, 2, ...)
        }
        return total
    }

    fun WalkParallel(recipes: FormList, results: FormList) {
        for recipe, i in recipes {
            var result = results.GetAt(i)
            // ...
        }
    }
}
```

Int ranges use exclusive `..`. Ascending and descending (countdown) are both supported:

```vellum
script MyScript {

    fun CountUp(n: Int) {
        // 0, 1, ..., n-1
        for i in 0..n {
            // ...
        }
    }

    fun CountDown(n: Int) {
        // n, n-1, ..., 1 (excludes 0)
        for i in n..0 {
            // ...
        }
    }
}
```

Equal bounds (`5..5`) produce no iterations. Index bindings (`for x, i in …`) are only for arrays and FormLists, not ranges.

## Jump statements

You can use a `return` statement for early returning from a function:

```vellum
script MyScript : ObjectReference {

    event OnActivate(obj: ObjectReference) {
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

```vellum
script MyScript : ObjectReference {

    fun Foo() {
        var i = 0
        while true {
            if i == 10 {
                break
            }
            i += 1
        }
    }

    fun Bar(actors: [Actor]) {
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
