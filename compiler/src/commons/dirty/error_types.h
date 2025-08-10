#pragma once

#include "tokenizer_state.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace cprime {

/**
 * Error severity levels.
 */
enum class ErrorSeverity {
    Info = 0,       // Informational messages
    Warning = 1,    // Warnings that don't prevent compilation
    Error = 2,      // Errors that prevent successful compilation  
    Fatal = 3       // Fatal errors that stop compilation immediately
};

/**
 * Error categories by source layer.
 */
enum class ErrorLayer {
    Layer0 = 0,     // Input processing errors
    Layer1 = 1,     // Tokenization errors
    Layer2 = 2,     // Structure building errors
    Layer3 = 3,     // Contextualization errors
    Layer4 = 4,     // RAII analysis errors
    ErrorHandler = 99 // Error handler internal errors
};

/**
 * Standard error codes for each layer.
 */
namespace ErrorCodes {
    // Layer 0 - Input Processing
    constexpr uint32_t FILE_NOT_FOUND = 1001;
    constexpr uint32_t FILE_READ_ERROR = 1002;
    constexpr uint32_t INVALID_FILE_EXTENSION = 1003;
    
    // Layer 1 - Tokenization
    constexpr uint32_t UNTERMINATED_STRING = 2001;
    constexpr uint32_t UNTERMINATED_COMMENT = 2002;
    constexpr uint32_t INVALID_NUMBER_FORMAT = 2003;
    constexpr uint32_t UNKNOWN_CHARACTER = 2004;
    
    // Layer 2 - Structure Building
    constexpr uint32_t UNMATCHED_BRACE = 3001;
    constexpr uint32_t MISSING_SEMICOLON = 3002;
    constexpr uint32_t INVALID_SCOPE_NESTING = 3003;
    
    // Layer 3 - Contextualization
    constexpr uint32_t INVALID_CONTEXT_PATTERN = 4001;
    constexpr uint32_t COMPRESSION_FAILED = 4002;
    constexpr uint32_t UNKNOWN_IDENTIFIER_CONTEXT = 4003;
    
    // Layer 4 - RAII Analysis
    constexpr uint32_t INVALID_RAII_PATTERN = 5001;
    constexpr uint32_t DESTRUCTOR_ORDER_CONFLICT = 5002;
    constexpr uint32_t DEFER_SEMANTIC_ERROR = 5003;
}

/**
 * Reference to a scope for error correlation.
 */
struct ScopeReference {
    size_t scope_index;
    std::string scope_description;
    
    ScopeReference(size_t idx, const std::string& desc)
        : scope_index(idx), scope_description(desc) {}
};

/**
 * Standardized error package from layers.
 */
struct LayerError {
    // Error identification
    ErrorLayer source_layer;
    ErrorSeverity severity;
    uint32_t error_code;
    std::string message;
    
    // Source correlation
    std::vector<size_t> related_token_indices;      // References to TokenizerState
    std::optional<ScopeReference> related_scope;    // Reference to scope if applicable
    
    // Additional context
    std::string detailed_description;
    std::vector<std::string> suggested_fixes;
    
    // Construction helpers
    LayerError(ErrorLayer layer, ErrorSeverity sev, uint32_t code, const std::string& msg)
        : source_layer(layer), severity(sev), error_code(code), message(msg) {}
    
    /**
     * Add a token reference for source correlation.
     */
    void add_token_reference(size_t token_index) {
        related_token_indices.push_back(token_index);
    }
    
    /**
     * Set the related scope.
     */
    void set_scope_reference(size_t scope_index, const std::string& description) {
        related_scope = ScopeReference(scope_index, description);
    }
    
    /**
     * Add a suggested fix.
     */
    void add_suggestion(const std::string& suggestion) {
        suggested_fixes.push_back(suggestion);
    }
    
    /**
     * Check if this is a blocking error (prevents compilation).
     */
    bool is_blocking() const {
        return severity >= ErrorSeverity::Error;
    }
    
    /**
     * Get error category name.
     */
    std::string get_layer_name() const {
        switch (source_layer) {
            case ErrorLayer::Layer0: return "Input Processing";
            case ErrorLayer::Layer1: return "Tokenization";
            case ErrorLayer::Layer2: return "Structure Building";
            case ErrorLayer::Layer3: return "Contextualization";
            case ErrorLayer::Layer4: return "RAII Analysis";
            case ErrorLayer::ErrorHandler: return "Error Handler";
            default: return "Unknown";
        }
    }
    
    /**
     * Get severity name.
     */
    std::string get_severity_name() const {
        switch (severity) {
            case ErrorSeverity::Info: return "info";
            case ErrorSeverity::Warning: return "warning";
            case ErrorSeverity::Error: return "error";
            case ErrorSeverity::Fatal: return "fatal";
            default: return "unknown";
        }
    }
};

/**
 * Factory functions for creating common error types.
 */
namespace ErrorFactory {
    
    inline LayerError file_not_found(const std::string& file_path) {
        LayerError error(ErrorLayer::Layer0, ErrorSeverity::Fatal, 
                        ErrorCodes::FILE_NOT_FOUND, 
                        "File not found: " + file_path);
        error.add_suggestion("Check that the file path is correct");
        error.add_suggestion("Ensure the file exists and is readable");
        return error;
    }
    
    inline LayerError unterminated_string(size_t token_index, uint32_t line) {
        LayerError error(ErrorLayer::Layer1, ErrorSeverity::Error,
                        ErrorCodes::UNTERMINATED_STRING,
                        "Unterminated string literal at line " + std::to_string(line));
        error.add_token_reference(token_index);
        error.add_suggestion("Add closing quote to complete the string");
        return error;
    }
    
    inline LayerError unmatched_brace(size_t token_index, const std::string& brace_type) {
        LayerError error(ErrorLayer::Layer2, ErrorSeverity::Error,
                        ErrorCodes::UNMATCHED_BRACE,
                        "Unmatched " + brace_type + " brace");
        error.add_token_reference(token_index);
        error.add_suggestion("Add matching brace to balance the scope");
        return error;
    }
    
    inline LayerError invalid_raii_pattern(size_t scope_index, const std::string& pattern) {
        LayerError error(ErrorLayer::Layer4, ErrorSeverity::Error,
                        ErrorCodes::INVALID_RAII_PATTERN,
                        "Invalid RAII pattern: " + pattern);
        error.set_scope_reference(scope_index, "Function with invalid RAII pattern");
        error.add_suggestion("Check defer statement placement and variable lifetime");
        return error;
    }
}

} // namespace cprime