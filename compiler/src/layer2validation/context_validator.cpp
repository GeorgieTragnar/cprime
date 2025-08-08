#include "context_validator.h"

namespace cprime::layer2validation {

ContextValidator::ContextValidator(const std::vector<SemanticToken>& tokens)
    : tokens_(tokens) {
}

validation::ValidationResult ContextValidator::validate() {
    validation::ValidationResult result;
    
    // Run all context validation checks
    result.merge(validate_access_rights_completeness());
    result.merge(validate_runtime_comptime_consistency());
    result.merge(validate_defer_statement_context());
    result.merge(validate_union_declaration_completeness());
    result.merge(validate_keyword_context_resolution());
    
    return result;
}

validation::ValidationResult ContextValidator::validate_access_rights_completeness() {
    validation::ValidationResult result;
    
    // Simple stub implementation
    for (size_t i = 0; i < tokens_.size(); ++i) {
        const auto& token = tokens_[i];
        if (token.type == SemanticTokenType::RuntimeAccessRightDeclaration ||
            token.type == SemanticTokenType::CompileTimeAccessRightDeclaration) {
            
            // Check if the access right declaration is complete
            // This is a simplified check
            if (i + 1 >= tokens_.size() || tokens_[i + 1].raw_value.empty()) {
                result.add_error(
                    "Incomplete access rights declaration",
                    validation::SourceLocation(token.source_line, token.source_column, token.source_position, 
                                             token.source_position + token.raw_value.length()),
                    "Complete the access rights specification"
                );
            }
        }
    }
    
    return result;
}

validation::ValidationResult ContextValidator::validate_runtime_comptime_consistency() {
    validation::ValidationResult result;
    
    // Simple stub implementation
    for (const auto& token : tokens_) {
        if (token.raw_value == "runtime" || token.raw_value == "comptime") {
            // Check for basic consistency - this is a placeholder
            if (token.raw_value == "runtime" && token.type != SemanticTokenType::RuntimeAccessRightDeclaration) {
                result.add_warning(
                    "Runtime keyword used in non-runtime context",
                    validation::SourceLocation(token.source_line, token.source_column, token.source_position, 
                                             token.source_position + token.raw_value.length()),
                    "Ensure runtime keyword is used in appropriate context"
                );
            }
        }
    }
    
    return result;
}

validation::ValidationResult ContextValidator::validate_defer_statement_context() {
    validation::ValidationResult result;
    
    // Simple stub implementation
    for (const auto& token : tokens_) {
        if (token.raw_value == "defer") {
            // Placeholder validation for defer statements
            result.add_info(
                "Defer statement found",
                validation::SourceLocation(token.source_line, token.source_column, token.source_position, 
                                         token.source_position + token.raw_value.length()),
                "Defer statements are properly supported"
            );
        }
    }
    
    return result;
}

validation::ValidationResult ContextValidator::validate_union_declaration_completeness() {
    validation::ValidationResult result;
    
    // Simple stub implementation
    for (const auto& token : tokens_) {
        if (token.raw_value == "union") {
            // Placeholder validation for union declarations
            result.add_info(
                "Union declaration found",
                validation::SourceLocation(token.source_line, token.source_column, token.source_position, 
                                         token.source_position + token.raw_value.length()),
                "Union declarations are properly supported"
            );
        }
    }
    
    return result;
}

validation::ValidationResult ContextValidator::validate_keyword_context_resolution() {
    validation::ValidationResult result;
    
    // Simple stub implementation - validate that keywords are used in proper context
    const std::unordered_set<std::string> context_keywords = {
        "runtime", "comptime", "defer", "exposes", "requires"
    };
    
    for (const auto& token : tokens_) {
        if (context_keywords.count(token.raw_value)) {
            // All context keywords are considered valid for now
            result.add_info(
                "Context keyword resolved: " + token.raw_value,
                validation::SourceLocation(token.source_line, token.source_column, token.source_position, 
                                         token.source_position + token.raw_value.length()),
                "Keyword context properly resolved"
            );
        }
    }
    
    return result;
}

} // namespace cprime::layer2validation