# Overview

Vellum is a scripting language created for the Creation Kit. Vellum keeps compatibility with the Papyrus ecosystem and offers clearer syntax and tooling.

For now, only the Skyrim PEX subset is supported. Support for Fallout 4 and Starfield is planned.

## Script declaration

See [Inheritance](inheritance.md) for extending base scripts and script attachment.

Put your script inside a **`.vel`** file. Classic hello world example in Vellum:

```vellum
script HelloWorld : ObjectReference {
    event onActivate(actionRef: ObjectReference) {
        Debug.MessageBox("Hello, World!")
    }
}
```

## Functions and events

See [Functions](functions.md).

You can define a function using the `fun` keyword. A function that takes two `Int` parameters and returns an `Int`:

```vellum
fun sum(a: Int, b: Int) -> Int {
    return a + b
}
```

Event definitions start with the `event` keyword:

```vellum
event OnActivate(activator: ObjectReference) {
  PlayAnimation("CoolStuff")
}
```

## Variables

See [Variables](variables.md).

In Vellum, you declare a variable using the `var` keyword followed by the name of the variable:

```vellum
var x: Int = 4
var m: String = "Hello"
```

Vellum supports type inference, so you can omit the type after the variable name:

```vellum
var x = 4 // type `Int` is inferred
```

## Properties

See [Properties](properties.md).

Properties are defined using the `var` keyword followed by name, type, and accessors:

```vellum
var myProperty: String {get set} // defines auto property myProperty with type String

var anotherProperty: Float {get} // defines readonly property anotherProperty with type Float
```

## Arrays

See [Arrays](arrays.md).

Arrays can hold multiple items of the same type:

```vellum
var numbers = [Int; 10] // defines array of 10 Ints

numbers[0] = 5 // assign 5 to the first element of array numbers

var x = numbers[9] // assign array last element value to new variable x
```

You can use the `length` property to get the number of elements:

```vellum
var numbersCount = numbers.length
```

## For loop

See [Control flow](control_flow.md).

Use a `for` loop to iterate over an array:

```vellum
var messages = [String; 5] // array of 5 String objects

// fill messages array somehow
// ...
// ...

for message in messages {
    processMessage(message)
}
```

## While loop

See [Control flow](control_flow.md).

A `while` loop runs the code inside the loop body continuously while the condition is satisfied:

```vellum
var x = 0
while x < 10 {
    x += 1
}
```

You can use `break` and `continue` statements to control the flow of loops (like `for` and `while`).

## If statement

See [Control flow](control_flow.md).

```vellum
// assume value is set
var x: Int
if value > 10 {
    x = 1
} else {
    x = 0
}
```

## Ternary conditional operator

See [Control flow](control_flow.md).

In some cases you can shorten your if statement:

```vellum
// assume value is set
var x = value > 10 ? 1 : 0
```

You can also use it in a return statement:

```vellum
fun max(a: Int, b: Int) -> Int {
    return a > b ? a : b
}
```

## States

See [States](states.md).

```vellum
script MyScript {

}

state MyState {
    event onInit() {

    }

    fun foo() {

    }
}
```

If you want your script to start in a particular state, put the `auto` keyword before `state`:

```vellum
script MyScript {

}

auto state InitialState {

}
```

## Casts

See [Casts](casts.md).

Write **`expression as Type`** when the compiler allows that conversion, for example `obj as Actor` to narrow a reference.
