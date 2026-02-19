# Vellum Language Roadmap

## Overview

Vellum is a modern language compiler targeting Papyrus PEX format. This roadmap categorizes features into completed implementations and planned work, organized by priority and feature type.

## Planned Features

### Papyrus Core Features (Priority: High)

#### Native Functions

**Status**: `NATIVE` token and parser support exist, but may need completion.

**Implementation needed**:

- Mark functions as native in AST
- Compile native functions correctly (no body compilation)
- Validate native function signatures

**Files to modify**:

- `src/ast/decl/declaration.h` - Ensure `FunctionDeclaration` tracks native flag
- `src/compiler/pex_object_compiler.cpp` - Set native flag in PEX output
- `src/analyze/semantic_analyzer.cpp` - Validate native functions have no body

#### Hidden and Conditional Modifier Keywords

**Status**: Not implemented.

**Implementation needed**:

- Leading modifier keywords (same style as `static`): `hidden`, `conditional`
- **hidden** – valid on script, property (hides from editor UI)
- **conditional** – valid on script, variable (used by condition system)
- Parse optional modifier list before declaration keyword; reject invalid combinations (e.g. `conditional` on property)
- Add `hidden` / `conditional` flags to AST (ScriptDeclaration, VariableDeclaration, PropertyDeclaration)
- Emit corresponding PEX user-flag bits when compiling so CK/engine behavior is correct

**Files to modify**:

- `src/lexer/lexer.cpp` or `src/lexer/token.h` - Add `HIDDEN`, `CONDITIONAL` tokens
- `src/parser/parser.cpp` - Consume modifier keywords before `script` / `var` / `property` in declaration parsing
- `src/ast/decl/declaration.h` - Add `hidden`, `conditional` (or flags enum) to script, variable, property declarations
- `src/compiler/pex_object_compiler.cpp` - Set PEX user flags for Hidden/Conditional when emitting objects, variables, properties

### Vellum New Features (Priority: Medium)

#### For Loop

**Status**: `FOR` token exists in lexer, but no `ForStatement` in AST.

**Implementation needed**:

- Standard C-style for loop: `for (init; condition; increment) { body }`
- Parse for loop syntax
- Semantic analysis (scope handling for loop variables)
- Compilation to PEX (likely desugars to while loop)

**Files to create/modify**:

- `src/parser/parser.cpp` - Add `forStatement()` method
- `src/ast/statement/statement.h` - Add `ForStatement` class
- `src/analyze/semantic_analyzer.cpp` - Add `visitForStatement`
- `src/compiler/pex_function_compiler.cpp` - Compile for loops

#### Foreach Loop

**Status**: Not implemented.

**Implementation needed**:

- Foreach syntax: `foreach (var item : collection) { body }`
- Support iteration over arrays
- Iterator variable scoping
- Compilation to PEX (likely uses array indexing)

**Files to create/modify**:

- `src/parser/parser.cpp` - Add `foreachStatement()` method
- `src/ast/statement/statement.h` - Add `ForeachStatement` class
- `src/analyze/semantic_analyzer.cpp` - Add `visitForeachStatement`
- `src/compiler/pex_function_compiler.cpp` - Compile foreach loops

#### Ternary Operator

**Status**: Not implemented.

**Implementation needed**:

- Ternary syntax: `condition ? trueExpr : falseExpr`
- Operator precedence (lower than assignment, higher than logical OR)
- Type checking (both branches must be compatible)
- Compilation to PEX (conditional jump instructions)

**Files to modify**:

- `src/parser/parser.cpp` - Add ternary to expression parsing (after logical OR)
- `src/ast/expression/expression.h` - Add `TernaryExpression` class
- `src/analyze/semantic_analyzer.cpp` - Add `visitTernaryExpression`
- `src/compiler/pex_function_compiler.cpp` - Compile ternary expressions

#### String Format

**Status**: Not implemented.

**Implementation needed**:

- String interpolation syntax (e.g., `"Hello {name}!"` or `format("Hello {}!", name)`)
- Format string parsing and validation
- Argument count/type checking
- Compilation to PEX string concatenation or format function

**Files to modify**:

- `src/lexer/lexer.cpp` - Handle format placeholders in string literals
- `src/parser/parser.cpp` - Parse format expressions
- `src/ast/expression/expression.h` - Add `StringFormatExpression` class
- `src/analyze/semantic_analyzer.cpp` - Validate format strings
- `src/compiler/pex_function_compiler.cpp` - Compile format expressions

#### State Pattern Matching

**Status**: Not implemented. `MATCH` token exists in lexer but unused.

**Implementation needed**:

- Pattern matching syntax for state transitions
- Match expressions against state names
- Exhaustiveness checking
- Compilation to state transition logic

**Files to modify**:

- `src/parser/parser.cpp` - Add pattern matching syntax
- `src/ast/expression/expression.h` - Add `MatchExpression` class
- `src/analyze/semantic_analyzer.cpp` - Validate pattern matches
- `src/compiler/pex_function_compiler.cpp` - Compile matches

## Completed Features

### Core Language Structure

- **Script declarations** - Script definition with inheritance (`script Name : Parent`)
- **State declarations** - State blocks with auto-state support (`state Name`, `auto state Name`)
- **State semantics** - State-specific function resolution, PEX state compilation, auto-state handling
- **State functions** - Built-in `GetState()` and `GoToState(name: String)` functions for state management
- **Import system** - Import declarations and resolution (`import ModuleName`)
- **Inheritance** - Script inheritance via `:` syntax

### Declarations

- **Global variables** - Script-level variable declarations with type annotations
- **Local variables** - Function-scoped variables with type inference
- **Properties** - Property declarations with get/set accessors
- **Functions** - Function declarations with parameters and return types
- **Function default arguments** - Default parameter values (`fun foo(x: Int = 5)`), type checking, PEX compilation with default values in function metadata
- **Events** - Event declarations (special function type)
- **Static functions** - Static function declarations

### Statements

- **If/else** - Conditional statements with else branches
- **While loops** - While loop statements
- **Return statements** - Function return with expressions
- **Expression statements** - Standalone expressions
- **Variable statements** - Local variable declarations

### Expressions

- **Binary operations** - All arithmetic (`+`, `-`, `*`, `/`, `%`) and comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`) operators
- **Logical operations** - Logical AND (`&&`) and OR (`||`)
- **Unary operations** - Negation (`-`) and logical NOT (`!`)
- **Cast expressions** - Type casting with `AS` keyword; cast semantic validation (Skyrim: compatible cast matrix, no cast to array), implicit Int→Float in arithmetic and assignment
- **Function calls** - Method and function invocation
- **Property access** - Property get and set operations
- **Assignments** - Variable and property assignments
- **Super expressions** - Parent class method calls (`super.method()`) with full semantic validation and `CallParent` PEX compilation
- **Self expression** - `self` keyword resolving to current script instance, `SelfExpression` in AST, semantic and PEX compilation
- **Array creation** - Array instantiation with type and length (`[Type; length]`)
- **Array operations** - Array indexing (`array[index]`), length property (`array.length`), find methods (`array.find()`, `array.rfind()`)
- **Literals** - Integer, float, boolean, string, and `none` literals
- **Grouping** - Parenthesized expressions
- **Identifier expressions** - Variable and function references

### Type System

- **Type annotations** - Type declarations for variables, parameters, and return types
- **Type inference** - Local variable type inference from initializers
- **Type checking** - Semantic type checking in `TypeChecker`
- **Array types** - Array type support with subtype checking

### Compilation

- **PEX compilation** - Full compilation to PEX bytecode format
- **PEX debug info support** - Optional debug info in PEX output (CLI `--debug-info`/`-g`, default on): modification time, per-function instruction-to-line mapping (object/state/function name, function type, line map); populated from AST token locations during compilation; Skyrim format (no property groups/struct orders)
- **Semantic analysis** - Complete semantic analyzer with type checking
- **Static context validation** - Instance members (variables, properties, non-static functions), `self`, and `super` forbidden inside static functions; `inStaticContext` flag, `Resolver::isInstanceMember`, `CompilerErrorKind::InstanceMemberInStaticContext`, and unit tests
- **Error handling** - Comprehensive error reporting system

## Implementation Priority

### Phase 1: Papyrus Compatibility (Critical)

1. Cast semantic validation

### Phase 2: Compiler Quality

- (PEX debug info support completed)

### Phase 3: Essential Control Flow

1. For loop
2. Ternary operator

### Phase 4: Enhanced Features

1. Foreach loop
2. String format
3. State pattern matching

### Phase 5: Low Priority Features

1. Native functions (completion) - `NATIVE` token and parser support exist, but may need completion for full PEX compilation

## Notes

- The codebase has a solid foundation with comprehensive AST, semantic analysis, and PEX compilation infrastructure
- Many PEX instructions already exist in `src/pex/pex_instruction.h` that can be leveraged
- The parser uses recursive descent with good error recovery
- Type system is well-structured and extensible
- Consider adding tests for each new feature in `test/unit/`

## Testing Strategy

For each new feature:

1. Add parser tests (syntax validation)
2. Add semantic tests (type checking, error cases)
3. Add compilation tests (PEX output verification)
4. Add integration tests (end-to-end compilation)

