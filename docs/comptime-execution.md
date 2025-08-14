# CPrime Comptime Execution: The Universal Language Extension Mechanism

## Overview

**Comptime execution is CPrime's revolutionary approach to language extensibility - a keyword aliasing system that intercepts compilation between contextualization and RAII insertion, allowing library code to transform the AST as if it were a built-in language feature.**

This system fundamentally changes how programming languages are architected. Instead of requiring compiler modifications for new language features, comptime enables libraries to extend the language syntax, create new keywords, inject new types, and resolve complex syntax transformations at compile-time.

## Core Concept: Comptime as Compiler Hook

```cpp
comptime {
    // This block executes during compilation
    // It can read the fully-contextualized AST
    // It can inject, modify, or remove code
    // It acts as a "language feature factory"
}
```

The key insight: **Most "language features" are just patterns of code transformation that happen during compilation. Comptime makes these transformations programmable.**

## The Fundamental Architecture

### Compilation Pipeline Integration

```
Source Code → Tokenize → Parse → Contextualize → [COMPTIME LAYER] → Validate → RAII → Optimize → Codegen
                                                         ↑
                                        Library "language features" execute here
```

The comptime layer sits between contextualization and validation, giving library code access to:

1. **Fully parsed AST** - Complete syntax tree with all constructs identified
2. **Complete type information** - All types resolved and available for manipulation
3. **Context resolution** - All identifiers and scopes properly resolved
4. **Pre-validation state** - Can inject code that will be validated in next phase

### Integration with Existing CPrime Architecture

Comptime execution integrates seamlessly with CPrime's existing systems:

- **Three-Class System**: Comptime can generate Data, Functional, and Danger classes
- **Signal Handling**: Stage failures and transformations can use signal integration
- **Memory Management**: Generated code follows RAII principles
- **Runtime/Comptime Keywords**: Uses existing performance indication system

## Part 1: Absolute Core - What Must Be Built-In

### 1.1 Memory Model & Object Lifetime

```cpp
// These CANNOT be implemented via comptime - they're fundamental
- Stack vs heap allocation
- Object construction/destruction ordering  
- Move vs copy semantics
- RAII guarantee (deterministic destruction)
- Reference vs value semantics
```

**Why Core:** The compiler must understand memory layout, lifetime management, and safety for optimization and ABI compatibility.

### 1.2 Control Flow Primitives

```cpp
// Core control flow that affects program counter and stack frames
- if/else conditionals
- while/for loops  
- return/break/continue
- Function calls and returns
- defer (scope guards)
```

**Why Core:** These directly affect program counter, stack frame management, and optimizer understanding of control flow.

### 1.3 Type System Foundation

```cpp
// Fundamental type system that affects memory layout and ABI
- Primitive types (int, float, bool, etc.)
- Pointers/references
- struct/class (data layout)
- unions (tagged)
- Function types
```

**Why Core:** Type checking, memory layout computation, and ABI compatibility depend on these fundamental constructs.

### 1.4 Execution Contexts

```cpp
// Runtime execution models that require scheduler integration
- Functions (calling conventions)
- Coroutines (suspension/resumption)
- Channels (concurrent communication)
- Signals (stack unwinding mechanism)
```

**Why Core:** These require deep runtime support, stack manipulation, and scheduler integration that cannot be library-implemented.

### 1.5 The Comptime System Itself

```cpp
// The comptime execution framework
- comptime blocks
- Code injection (quote/inject)
- AST inspection and manipulation
- Type creation and modification hooks
```

**Why Core:** This is the foundation that enables everything else to be library-implemented.

## Part 2: Library Features - Via Comptime

### 2.1 Template System

Templates become a library feature implemented through comptime transformations:

```cpp
// User writes:
template<typename T>
T max(T a, T b) { return a > b ? a : b; }

// Comptime library transforms to:
comptime {
    registerSyntaxPattern("template<$params> $decl", (match) => {
        return quote {
            comptime {
                onInstantiation($(match.name), $(match.params), 
                               $(match.decl));
            }
        };
    });
}

// Template instantiation
comptime {
    registerTemplate("max", ["T"], (TypeList args) => {
        Type T = args[0];
        injectCode(quote {
            $(T) max($(T) a, $(T) b) { 
                return a > b ? a : b; 
            }
        });
    });
}
```

### 2.2 Interface/Concept Generation

Interface definitions can be automatically generated:

```cpp
// User writes:
interface Drawable {
    fn draw(&self);
    fn bounds(&self) -> Rectangle;
}

// Comptime transforms to:
comptime {
    registerSyntaxPattern("interface $name { $methods }", (match) => {
        return quote {
            // Generate data class for interface contract
            class $(match.name)Contract {
                vtable_ptr: *const $(match.name)VTable,
                constructed_by: $(match.name)Manager,
            }
            
            // Generate functional class for interface operations
            functional class $(match.name)Manager {
                $(generateInterfaceMethods(match.methods))
            }
        };
    });
}
```

### 2.3 Property System

Automatic getter/setter generation:

```cpp
// User writes:
class Person {
    property String name { get; set; }
    property i32 age { get; private set; }
}

// Comptime transforms to:
comptime {
    registerSyntaxPattern("property $type $name { $accessors }", (match) => {
        return quote {
            private: $type _$(match.name);
            public:
            $(if (match.accessors.contains("get")) {
                quote { $type $(match.name)() const { return _$(match.name); } }
            })
            $(if (match.accessors.contains("set")) {
                quote { void set_$(match.name)($type val) { _$(match.name) = val; } }
            })
        };
    });
}
```

### 2.4 Async/Await Syntax

Coroutine syntax sugar implemented via comptime:

```cpp
// User writes:
async fn fetch_data(url: String) -> Result<Data> {
    let response = await http_get(url);
    let data = await parse_response(response);
    Ok(data)
}

// Comptime transforms to:
comptime {
    registerSyntaxPattern("async $func", (match) => {
        return transformToCoroutine(match.func);
    });
    
    registerSyntaxPattern("await $expr", (match) => {
        return quote { co_await $(match.expr); };
    });
}

// Generated coroutine implementation
suspend fn fetch_data(url: String) -> CoroResult<Data> {
    let response = co_await http_get(url);
    let data = co_await parse_response(response);
    CoroResult::Ok(data)
}
```

### 2.5 Pattern Matching

Match expressions as library feature:

```cpp
// User writes:
let result = match(value) {
    Some(x) => x * 2,
    None => 0,
};

// Comptime transforms to:
comptime {
    registerSyntaxPattern("match($expr) { $cases }", (match) => {
        return generateSwitchStatement(match.expr, match.cases);
    });
}

// Generated switch implementation  
let result = {
    auto __match_val = value;
    if (__match_val.is_some()) {
        auto x = __match_val.unwrap();
        x * 2;
    } else {
        0;
    }
};
```

### 2.6 Reflection System

Complete reflection implemented via comptime:

```cpp
// User writes:
let type_info = reflect(MyStruct);

// Comptime transforms to:
comptime {
    registerSyntaxPattern("reflect($type)", (match) => {
        Type t = evaluateType(match.type);
        return quote {
            ReflectionData {
                .name = $(stringLiteral(t.name)),
                .size = $(intLiteral(t.size)),
                .fields = $(generateFieldArray(t)),
                .methods = $(generateMethodArray(t)),
                .inheritance = $(generateAccessRightsArray(t))
            }
        };
    });
}
```

### 2.7 Serialization Framework

Automatic serialization via comptime:

```cpp
// User writes:
@[serializable]
class UserData {
    id: u64,
    name: String,
    email: String,
}

// Comptime transforms to:
comptime {
    registerAttribute("serializable", (Type t) => {
        generateMethod(t, "serialize", quote {
            ByteBuffer buf;
            $(forEach(t.fields, (field) => quote {
                buf.write(this.$(field.name));
            }))
            return buf;
        });
        
        generateMethod(t, "deserialize", quote {
            $(t.name) obj;
            $(forEach(t.fields, (field) => quote {
                obj.$(field.name) = buf.read<$(field.type)>();
            }))
            return obj;
        });
    });
}
```

## The Mechanism: How Comptime Works

### Pattern Matching Engine

The comptime system includes a powerful pattern matching engine for syntax transformation:

```cpp
comptime {
    // Pattern syntax
    "$identifier"     // Matches any identifier
    "$expr"          // Matches any expression
    "$type"          // Matches any type
    "$*"             // Matches zero or more tokens
    "$+"             // Matches one or more tokens
    "literal"        // Matches exact token sequence
    "$($pattern)"    // Grouped pattern
    "$pattern?"      // Optional pattern
    "$pattern|$alt"  // Alternative patterns
}
```

### Keyword Registration System

```cpp
comptime {
    struct SyntaxHook {
        string pattern;           // "template<$params> $decl"
        CompilePhase phase;       // When to apply transformation
        Scope scope;             // Where the pattern is valid
        TransformFn transform;   // Transformation function
        Priority priority;       // Resolution order
    };
    
    registerSyntax(SyntaxHook {
        .pattern = "foreach($var : $container) { $body }",
        .phase = CompilePhase::BeforeValidation,
        .scope = Scope::Statement,
        .transform = generateForeachLoop,
        .priority = Priority::High
    });
}
```

### Code Generation (Quote/Unquote System)

The quote/unquote system enables structured code generation:

```cpp
comptime {
    // quote creates AST nodes
    auto code = quote {
        int x = $(computed_value);           // $ unquotes expression
        $(generateStatements(statements));   // $ unquotes statement list
        for (int i = 0; i < $(count); ++i) { // $ in any position
            $(body)                         // $ unquotes block
        }
    };
    
    // inject places generated code in parent scope
    injectCode(code);
    
    // Alternative: return for expression context
    return code;
}
```

### Type System Manipulation

Comptime can inspect and modify the type system:

```cpp
comptime {
    // Read type information
    Type t = typeof(expression);
    TypeList fields = getFields(t);
    MethodList methods = getMethods(t);
    AccessRightsList access = getAccessRights(t);
    
    // Modify existing types
    addMethod(t, "toString", generateToStringImpl(t));
    addField(t, "id", TypeRef::u64());
    addAccessRight(t, "DebugOps", fields);
    
    // Create new types
    Type newType = createDataClass("Generated", fields);
    Type funcType = createFunctionalClass("GeneratedOps", methods);
    
    // Generate interface contracts
    Interface contract = createInterface("Serializable", {
        method("serialize", TypeRef::function([], TypeRef::vec_u8())),
        method("deserialize", TypeRef::function([TypeRef::vec_u8()], t))
    });
}
```

### Scope and Phase Management

Comptime transformations can be controlled by compilation phase and scope:

```cpp
comptime {
    // Phase-specific transformations
    onPhase(CompilePhase::AfterParsing, () => {
        // Macro-like transformations
    });
    
    onPhase(CompilePhase::AfterContextualization, () => {
        // Template instantiation
    });
    
    onPhase(CompilePhase::BeforeValidation, () => {
        // Code injection and modification
    });
    
    // Scope-specific patterns
    registerSyntax("module $name;", Scope::Global, transformModule);
    registerSyntax("property $prop", Scope::ClassBody, transformProperty);
    registerSyntax("await $expr", Scope::AsyncFunction, transformAwait);
}
```

## Integration with Three-Class System

### Generating Data Classes

Comptime can generate data classes following CPrime's architecture:

```cpp
comptime {
    // Generate data class for ORM entity
    generateDataClass("User", {
        .fields = [
            field("id", u64, semconst),
            field("name", String, mutable),
            field("email", String, mutable),
            field("created_at", Timestamp, semconst)
        ],
        .constructed_by = "UserOps",
        .access_rights = {
            expose("UserService", ["name", "email"]),
            expose("AdminService", ["id", "name", "email", "created_at"])
        }
    });
}
```

### Generating Functional Classes

Comptime can create functional classes with appropriate operations:

```cpp
comptime {
    generateFunctionalClass("UserOps", {
        .constructor = quote {
            fn construct(name: String, email: String) -> User {
                User {
                    id: generate_id(),
                    name,
                    email,
                    created_at: Timestamp::now()
                }
            }
        },
        .methods = [
            method("validate_email", quote {
                fn validate_email(user: &User) -> bool {
                    user.email.contains("@") && user.email.contains(".")
                }
            }),
            method("update_email", quote {
                fn update_email(user: &mut User, new_email: String) -> Result<()> {
                    if !Self::validate_email_format(&new_email) {
                        return Err("Invalid email format");
                    }
                    user.email = new_email;
                    Ok(())
                }
            })
        ],
        .memoize_fields = [
            field("email_validation_cache", "HashMap<String, bool>")
        ]
    });
}
```

### Integration with Signal Handling

Comptime transformations can integrate with CPrime's signal system:

```cpp
comptime {
    registerSyntaxPattern("try_convert($expr)", (match) => {
        return quote {
            catch(CONVERSION_ERROR) {
                $(match.expr)
            } recover {
                match current_signal() {
                    CONVERSION_ERROR(err) => {
                        return Err(ConversionError::from(err));
                    }
                }
            }
        };
    });
}

// Usage:
let result = try_convert(string_to_int("42"));  // Automatically wrapped with signal handling
```

## Advanced Examples

### Domain-Specific Language (DSL) Integration

Comptime enables embedding complete DSLs:

```cpp
// SQL DSL
namespace sql {
    comptime {
        registerSyntaxPattern("SELECT $fields FROM $table WHERE $condition", (match) => {
            return quote {
                QueryBuilder::new()
                    .select($(generateFieldList(match.fields)))
                    .from($(match.table))
                    .where($(transformCondition(match.condition)))
                    .build()
            };
        });
    }
}

// Usage:
import sql;
let users = SELECT name, email FROM users WHERE age > 18;
```

### State Machine Generation

Complex state machines via comptime:

```cpp
comptime {
    registerSyntaxPattern("state_machine $name { $states }", (match) => {
        return quote {
            // Generate state enum
            enum class $(match.name)State {
                $(generateStateEnum(match.states))
            };
            
            // Generate data class for state machine
            class $(match.name) {
                current_state: $(match.name)State,
                $(generateStateFields(match.states))
                
                constructed_by: $(match.name)Manager,
            }
            
            // Generate functional class for transitions
            functional class $(match.name)Manager {
                $(generateTransitionMethods(match.states))
            }
        };
    });
}

// Usage:
state_machine ConnectionSM {
    Disconnected -> Connecting(attempt: u32),
    Connecting -> Connected(socket: TcpSocket) | Failed(error: String),
    Connected -> Disconnected,
}
```

### Test Framework Implementation

Complete testing framework via comptime:

```cpp
comptime {
    registerSyntaxPattern("#[test] fn $name() { $body }", (match) => {
        return quote {
            fn $(match.name)() {
                TestRunner::register_test($(stringLiteral(match.name)), || {
                    $(match.body)
                });
            }
            
            // Auto-registration
            comptime {
                TestRegistry::add($(match.name));
            }
        };
    });
    
    registerSyntaxPattern("assert_eq!($left, $right)", (match) => {
        return quote {
            if ($(match.left) != $(match.right)) {
                TestRunner::fail(format!("Assertion failed: {} != {}", 
                                        $(match.left), $(match.right)));
            }
        };
    });
}
```

## Benefits of This Design

### 1. Minimal Core Language

```cpp
// Core compiler: ~10,000 lines
// - Memory management
// - Control flow  
// - Basic type system
// - Comptime system

// Standard library: ~100,000 lines
// - All "language features" 
// - Templates, interfaces, reflection
// - Async/await, pattern matching
// - Testing, serialization, etc.
```

### 2. Unlimited Extensibility

```cpp
// Users can add any language feature
comptime {
    // Add Python-like list comprehensions
    registerSyntax("[expr for var in container if condition]", 
                   generateListComprehension);
}

// Now this works:
auto evens = [x*2 for x in numbers if x % 2 == 0];

// Add Haskell-like guards
comptime {
    registerSyntax("fn $name($params) | $guards = $exprs", 
                   generateGuardedFunction);
}

// Now this works:
fn factorial(n: i32) 
  | n <= 0 = 1
  | n > 0  = n * factorial(n - 1);
```

### 3. Library Versioning

```cpp
// Different language feature sets via libraries
import std.cpp11;        // C++11-style features
import std.cpp20;        // C++20-style features  
import experimental;     // Cutting-edge features
import company.style;    // Company-specific syntax
```

### 4. Evolution and Innovation

```cpp
// Language evolution happens in libraries, not compiler
// New features can be:
// 1. Prototyped in libraries
// 2. Battle-tested by community
// 3. Standardized when mature
// 4. Implemented without compiler changes

// Innovation becomes decentralized:
// - Framework authors add domain-specific features
// - Language designers experiment with new paradigms  
// - Companies create internal coding standards
// - Community creates specialized tooling
```

### 5. Zero Migration Cost

```cpp
// Backwards compatibility via library versions
module legacy {
    import std.cpp98;    // Old C++ style
    // Legacy code works unchanged
}

module modern {
    import std.latest;   // Latest features
    // New code uses modern features
}

// Both coexist in same codebase
```

## The Boundary Principle

### What MUST Be Core

1. **Memory safety primitives** - Compiler needs to understand memory layout and lifetime
2. **Control flow basics** - Optimizer needs to understand program flow
3. **Fundamental types** - ABI compatibility requires fixed primitive types
4. **Calling conventions** - Function calls need consistent stack behavior
5. **Concurrency primitives** - Runtime scheduler must understand tasks and channels
6. **Comptime system itself** - The foundation that enables everything else

### What CAN Be Library

1. **Syntax sugar** - Transform to core constructs (foreach loops, operators)
2. **Type system extensions** - Build on core types (templates, concepts, traits)
3. **Metaprogramming patterns** - Use comptime for generation (reflection, serialization)
4. **Domain features** - Specialized for specific use cases (SQL, regex, state machines)
5. **Convenience features** - Quality of life improvements (properties, automatic implementations)
6. **Execution models** - Different paradigms (functional programming, reactive systems)

## Implementation Strategy

### Phase 1: Core Compiler

```cpp
// Implement minimal core:
- Memory model (stack/heap, RAII, move/copy)
- Control flow (if, while, for, function calls)
- Type system (primitives, structs, pointers)
- Three-class system enforcement
- Signal handling primitives
- Comptime execution framework
```

### Phase 2: Bootstrap Standard Library

```cpp
// Implement essential features via comptime:
import std.core;

comptime {
    // Templates
    registerTemplateSystem();
    
    // Basic interfaces
    registerInterfaceSystem();
    
    // Common patterns
    registerForEachLoop();
    registerPropertySystem();
    
    // Essential types
    registerOptionalType();
    registerResultType();
}
```

### Phase 3: Full Standard Library

```cpp
// Complete language feature set:
import std;

// Now includes:
// - Full template system
// - Reflection framework
// - Async/await syntax
// - Pattern matching
// - Test framework
// - Serialization
// - And much more...
```

### Phase 4: Community Ecosystem

```cpp
// Community-driven extensions:
import graphics.shaders;     // Shader language embedded in CPrime
import web.html;            // HTML templating syntax  
import database.orm;        // ORM with query language
import ai.models;           // ML model definition syntax
```

## Cross-References

- **[Runtime/Comptime Keywords](runtime-comptime-keywords.md)**: Comptime execution builds on the existing performance indication system, using `comptime` to signal zero-cost transformations
- **[Compilation Model](compilation.md)**: The comptime layer integrates into the existing 3-phase compilation pipeline between contextualization and validation
- **[Three-Class System](three-class-system.md)**: Comptime can generate Data, Functional, and Danger classes while respecting architectural boundaries
- **[Signal Handling](signal-handling.md)**: Comptime transformations can integrate with signal-based error handling and generate signal-aware code
- **[Language Summary](language-summary.md)**: Comptime execution represents a fundamental architectural shift, making CPrime's core language minimal while enabling unlimited library-based extension

## Conclusion

Comptime execution transforms CPrime from a programming language into a **language construction toolkit**. The core language provides the essential semantics that cannot be library-implemented (memory model, control flow, type system fundamentals), while everything else becomes a library concern.

This approach offers:

- **Minimal compiler complexity** - Core language is small and focused
- **Unlimited extensibility** - Libraries can add any conceivable language feature  
- **Community innovation** - Language evolution happens in libraries, not compiler
- **Perfect domain fit** - Each domain can have ideal syntax via custom libraries
- **Zero migration cost** - Old and new features coexist via library versioning
- **Rapid prototyping** - New language features can be implemented and tested quickly

**The key insight: Most "language features" are just patterns of code transformation that happen during compilation. Comptime execution makes these transformations programmable, turning the compiler into a platform for language innovation.**