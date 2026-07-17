# Arrays

Vellum supports arrays for holding a collection of items of the same type.

## Array usage

To declare an array type, wrap the element type into square brackets `[Type]`. For now, only fixed-size arrays are available.

You can create an array in two ways:

- **Sized allocation** — `[Type; N]` with an `Int` literal size (elements start at type defaults / unset until you assign them).
- **Initializer list** — `[a, b, c, ...]` fills the array from the listed values; length is the number of elements.

To retrieve the number of elements, use the `length` property.

For example:

```vellum
script MyScript {

    var elements: [Int] // default-initialized to none

    fun Foo() {
        elements = [Int; 4] // creates array of 4 Int
        elements[0] = 1
        elements[1] = 2
        elements[2] = 3
        elements[3] = 4

        // same result with an initializer list
        elements = [1, 2, 3, 4]

        var elementsCount = elements.length // 4

        var sum = 0
        for elem in elements {
            sum += elem
        }

        // optional index binding — see [Control flow](control_flow.md)
        for elem, i in elements {
            Debug.Trace($"[{i}] = {elem}")
        }

        Debug.ShowMessage(sum as String) // shows "10"
    }
}
```

## Find methods

Arrays can be searched for specific items using `find` and `rfind` methods.

`find` searches through the array for the element you pass and returns the index of the element if found and -1 otherwise.

`rfind` does the same thing but searches in the backward direction (starts from the end of the array).

Both methods have an optional second parameter for the start index.

```vellum
script MyScript {

    fun Foo() {
        var items = MakeArray()
        if items.find("Vellum") != -1 {
            Debug.Trace("Found!")
        }
    }

    fun MakeArray() -> [String] {
        return ["Hello", "Vellum", "World"]
    }
}
```
