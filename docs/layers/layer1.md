# Layer 1 - Raw Tokenization

## Responsibilities
- **Pure lexical analysis**: Convert source text into TokenKind-based RawTokens
- **String interning**: Populate StringTable with identifier/literal strings for deduplication
- **Position tracking**: Accurate line/column information for error reporting
- **No semantic interpretation**: Context-insensitive keyword recognition only

## Input/Output
- **Input**: Raw source code string
- **Output**: Vector of RawTokens + populated StringTable
- **Dependencies**: StringTable for string interning

## Core Components
- **RawTokenizer**: Main tokenization engine with StringTable integration
- **ContextStack**: Context tracking utilities (used by upper layers)
- **RawToken**: Position-aware token with TokenKind enum and string indices