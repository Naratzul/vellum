# States

A Vellum script (like Papyrus) operates in states. A script can only be in one state at a time. All events and functions run from the current state.

Use the `state` keyword to declare a state.

Put `auto` before `state` and the script will start in this particular state.

The empty state is the default state. All members inside the `script` block are in the empty state.

Only functions and events are allowed inside a `state` block. You can't declare variables and properties here. You can still access the script's variables and properties inside state functions.

```swift
script MyScript {
    var i = 42
    var x: String {get set} = "Some text"

    fun foo() {

    }
}

state MyState {
    fun foo() {
        var local = i
    }
}

auto state InitialState {
    fun foo() {
        var local = x
    }
}
```

Note: The functions and events must be defined in the empty state with the same signature (name, parameter types, return type).

## Switching states

You can use `GetState` to get current state and `GoToState` to change the state:

```swift
script MyScript : ObjectReference {
    event onActivate(obj: ObjectReference) {
        GoToState("Busy")
    }
}

state Busy {
    event onActivate(obj: ObjectReference) {
        // do nothing, just wait
        Utility.Wait(5)
        // and switch back to empty state
        GoToState("")
    }
}
```