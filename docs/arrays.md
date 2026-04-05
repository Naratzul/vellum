# Arrays

Vellum supports arrays for holding a collection of items of the same type.

## Array usage

To declare an array type, wrap the element type into square brackets `[Type]`. For now, only fixed-size arrays are available. It means that you have to specify the array size at compile time using an `Int` literal.

To retrieve the number of elements, use the `length` property.

For example:

```swift
script MyScript {

    var elements: [Int] // default-initialized to none

    fun foo() {
        elements = [Int; 4] // creates array of 4 Int
        elements[0] = 1
        elements[1] = 2
        elements[2] = 3
        elements[3] = 4

        var elementsCount = elements.length // 4

        var sum = 0
        for elem in elements {
            sum += elem
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

```swift
script MyScript {

    fun foo() {
        var items = makeArray()
        if items.find("Vellum") != -1 {
            Debug.Trace("Found!")
        }
    }

    fun makeArray() -> [String] {
        var items = [String; 3]

        items[0] = "Hello"
        items[1] = "Vellum"
        items[2] = "World"

        return items
    }
}
```