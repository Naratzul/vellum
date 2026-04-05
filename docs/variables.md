# Variables
Variables can hold a value of literal or object type.

Literal types are:

- `Int`
- `Float`
- `Bool`
- `String`

Variables are private. It means that only that script is aware of them, can write to them and read from them. Other scripts can't access that script's variables.

To declare a variable, use the `var` keyword. You also can specify the type explicitly, but it is optional — the type will be inferred from initializer. 

But if you omit an initializer, you have to specify the type; the variable gets a default value for its type: numeric types `0`, Bool `false`, String `""`, and object and array types default to `none`.

```swift
script MyScript {

    var myVariable = 42 // Int type will be inferred automatically
    var anotherVariable: Int = 100 // explicit type annotation
    var varWithDefaultValue: Int // omit initializer, will be default initialized (see above)

    fun foo() {
        myVariable *= 2
        var localVar = myVariable
    }
}
```

Local variables are those declared inside functions or loops and have a shorter lifetime than member (script) variables.

In addition to literals, you can declare a variable of any available object type. 

```swift
script MyQuestScript: Quest {

    var player: Actor // initialized to none (same as other object and array types)

    event onInit() {
        player = Game.GetPlayer()
    }

    fun foo() {
        if player != none {
            // do some stuff with player
        }
    }
}
```