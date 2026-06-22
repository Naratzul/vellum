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

- `libs/vellum-compiler/ast/decl/declaration.h` - Ensure `FunctionDeclaration` tracks native flag
- `libs/vellum-compiler/compiler/pex_object_compiler.cpp` - Set native flag in PEX output
- `libs/vellum-compiler/analyze/semantic_analyzer.cpp` - Validate native functions have no body

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

- `libs/vellum-compiler/lexer/lexer.cpp` or `libs/vellum-compiler/lexer/token.h` - Add `HIDDEN`, `CONDITIONAL` tokens
- `libs/vellum-compiler/parser/parser.cpp` - Consume modifier keywords before `script` / `var` / `property` in declaration parsing
- `libs/vellum-compiler/ast/decl/declaration.h` - Add `hidden`, `conditional` (or flags enum) to script, variable, property declarations
- `libs/vellum-compiler/compiler/pex_object_compiler.cpp` - Set PEX user flags for Hidden/Conditional when emitting objects, variables, properties

### Vellum New Features (Priority: Medium)

#### String Format

**Status**: Not implemented.

**Implementation needed**:

- String interpolation syntax (e.g., `"Hello {name}!"` or `format("Hello {}!", name)`)
- Format string parsing and validation
- Argument count/type checking
- Compilation to PEX string concatenation or format function

**Files to modify**:

- `libs/vellum-compiler/lexer/lexer.cpp` - Handle format placeholders in string literals
- `libs/vellum-compiler/parser/parser.cpp` - Parse format expressions
- `libs/vellum-compiler/ast/expression/expression.h` - Add `StringFormatExpression` class
- `libs/vellum-compiler/analyze/semantic_analyzer.cpp` - Validate format strings
- `libs/vellum-compiler/compiler/pex_function_compiler.cpp` - Compile format expressions

#### State Pattern Matching

**Status**: Not implemented. `MATCH` token exists in lexer but unused.

**Implementation needed**:

- Pattern matching syntax for state transitions
- Match expressions against state names
- Exhaustiveness checking
- Compilation to state transition logic

**Files to modify**:

- `libs/vellum-compiler/parser/parser.cpp` - Add pattern matching syntax
- `libs/vellum-compiler/ast/expression/expression.h` - Add `MatchExpression` class
- `libs/vellum-compiler/analyze/semantic_analyzer.cpp` - Validate pattern matches
- `libs/vellum-compiler/compiler/pex_function_compiler.cpp` - Compile matches

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
- **For-in loops** - `for item in arrayExpr { body }` (requires array-typed collection); `IN` keyword; iterator PEX names via `pexName` / `loopCount` mangling and hidden index local; per-loop `pushScope` so nested loops can reuse the same iterator name; lowered to `ArrayLength`, counter, `CmpLt`/`JmpF`, `ArrayGetElement`, index bump, back-edge `Jmp`; tests in `test/unit/` (parser, semantic, compiler, lexer)
- **Break and continue** - `break` and `continue` inside `while` and for-in loops; semantic validation (only inside loop bodies); PEX `Jmp` patching to loop end or condition; nested loop support
- **Return statements** - Function return with expressions
- **Expression statements** - Standalone expressions
- **Variable statements** - Local variable declarations

### Expressions

- **Binary operations** - All arithmetic (`+`, `-`, `*`, `/`, `%`) and comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`) operators
- **Comparison operand coercion (autocast)** — `commonComparisonType` / `canImplicitlyCast` in `TypeChecker` (Int↔Float → `Float`; script types → common ancestor via `Resolver::isScriptSubtypeOf`); `comparisonOperandType` on comparisons; PEX `Cast` before `Cmp`* when reference operands must unify; tests in `semantic_tests` / `compiler_tests`
- **Logical operations** - Logical AND (`&&`) and OR (`||`)
- **Ternary operator** - `condition ? trueExpr : falseExpr` (`QUES` token); binds looser than `||`/`&&` (C-style: `a || b ? c : d` → `(a || b) ? c : d`), right-associative; bool condition; branch compatibility via `TypeChecker` (`commonTernaryBranchType`, `TernaryBranch` context, Int→Float promotion, `TernaryTypeMismatch`); PEX `JmpF`/`Jmp` and `Cast` when unifying to Float; parse `consume()` errors retain kinds (e.g. `ExpectColon`); tests in `test/unit/` (lexer, parser, semantic, compiler)
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

## Notes

- The codebase has a solid foundation with comprehensive AST, semantic analysis, and PEX compilation infrastructure
- Many PEX instructions already exist in `libs/vellum-compiler/pex/pex_instruction.h` that can be leveraged
- The parser uses recursive descent with good error recovery
- Type system is well-structured and extensible
- Ternary operator has layered unit tests in `test/unit/` (lexer through compiler); add similar coverage for other new features

## Testing Strategy

For each new feature:

1. Add parser tests (syntax validation)
2. Add semantic tests (type checking, error cases)
3. Add compilation tests (PEX output verification)
4. Add integration tests (end-to-end compilation)

