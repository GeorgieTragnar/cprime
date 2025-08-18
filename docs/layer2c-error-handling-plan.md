# Layer 2C Contextualization Error Handling Implementation Plan

## Overview
Implement a robust error handling system for Layer 2C contextualization with proper separation of concerns between semantic error detection and hierarchical error resolution.

## Architecture Goals
1. **Clean Separation**: Contextualization functions only report semantic issues
2. **Lambda-Based Reporting**: Use function pointers to delegate error composition to callers
3. **Configurable Severity**: Error handler determines severity based on error type configuration
4. **Deferred Source Resolution**: Orchestrator resolves source locations after layer completion
5. **Robust Processing**: Errors don't stop compilation, generate INVALID contextual tokens

## Implementation Plan

### Phase 1: Core Error Infrastructure

#### 1.1 Create Error Enums and Structures
- **File**: `compiler/src/commons/enum/contextualizationError.h`
- **Content**: 
  ```cpp
  enum class ContextualizationErrorType {
      UNSUPPORTED_TOKEN_PATTERN,
      AMBIGUOUS_OPERATOR_CONTEXT,
      UNRESOLVED_IDENTIFIER,
      INVALID_EXPRESSION_STRUCTURE,
      MISSING_TYPE_INFORMATION,
      INCOMPLETE_STATEMENT,
      INVALID_FUNCTION_CALL,
      TYPE_MISMATCH,
      UNDECLARED_VARIABLE
  };

  enum class ErrorSeverity {
      SUPPRESS,
      WARNING, 
      ERROR
  };

  enum class InstructionType {
      HEADER,
      BODY,
      FOOTER
  };
  ```

#### 1.2 Create Error Data Structures
- **File**: `compiler/src/commons/contextualizationError.h`
- **Content**:
  ```cpp
  struct ContextualizationError {
      ContextualizationErrorType error_type;
      std::string extra_info;
      std::vector<uint32_t> token_indices;
      uint32_t scope_index;
      uint32_t instruction_index;
      InstructionType instruction_type;
      // Source location resolved later
      SourceLocation source_location;
  };

  using ErrorReporter = std::function<void(ContextualizationErrorType error_type,
                                          const std::string& extra_info,
                                          const std::vector<uint32_t>& token_indices)>;
  ```

#### 1.3 Create Error Handler
- **File**: `compiler/src/commons/errorHandler.h/.cpp`
- **Features**:
  - Configurable severity mapping
  - Error collection during compilation
  - Source location resolution
  - Rich error reporting with source highlighting

### Phase 2: Contextualization Function Updates

#### 2.1 Update Function Signatures
- **Files**: `layer2/contextualization/*.cpp`
- **Changes**:
  ```cpp
  // Old
  bool contextualize_instruction(Instruction& instruction);

  // New  
  bool contextualize_instruction(Instruction& instruction, ErrorReporter report_error);
  bool contextualize_header(Instruction& header_instruction, ErrorReporter report_error);
  bool contextualize_footer(Instruction& footer_instruction, ErrorReporter report_error);
  ```

#### 2.2 Implement Semantic Pattern Recognition
- **Pattern Detection**:
  - Unsupported token combinations
  - Ambiguous operator contexts (*, &, etc.)
  - Unresolved identifiers
  - Invalid expression structures
- **Error Reporting**: Use ErrorReporter lambda for each detected issue
- **Graceful Handling**: Generate EContextualToken::INVALID for problematic tokens

#### 2.3 Expand EContextualToken Enum
- **File**: `commons/enum/contextualToken.h`
- **Add semantic token types**:
  ```cpp
  enum class EContextualToken : uint32_t {
      INVALID = 0,
      VARIABLE_DECLARATION,
      ASSIGNMENT,
      FUNCTION_CALL,
      CONTROL_FLOW,
      EXPRESSION,
      TYPE_REFERENCE,
      OPERATOR,
      LITERAL_VALUE,
      SCOPE_REFERENCE
  };
  ```

### Phase 3: Sublayer 2C Integration

#### 3.1 Update Sublayer 2C Main Loop
- **File**: `layer2/sublayer2c.cpp`
- **Changes**:
  - Create ErrorReporter lambdas with closure over scope/instruction indices
  - Pass lambdas to contextualization functions
  - Handle instruction type classification (header/body/footer)

#### 3.2 Lambda Implementation Pattern
```cpp
// For each instruction type
auto report_error = [&](ContextualizationErrorType error_type,
                        const std::string& extra_info,
                        const std::vector<uint32_t>& token_indices) {
    error_handler.register_contextualization_error({
        .error_type = error_type,
        .extra_info = extra_info,
        .token_indices = token_indices,
        .scope_index = scope_index,
        .instruction_index = instr_index,
        .instruction_type = current_instruction_type
    });
};
```

### Phase 4: Orchestrator Integration

#### 4.1 Error Handler Integration
- **File**: `orchestrator.cpp`
- **Features**:
  - Pass ErrorHandler to Layer 2
  - After Layer 2 completion: resolve source locations
  - Report errors with rich source context

#### 4.2 Source Location Resolution
- **Process**: Error Handler + Scopes + Streams + StringTable → Source locations
- **Output**: Rich error messages with file/line/column and source highlighting

### Phase 5: Configuration and Testing

#### 5.1 Error Severity Configuration
- **Default policies**: Development vs Production modes
- **Runtime configuration**: Command-line options for error strictness

#### 5.2 Test Implementation
- **Unit tests**: Each contextualization pattern
- **Integration tests**: End-to-end error reporting
- **Error message quality**: Verify helpful, actionable error messages

## Implementation Order
1. Core error infrastructure (enums, structs, error handler)
2. Update contextualization function signatures
3. Implement basic pattern recognition with error reporting
4. Integrate with Sublayer 2C lambda system
5. Add orchestrator error resolution
6. Expand semantic pattern recognition
7. Test and refine error messages

## Benefits
- **Robust Compilation**: Errors don't crash the compiler
- **Rich Error Messages**: Precise source location with context
- **Maintainable Architecture**: Clear separation of concerns
- **Configurable Behavior**: Adjustable error policies
- **Future-Ready**: Foundation for advanced semantic analysis

This plan establishes a solid foundation for comprehensive compiler error handling while maintaining clean architectural boundaries.