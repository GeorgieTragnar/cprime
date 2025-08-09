# Layer 2 - Structure Building

## Responsibilities
- **Cache-and-boundary methodology**: Token accumulation until delimiters (`;`, `{`, `}`)
- **Scope hierarchy creation**: Flat vector with index-based parent-child relationships
- **Pure structural patterns**: Bracket matching and scope type detection only
- **No contextualization**: Stores raw TokenKind values in uint32_t format

## Input/Output
- **Input**: Vector of RawTokens + StringTable from Layer 1
- **Output**: StructuredTokens (contextualized=false)
- **Dependencies**: Layer 1 components, StringTable

## Core Algorithm
- **Cache tokens** until boundary (`;` → instruction, `{` → scope entry, `}` → scope exit)
- **Determine scope types** from cached signature patterns (class, function, control flow)
- **Build flat hierarchy** using index-based scope references for GPU compatibility