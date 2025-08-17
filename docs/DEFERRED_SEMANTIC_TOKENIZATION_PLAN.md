# CPrime Deferred Semantic Tokenization Plan

## Executive Summary

This document outlines a fundamental architectural redesign of the CPrime compiler's tokenization and namespace resolution system. The current approach suffers from namespace pollution due to premature semantic tokenization decisions in Layer 1. The proposed solution separates structural from semantic tokenization, enabling proper namespace-aware exec alias resolution.

## Problem Analysis

### Current Architecture Issues

#### 1. Premature Semantic Tokenization
- **Layer 1** forces keyword/identifier/exec_alias decisions without namespace context
- **Global exec alias registry** converts ALL occurrences of registered names to EXEC_ALIAS tokens
- **Namespace pollution**: Regular identifiers incorrectly converted across different contexts

#### 2. Examples of the Problem
```cpp
exec template<...> vector<T> {};  // Registers "vector" as exec alias globally

namespace std {
    class vector {              // ❌ "vector" becomes EXEC_ALIAS token!
        // Should be IDENTIFIER, not EXEC_ALIAS
    };
}

void func() {
    std::vector<int> v;         // ❌ "vector" becomes EXEC_ALIAS token!
    // Should be regular identifier lookup
}

int vector = 42;               // ❌ "vector" becomes EXEC_ALIAS token!
// Should be variable declaration
```

#### 3. Template System Complexity
- Attempted to handle template patterns in core language layers
- Created invisible exec aliases that bypass Layer 1 detection
- Mixed lexical and semantic concerns inappropriately

## Proposed Solution: Deferred Semantic Tokenization

### Core Principle
**Separate structural tokenization from semantic resolution to enable proper namespace-aware decisions.**

### Architecture Overview

#### Layer 1: Structural-Only Tokenization
```
Sublayer 1a: Comments and whitespace
Sublayer 1b: String literals 
Sublayer 1c: Character literals
Sublayer 1d: Numeric literals
Sublayer 1e: MODIFIED - Keep identifier chunks as ERawToken with unresolved content
```

#### Layer 2: Hierarchical Scope + Namespace Resolution
```
Sublayer 2a: Build scope structure (existing)
Sublayer 2a Enhanced: Hierarchical namespace resolution
- Track namespace creation (namespace, class, struct, ::)
- Each scope gets namespace context vector
- Resolve chunks → keywords/identifiers/exec_aliases with proper context
```

## Implementation Design

### Modified ERawToken Structure
```cpp
struct ERawToken {
    std::string unresolved_content;  // "vector", "namespace", "class", etc.
    // After resolution becomes proper keyword/identifier/exec_alias
    uint32_t line;
    uint32_t column; 
    uint32_t position;
};
```

### Enhanced Scope Structure
```cpp
struct Scope {
    std::vector<std::string> namespace_context;  // e.g., ["std", "containers"]
    Instruction _header;
    std::variant<Instruction, uint32_t> _footer;
    std::vector<std::variant<Instruction, uint32_t>> _instructions;
    uint32_t _parentScopeIndex;
    // ... existing fields
};
```

### Hierarchical Namespace Processing Algorithm

```cpp
void process_scope_hierarchically(Scope& scope, std::vector<std::string>& current_namespace) {
    // 1. Copy current namespace context to this scope
    scope.namespace_context = current_namespace;
    
    // 2. Check if this scope creates new namespace
    if (is_namespace_scope(scope.header)) {
        std::string new_namespace = extract_namespace_name(scope.header);
        current_namespace.push_back(new_namespace);
    }
    
    // 3. Process nested scopes with updated context
    for (auto& nested_scope_ref : scope.instructions) {
        if (is_nested_scope(nested_scope_ref)) {
            uint32_t nested_index = std::get<uint32_t>(nested_scope_ref);
            process_scope_hierarchically(all_scopes[nested_index], current_namespace);
        }
    }
    
    // 4. Resolve chunks to keywords/identifiers with proper namespace context
    resolve_chunks_in_scope(scope);
    
    // 5. Pop namespace if we added one
    if (is_namespace_scope(scope.header)) {
        current_namespace.pop_back();
    }
}
```

### Context-Aware Chunk Resolution

```cpp
TokenType resolve_chunk_with_context(const std::string& chunk_content, 
                                    const std::vector<std::string>& namespace_context) {
    // 1. Check for keywords first (namespace-independent)
    if (is_keyword(chunk_content)) {
        return get_keyword_token_type(chunk_content);
    }
    
    // 2. Check for exec aliases in current namespace context
    if (exec_registry.is_exec_alias_in_namespace(chunk_content, namespace_context)) {
        return EXEC_ALIAS;
    }
    
    // 3. Default to identifier
    return IDENTIFIER;
}
```

### Namespace Detection Logic

#### Namespace-Creating Constructs:
1. **Direct namespace declaration**: `namespace std { ... }`
2. **Class declarations**: `class MyClass { ... }` (creates `MyClass::` namespace)
3. **Struct declarations**: `struct MyStruct { ... }` (creates `MyStruct::` namespace)  
4. **Explicit namespace operators**: `std::vector` (references `std` namespace)

#### Namespace Context Inheritance:
- Child scopes inherit parent namespace context
- Namespace-creating scopes add to context for their children
- Non-namespace scopes pass through parent context unchanged

## Implementation Phases

### Phase 1: Modify Sublayer 1e (Chunk Preservation)
**Goal**: Stop premature keyword/identifier tokenization
**Tasks**:
- [ ] Modify sublayer1e to preserve identifier chunks as ERawToken
- [ ] Enhance ERawToken structure with unresolved_content field
- [ ] Remove keyword/identifier tokenization logic from Layer 1
- [ ] Update Layer 1 tests to expect ERawToken chunks

### Phase 2: Enhance Scope Structure (Namespace Context)
**Goal**: Add namespace tracking to scopes
**Tasks**:
- [ ] Add namespace_context vector to Scope structure
- [ ] Update scope construction to initialize empty namespace context
- [ ] Modify scope serialization for debugging to show namespace context
- [ ] Update scope-related tests

### Phase 3: Implement Hierarchical Processing (Namespace Resolution)
**Goal**: Build namespace-aware scope processing
**Tasks**:
- [ ] Implement hierarchical scope traversal in Sublayer 2a
- [ ] Add namespace detection logic (namespace, class, struct keywords)
- [ ] Implement namespace context inheritance algorithm
- [ ] Add namespace stack management (push/pop)

### Phase 4: Context-Aware Token Resolution (Chunk Resolution)
**Goal**: Resolve ERawToken chunks with proper context
**Tasks**:
- [ ] Implement context-aware chunk resolution function
- [ ] Integrate exec alias registry with namespace context
- [ ] Add keyword detection with namespace awareness
- [ ] Update token creation to use resolved types

### Phase 5: Integration and Testing (System Verification)
**Goal**: Verify complete system functionality
**Tasks**:
- [ ] Test namespace pollution resolution
- [ ] Verify exec alias resolution in different namespace contexts
- [ ] Test template syntax with proper exec alias detection
- [ ] Performance testing of hierarchical processing
- [ ] Integration testing with existing Layer 2+ functionality

## Expected Benefits

### 1. No Namespace Pollution
- Identifiers resolved only in proper namespace context
- No false EXEC_ALIAS conversions for regular identifiers
- Clean separation between different namespace contexts

### 2. Perfect Context
- Each scope knows its exact namespace hierarchy
- Exec alias resolution happens with full namespace information
- Template patterns can be properly distinguished

### 3. Clean Architecture
- Layer 1: Pure structural tokenization (operators, literals, comments)
- Layer 2: Context-aware semantic resolution
- Clear separation of lexical vs semantic concerns

### 4. Flexible Resolution
- Context-aware keyword/identifier/exec_alias decisions
- Support for complex namespace hierarchies
- Proper template and specialization handling

## Trade-offs and Considerations

### Implementation Complexity
- **Increased Layer 2 complexity**: Hierarchical processing adds logic
- **ERawToken handling**: Need to manage unresolved chunks through pipeline
- **Testing complexity**: More context-dependent behavior to verify

### Performance Implications
- **Deferred resolution**: Some processing moves from Layer 1 to Layer 2
- **Hierarchical traversal**: Additional scope processing overhead
- **Namespace lookups**: Context-aware exec alias resolution cost

### Compatibility
- **Token stream changes**: ERawToken chunks instead of immediate resolution
- **Layer interfaces**: Updates needed for chunk handling
- **Testing updates**: Tests need to account for deferred tokenization

## Success Criteria

### 1. Namespace Pollution Eliminated
- [ ] `vector` can exist as both exec alias and class name without conflict
- [ ] Regular identifiers not converted to EXEC_ALIAS incorrectly
- [ ] Namespace-specific exec alias resolution working

### 2. Template System Clean
- [ ] All exec aliases start with `exec` keyword (Layer 1 visible)
- [ ] No invisible alias generation
- [ ] Proper template declaration vs usage distinction

### 3. Architecture Improvements
- [ ] Clean Layer 1 (structural tokenization only)
- [ ] Context-aware Layer 2 (semantic resolution)
- [ ] Maintainable namespace tracking system

## Progress Tracking

**Status**: 📋 Planning Phase
**Next Milestone**: Phase 1 - Modify Sublayer 1e
**Completion Target**: TBD based on implementation complexity

### Implementation Checklist
- [ ] Phase 1: Sublayer 1e modifications
- [ ] Phase 2: Scope structure enhancements  
- [ ] Phase 3: Hierarchical processing implementation
- [ ] Phase 4: Context-aware token resolution
- [ ] Phase 5: Integration testing and verification

---

*This document will be updated as implementation progresses to track actual changes and discoveries during development.*