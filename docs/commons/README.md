# Commons - Shared Compiler Components

## Responsibilities
- **StringTable**: Global string deduplication and interning for memory efficiency
- **Debug Utilities**: Unified debug/CLI output functions for all layers 
- **Token Types**: Core TokenKind/ContextualTokenKind enum definitions
- **Structural Types**: Non-templated StructuredTokens with contextualized flag
- **Logger Components**: Component-based selective logging with buffering

## Key Components
- **StringTable**: Thread-safe string interning with index-based access
- **DebugUtils**: Centralized token/scope debug formatting and analysis
- **StructuredTokens**: GPU-friendly flat vector with contextualized enum interpretation
- **Component Loggers**: Per-layer logging with selective buffering control

## Interface
- **Input**: None (utility library)
- **Output**: Shared utilities for all compiler layers
- **Dependencies**: spdlog for logging infrastructure