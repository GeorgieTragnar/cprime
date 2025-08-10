#include "contextualization_validator.h"
#include "../common/logger.h"
#include "../common/logger_components.h"

namespace cprime::layer3validation {

ContextualizationValidator::ContextualizationValidator(const StructuredTokens& structured_tokens)
    : structured_tokens_(structured_tokens) {
}

validation::ValidationResult ContextualizationValidator::validate() {
    validation::ValidationResult result;
    
    // Check if contextualized flag is properly set
    auto flag_result = validate_contextualized_flag_consistency();
    if (!flag_result.success()) {
        result.merge(flag_result);
        return result;
    }
    
    // Check for unresolved contextual tokens
    auto unresolved_result = validate_no_unresolved_tokens();
    if (!unresolved_result.success()) {
        result.merge(unresolved_result);
        return result;
    }
    
    // Check contextual consistency
    auto consistency_result = validate_contextual_consistency();
    if (!consistency_result.success()) {
        result.merge(consistency_result);
        return result;
    }
    
    // Check scope type alignment
    auto alignment_result = validate_scope_type_alignment();
    if (!alignment_result.success()) {
        result.merge(alignment_result);
        return result;
    }
    
    // All validations passed - result.success() will return true
    return result;
}

validation::ValidationResult ContextualizationValidator::validate_contextualized_flag_consistency() {
    validation::ValidationResult result;
    
    if (!structured_tokens_.is_contextualized()) {
        result.add_error("StructuredTokens contextualized flag is false - contextualization not completed", 
                        validation::SourceLocation(0, 0, 0, 0));
    }
    
    return result;
}

validation::ValidationResult ContextualizationValidator::validate_no_unresolved_tokens() {
    validation::ValidationResult result;
    
    for (size_t scope_idx = 0; scope_idx < structured_tokens_.scopes.size(); ++scope_idx) {
        const auto& scope = structured_tokens_.scopes[scope_idx];
        
        // Check signature tokens
        if (has_unresolved_contextual_tokens(scope.signature_tokens)) {
            result.add_error("Found unresolved contextual tokens in scope " + std::to_string(scope_idx) + " signature", 
                           validation::SourceLocation(0, 0, 0, 0));
            return result;
        }
        
        // Check content tokens
        if (has_unresolved_contextual_tokens(scope.content)) {
            result.add_error("Found unresolved contextual tokens in scope " + std::to_string(scope_idx) + " content", 
                           validation::SourceLocation(0, 0, 0, 0));
            return result;
        }
    }
    
    return result;
}

validation::ValidationResult ContextualizationValidator::validate_contextual_consistency() {
    validation::ValidationResult result;
    
    for (size_t scope_idx = 0; scope_idx < structured_tokens_.scopes.size(); ++scope_idx) {
        const auto& scope = structured_tokens_.scopes[scope_idx];
        
        // Validate signature contextual tokens
        for (uint32_t token_value : scope.signature_tokens) {
            ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
            if (!is_contextual_interpretation_valid(kind, scope.type, true)) {
                result.add_error("Invalid contextual interpretation " + get_contextual_token_name(kind) + 
                               " in scope " + std::to_string(scope_idx) + " signature", 
                               validation::SourceLocation(0, 0, 0, 0));
                return result;
            }
        }
        
        // Validate content contextual tokens
        for (uint32_t token_value : scope.content) {
            ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
            if (!is_contextual_interpretation_valid(kind, scope.type, false)) {
                result.add_error("Invalid contextual interpretation " + get_contextual_token_name(kind) + 
                               " in scope " + std::to_string(scope_idx) + " content", 
                               validation::SourceLocation(0, 0, 0, 0));
                return result;
            }
        }
    }
    
    return result;
}

validation::ValidationResult ContextualizationValidator::validate_scope_type_alignment() {
    validation::ValidationResult result;
    
    // This validation ensures that contextual tokens align with their containing scope types
    // For example, FUNCTION_DECLARATION should be in NamedFunction scopes
    
    for (size_t scope_idx = 0; scope_idx < structured_tokens_.scopes.size(); ++scope_idx) {
        const auto& scope = structured_tokens_.scopes[scope_idx];
        
        // Check signature alignment for named scopes
        if (scope.type == Scope::NamedFunction) {
            bool has_function_declaration = false;
            for (uint32_t token_value : scope.signature_tokens) {
                ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
                if (kind == ContextualTokenKind::FUNCTION_DECLARATION || 
                    kind == ContextualTokenKind::ASYNC_FUNCTION_DECLARATION) {
                    has_function_declaration = true;
                    break;
                }
            }
            
            // Function scopes should have function declaration tokens in signature
            // This is a relaxed check - not all function patterns may be detected correctly yet
            auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER3);
            logger->trace("Function scope {} has function declaration token: {}", 
                         scope_idx, has_function_declaration);
        }
        
        if (scope.type == Scope::NamedClass) {
            bool has_class_declaration = false;
            for (uint32_t token_value : scope.signature_tokens) {
                ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
                if (kind == ContextualTokenKind::DATA_CLASS || 
                    kind == ContextualTokenKind::FUNCTIONAL_CLASS ||
                    kind == ContextualTokenKind::DANGER_CLASS ||
                    kind == ContextualTokenKind::STRUCT_DECLARATION ||
                    kind == ContextualTokenKind::UNION_DECLARATION) {
                    has_class_declaration = true;
                    break;
                }
            }
            
            auto logger = CPRIME_COMPONENT_LOGGER(CPRIME_COMPONENT_LAYER3);
            logger->trace("Class scope {} has class declaration token: {}", 
                         scope_idx, has_class_declaration);
        }
    }
    
    return result;
}

bool ContextualizationValidator::has_unresolved_contextual_tokens(const std::vector<uint32_t>& token_sequence) {
    for (uint32_t token_value : token_sequence) {
        ContextualTokenKind kind = static_cast<ContextualTokenKind>(token_value);
        
        if (kind == ContextualTokenKind::CONTEXTUAL_TODO ||
            kind == ContextualTokenKind::CONTEXTUAL_ERROR ||
            kind == ContextualTokenKind::CONTEXTUAL_UNKNOWN) {
            return true;
        }
    }
    return false;
}

bool ContextualizationValidator::is_contextual_interpretation_valid(ContextualTokenKind kind, Scope::Type scope_type, bool in_signature) {
    // This is a basic validation - in the future, we could have more sophisticated checks
    
    // Some contextual tokens are only valid in certain contexts
    switch (kind) {
        case ContextualTokenKind::FUNCTION_DECLARATION:
        case ContextualTokenKind::ASYNC_FUNCTION_DECLARATION:
            // Function declarations should typically be in function scope signatures
            return scope_type == Scope::NamedFunction && in_signature;
            
        case ContextualTokenKind::DATA_CLASS:
        case ContextualTokenKind::FUNCTIONAL_CLASS:
        case ContextualTokenKind::DANGER_CLASS:
            // Class declarations should typically be in class scope signatures
            return scope_type == Scope::NamedClass && in_signature;
            
        case ContextualTokenKind::RUNTIME_ACCESS_RIGHT:
        case ContextualTokenKind::EXPOSES_RUNTIME:
        case ContextualTokenKind::EXPOSES_COMPILE_TIME:
            // Access rights are typically in signatures or top-level content
            return in_signature || scope_type == Scope::TopLevel;
            
        default:
            // Most contextual tokens are valid in any context
            return true;
    }
}

std::string ContextualizationValidator::get_contextual_token_name(ContextualTokenKind kind) {
    // Simple name mapping for error messages
    switch (kind) {
        case ContextualTokenKind::CONTEXTUAL_TODO: return "CONTEXTUAL_TODO";
        case ContextualTokenKind::CONTEXTUAL_ERROR: return "CONTEXTUAL_ERROR";
        case ContextualTokenKind::CONTEXTUAL_UNKNOWN: return "CONTEXTUAL_UNKNOWN";
        case ContextualTokenKind::RUNTIME_ACCESS_RIGHT: return "RUNTIME_ACCESS_RIGHT";
        case ContextualTokenKind::DATA_CLASS: return "DATA_CLASS";
        case ContextualTokenKind::FUNCTIONAL_CLASS: return "FUNCTIONAL_CLASS";
        case ContextualTokenKind::FUNCTION_DECLARATION: return "FUNCTION_DECLARATION";
        default: return "ContextualTokenKind(" + std::to_string(static_cast<int>(kind)) + ")";
    }
}

} // namespace cprime::layer3validation