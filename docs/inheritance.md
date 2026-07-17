# Inheritance

Inheritance in Vellum allows you to extend base scripts. You can then attach your scripts to game objects and to override game events.

## Extending scripts

Use colon `:` after the script name followed by the base script name to extend a base script:

```vellum
script MyObject : ObjectReference {

}
```

## Overriding members

A child script can override any function/event of any base script in the hierarchy:

```vellum
script MyObject : ObjectReference {
    event OnInit() {

    }

    event OnActivate(obj: ObjectReference) {

    }
}
```

Parent properties are inherited; you cannot redeclare them on the child.

Signature rules are still loose in the compiler (there is no strict check yet) mainly because of the Papyrus case-insensitive nature. This way we can keep compatibility with the Papyrus scripts and extend them in Vellum. Prefer PascalCase in new Vellum code; parent names still match case-insensitively.

## Super expression

`super` gives you access to the base script properties and functions. This example will trace "Base::Bar":

```vellum
script Base {

    fun Bar() {
        Debug.Trace("Base::Bar")
    }
}

script Derived : Base {

    fun Bar() {
        super.Bar()
    }
}
```
