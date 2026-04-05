# Properties

As with [variables](variables.md) properties hold a value. They provide a mechanism for reading, writing and computing the value. Unlike variables, properties are public. It means that other scripts can access the property's value.

To declare a property, use the `var` keyword followed by name, type annotation and accessors. Optionally you can specify an initial value (literals only), otherwise the property will be default-initialized (same defaults as in [variables](variables.md)).

```swift
script GreetingsHelper {
    // default-initialized (to empty string) auto property
    var greetings: String {get set}

    // auto readonly property with initial value
    var greetingsReadonly: String {get} = "Hello, Vellum!"

    // readonly property with custom get accessor
    var greetingsCompute: String { get {
        return "Hello, Vellum"
    }}
}
```

Another script can access our properties:

```swift
script AnotherScript : Quest {
    var greetingsHelper: GreetingsHelper {get set}

    event onInit() {
        Debug.MessageBox(greetingsHelper.greetingsCompute)
    }
}
```

You can create a full property, specifying both accessors. In the setter, `newValue` is implicit (you don't declare it); it is the value being assigned.

```swift
script MyScript {

    var myValue_: Int // private backed variable

    var myProp: Int {
        get { return myValue_ }
        set { myValue_ = newValue }
    }
}
```