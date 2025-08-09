# Layer 3 - Contextualization

## Responsibilities
- **Zero-copy enum transformation**: In-place TokenKind → ContextualTokenKind conversion
- **Context-sensitive keyword resolution**: runtime/defer/class/exposes disambiguation
- **Contextualization flag management**: Enable ContextualTokenKind interpretation
- **Embarrassingly parallel processing**: GPU-friendly scope-by-scope contextualization

## Input/Output
- **Input**: StructuredTokens (contextualized=false) from Layer 2
- **Output**: StructuredTokens (contextualized=true, in-place transformation)
- **Dependencies**: Layer 2 components, StringTable for identifier resolution

## Core Process
- **Analyze scope context** for each token sequence (signature vs content)
- **Transform enum values** using same uint32_t storage, different interpretation
- **Preserve structure** - no memory reallocation or hierarchy changes
- **Enable contextual access** via contextualized flag for type-safe enum casting