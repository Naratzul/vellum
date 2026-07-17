# States

A Vellum script (like Papyrus) operates in states. A script can only be in one state at a time. All events and functions run from the current state.

Use the `state` keyword to declare a state.

Put `auto` before `state` and the script will start in this particular state.

The empty state is the default state. All members inside the `script` block are in the empty state.

Only functions and events are allowed inside a `state` block. You can't declare variables and properties here. You can still access the script's variables and properties inside state functions.

```vellum
script MyScript {
    var i = 42
    var x: String {get set} = "Some text"

    fun Foo() {

    }
}

state MyState {
    fun Foo() {
        var local = i
    }
}

auto state InitialState {
    fun Foo() {
        var local = x
    }
}
```

Note: The functions and events must be defined in the empty state with the same signature (name, parameter types, return type). That empty-state declaration may live on **this script or any ancestor** (for example, `ObjectReference` already defines `OnActivate`, so a child script only needs named-state overrides).

## Switching states

You can use `GetState` to get current state and `GoToState` to change the state:

```vellum
script MyScript : ObjectReference {
    event OnActivate(obj: ObjectReference) {
        GoToState("Busy")
    }
}

state Busy {
    event OnActivate(obj: ObjectReference) {
        // do nothing, just wait
        Utility.Wait(5)
        // and switch back to empty state
        GoToState("")
    }
}
```
