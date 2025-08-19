# Exec Usage Type System Implementation Plan

## Overview

This document outlines the implementation plan for adding explicit usage type declarations to the exec alias system in the CPrime compiler. This addresses the critical challenge of linking generated code back to call sites appropriately.

## Problem Statement

Currently, exec aliases can generate three distinct code patterns, but call sites are syntactically identical:

```cpp
exec_alias<params>();  // Could be injection, function generation, or type generation
```

The compiler needs to know how to integrate the generated code:
- **Code Injection**: Direct substitution at call site
- **Function Generation**: Create function scope + function call
- **Type Generation**: Create type scope + type declaration

## Solution: Explicit Usage Type Declaration

Instead of analyzing generated code to guess intent, Lua scripts will explicitly declare the usage type of generated code.

## Proposed Lua API

### Option A: Simple String Pair
```lua
exec assert<condition> {
    return "if(!(" + condition + ")) abort();", "injection"
}

exec logger<level> { 
    return "void log_" + level + "() { ... }", "function"
}

exec vector<T> {
    return "struct Vector_" + T + " { ... };", "type"
}
```

### Option B: Structured Return (Recommended)
```lua
exec logger<level> {
    return {
        code = "void log_" + level + "() { print('Level: " + level + "'); }",
        type = "function",
        name = "log_" + level  -- Optional metadata
    }
}
```

## Core Usage Types

### 1. `"injection"` - Code Injection
- **Purpose**: Direct code substitution at call site
- **Example**: Macro-like expansions, assertions, debug code
- **Integration**: Replace exec call with generated code directly

### 2. `"function"` - Function Generation
- **Purpose**: Generate function definition + replace call with function call
- **Example**: Template-like function generation
- **Integration**: Create new function scope, replace call site with function call

### 3. `"type"` - Type Generation  
- **Purpose**: Generate type definition + replace call with type reference
- **Example**: Generic type instantiation
- **Integration**: Create new type scope, register in namespace

## Implementation Plan

### Phase 1: Update Lua Execution Interface
1. **Modify ExecutableLambda::execute()** to handle structured returns
2. **Parse Lua return values** to extract code and type
3. **Validate usage types** against known patterns
4. **Store type information** alongside generated code

### Phase 2: Type-Specific Code Integration
5. **Injection Handler**: Direct code substitution at call site
6. **Function Handler**: Create function scope + function call replacement
7. **Type Handler**: Create type scope + namespace registration

### Phase 3: Error Handling & Validation
8. **Type validation**: Ensure returned type is known
9. **Consistency checking**: Optional validation of code vs declared type
10. **Clear error messages**: Pinpoint issues in exec script returns

## Benefits

### 1. Explicit Intent
- Script authors know exactly what they're generating
- No ambiguity about generated code usage
- Clear separation of concerns

### 2. Simple Implementation
- Clear dispatch logic based on type
- No fragile parsing or guessing required
- Robust error handling

### 3. Extensible Design
- Easy to add new usage patterns (template, macro, etc.)
- Future-proof architecture
- Maintainable codebase

### 4. Developer Experience
- Clear error messages
- Debuggable intent
- Self-documenting code

## Migration Strategy

### Backward Compatibility
- Current scripts return single string → assume "injection" type
- New scripts use structured return with explicit types
- Gradual migration of existing scripts

### Testing Strategy
- Test all three usage types with comprehensive examples
- Verify proper code generation and linking
- Validate error handling for invalid types

## Technical Architecture

### Data Structures
```cpp
struct ExecResult {
    std::string code;
    std::string type;
    std::map<std::string, std::string> metadata;  // Optional
};
```

### Processing Flow
```cpp
// 1. Execute Lua script
ExecResult result = executable_lambda.execute(parameters);

// 2. Dispatch based on type
if (result.type == "injection") {
    inject_code_at_call_site(result.code);
} else if (result.type == "function") {
    create_function_scope(result.code);
    replace_call_with_function_call();
} else if (result.type == "type") {
    create_type_scope(result.code);
    register_type_in_namespace();
}
```

## Future Extensions

### Additional Usage Types
- **`"template"`**: Generate template definitions
- **`"macro"`**: Generate preprocessor-like macros
- **`"namespace"`**: Generate namespace definitions
- **`"import"`**: Generate import/include statements

### Enhanced Metadata
```lua
return {
    code = "void func() { ... }",
    type = "function",
    name = "custom_function_name",
    namespace = "target_namespace",
    debug_info = "Generated from pattern X"
}
```

## Success Criteria

1. **Functional**: All three usage types working correctly
2. **Robust**: Clear error handling and validation
3. **Extensible**: Easy to add new patterns
4. **Maintainable**: Clean, documented code
5. **Compatible**: Backward compatibility maintained

## Timeline

- **Phase 1**: Lua execution interface updates
- **Phase 2**: Type-specific handlers implementation
- **Phase 3**: Error handling and validation
- **Testing**: Comprehensive test suite
- **Documentation**: Developer documentation and examples

---

*This plan establishes a robust foundation for the exec system that can evolve with the language while maintaining clear semantics and developer experience.*