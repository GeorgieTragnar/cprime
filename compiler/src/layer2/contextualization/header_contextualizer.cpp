#include "header_contextualizer.h"
#include "../../commons/logger.h"
#include "../../commons/enum/token.h"

namespace cprime::layer2_contextualization {

HeaderContextualizer::HeaderContextualizer() {
    setup_header_patterns();
}

bool HeaderContextualizer::token_matches_element(const Token& token, HeaderPatternElement element) {
    // First check if it's a base pattern element
    if (is_base_pattern_element(element)) {
        BasePatternElement base_element = to_base_pattern_element(element);
        return token_matches_base_element(token, base_element);
    }
    
    // Handle header-specific pattern elements
    switch (element) {
        // Function declaration keywords
        case HeaderPatternElement::KEYWORD_FUNC:
            return token._token == EToken::FUNC || token._token == EToken::FUNCTION;
            
        // Type definition keywords
        case HeaderPatternElement::KEYWORD_CLASS:
            return token._token == EToken::CLASS;
        case HeaderPatternElement::KEYWORD_STRUCT:
            return token._token == EToken::STRUCT;
        case HeaderPatternElement::KEYWORD_INTERFACE:
            return token._token == EToken::INTERFACE;
        case HeaderPatternElement::KEYWORD_ENUM:
            return false; // ENUM not in current EToken enum - may need to be added
        case HeaderPatternElement::KEYWORD_TYPEDEF:
            return false; // TYPEDEF not in current EToken enum - may need to be added
            
        // Template keywords
        case HeaderPatternElement::KEYWORD_TEMPLATE:
            return false; // TEMPLATE not in current EToken enum - may need to be added
            
        // Namespace keywords
        case HeaderPatternElement::KEYWORD_NAMESPACE:
            return false; // NAMESPACE not in current EToken enum - may need to be added
            
        // Visibility keywords
        case HeaderPatternElement::KEYWORD_PUBLIC:
            return token._token == EToken::OPEN; // CPrime uses OPEN instead of public
        case HeaderPatternElement::KEYWORD_PRIVATE:
            return token._token == EToken::CLOSED; // CPrime uses CLOSED instead of private
        case HeaderPatternElement::KEYWORD_PROTECTED:
            return false; // PROTECTED not in CPrime - uses OPEN/CLOSED model
        case HeaderPatternElement::KEYWORD_INTERNAL:
            return false; // INTERNAL not in current EToken enum
            
        // Inheritance keywords
        case HeaderPatternElement::KEYWORD_EXTENDS:
            return false; // EXTENDS not in current EToken enum - CPrime may use different syntax
        case HeaderPatternElement::KEYWORD_IMPLEMENTS:
            return token._token == EToken::IMPLEMENTS;
            
        // Import/Export keywords
        case HeaderPatternElement::KEYWORD_IMPORT:
            return false; // IMPORT not in current EToken enum - CPrime may use MODULE system
        case HeaderPatternElement::KEYWORD_EXPORT:
            return false; // EXPORT not in current EToken enum - CPrime may use MODULE system
        case HeaderPatternElement::KEYWORD_FROM:
            return false; // FROM not in current EToken enum
            
        // Special operators for headers
        case HeaderPatternElement::RETURN_TYPE_ARROW:
            return token._token == EToken::ARROW; // -> operator
            
        // Complex pattern elements - N:M mapping patterns
        case HeaderPatternElement::FUNCTION_PARAMETERS:
            return false; // Handled by specialized N:M matching logic
        case HeaderPatternElement::RETURN_TYPE:
            return token._token == EToken::IDENTIFIER || 
                   token._token == EToken::INT8_T || token._token == EToken::INT16_T ||
                   token._token == EToken::INT32_T || token._token == EToken::INT64_T ||
                   token._token == EToken::UINT8_T || token._token == EToken::UINT16_T ||
                   token._token == EToken::UINT32_T || token._token == EToken::UINT64_T ||
                   token._token == EToken::FLOAT || token._token == EToken::DOUBLE ||
                   token._token == EToken::BOOL || token._token == EToken::CHAR ||
                   token._token == EToken::VOID;
        case HeaderPatternElement::FUNCTION_SIGNATURE:
            return false; // Handled by specialized N:M matching logic
        case HeaderPatternElement::TYPE_BODY:
            return false; // Handled by specialized N:M matching logic
        case HeaderPatternElement::TEMPLATE_PARAMETERS:
            return false; // Handled by specialized N:M matching logic
        case HeaderPatternElement::TEMPLATE_CONSTRAINTS:
            return false; // Not implemented - CPrime may not have template constraints
        case HeaderPatternElement::TEMPLATE_SPECIALIZATION:
            return false; // Handled by specialized N:M matching logic
        case HeaderPatternElement::NAMESPACE_PATH:
            return false; // Handled by specialized N:M matching logic
        case HeaderPatternElement::NAMESPACE_ALIAS:
            return false; // Not implemented yet
        case HeaderPatternElement::INHERITANCE_LIST:
            return false; // Handled by specialized N:M matching logic
        case HeaderPatternElement::MODULE_PATH:
            return token._token == EToken::STRING_LITERAL || token._token == EToken::IDENTIFIER;
            
        default:
            return false;
    }
}

bool HeaderContextualizer::is_whitespace_pattern_element(HeaderPatternElement element) {
    // Check base whitespace patterns first
    if (is_base_pattern_element(element)) {
        uint32_t element_value = static_cast<uint32_t>(element);
        
        return element_value == static_cast<uint32_t>(BasePatternElement::OPTIONAL_WHITESPACE) ||
               element_value == static_cast<uint32_t>(BasePatternElement::REQUIRED_WHITESPACE) ||
               element_value == static_cast<uint32_t>(BasePatternElement::SINGLE_WHITESPACE) ||
               element_value == static_cast<uint32_t>(BasePatternElement::MERGED_WHITESPACE);
    }
    
    // Header-specific whitespace patterns (none for now)
    return false;
}

bool HeaderContextualizer::token_matches_base_element(const Token& token, BasePatternElement element) {
    switch (element) {
        // Generic token types
        case BasePatternElement::ANY_IDENTIFIER:
            return token._token == EToken::IDENTIFIER;
        case BasePatternElement::ANY_LITERAL:
            return token._token == EToken::STRING_LITERAL || 
                   token._token == EToken::INT_LITERAL ||
                   token._token == EToken::FLOAT_LITERAL;
        case BasePatternElement::ANY_STRING_LITERAL:
            return token._token == EToken::STRING_LITERAL;
        case BasePatternElement::ANY_INT_LITERAL:
            return token._token == EToken::INT_LITERAL;
            
        // Specific operators and punctuation
        case BasePatternElement::LITERAL_ASSIGN:
            return token._token == EToken::ASSIGN;
        case BasePatternElement::LITERAL_PLUS:
            return token._token == EToken::PLUS;
        case BasePatternElement::LITERAL_MINUS:
            return token._token == EToken::MINUS;
        case BasePatternElement::LITERAL_MULTIPLY:
            return token._token == EToken::MULTIPLY;
        case BasePatternElement::LITERAL_DIVIDE:
            return token._token == EToken::DIVIDE;
        case BasePatternElement::LITERAL_SEMICOLON:
            return token._token == EToken::SEMICOLON;
        case BasePatternElement::LITERAL_COLON:
            return token._token == EToken::COLON;
        case BasePatternElement::LITERAL_COMMA:
            return token._token == EToken::COMMA;
        case BasePatternElement::LITERAL_DOT:
            return token._token == EToken::DOT;
            
        // Brackets and delimiters
        case BasePatternElement::LITERAL_PAREN_L:
            return token._token == EToken::LEFT_PAREN;
        case BasePatternElement::LITERAL_PAREN_R:
            return token._token == EToken::RIGHT_PAREN;
        case BasePatternElement::LITERAL_BRACE_L:
            return token._token == EToken::LEFT_BRACE;
        case BasePatternElement::LITERAL_BRACE_R:
            return token._token == EToken::RIGHT_BRACE;
        case BasePatternElement::LITERAL_BRACKET_L:
            return token._token == EToken::LEFT_BRACKET;
        case BasePatternElement::LITERAL_BRACKET_R:
            return token._token == EToken::RIGHT_BRACKET;
        case BasePatternElement::LITERAL_LESS:
            return token._token == EToken::LESS_THAN;
        case BasePatternElement::LITERAL_GREATER:
            return token._token == EToken::GREATER_THAN;
            
        // Compound operators
        case BasePatternElement::LITERAL_DOUBLE_COLON:
        case BasePatternElement::LITERAL_ARROW:
        case BasePatternElement::LITERAL_PLUS_ASSIGN:
        case BasePatternElement::LITERAL_MINUS_ASSIGN:
            return false; // Not implemented yet
            
        // Complex patterns
        case BasePatternElement::EXPRESSION_TOKENS:
        case BasePatternElement::TYPE_TOKEN_LIST:
        case BasePatternElement::PARAMETER_LIST:
        case BasePatternElement::ARGUMENT_LIST:
            return false; // Not implemented yet
            
        default:
            return false;
    }
}

void HeaderContextualizer::setup_header_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    LOG_INFO("Setting up header contextualization patterns");
    
    setup_function_declaration_patterns();
    setup_type_declaration_patterns();
    setup_template_patterns();
    setup_namespace_patterns();
    setup_visibility_patterns();
    setup_inheritance_patterns();
    setup_import_export_patterns();
    
    LOG_INFO("Header pattern setup complete - {} patterns registered", pattern_count());
}

void HeaderContextualizer::setup_function_declaration_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    LOG_DEBUG("Setting up function declaration patterns");
    
    // Pattern: func identifier()
    // Example: func main()
    HeaderContextualizationPattern simple_function_declaration(
        "simple_function_declaration",
        {HeaderPatternElement::KEYWORD_FUNC, 
         static_cast<HeaderPatternElement>(BasePatternElement::REQUIRED_WHITESPACE),
         static_cast<HeaderPatternElement>(BasePatternElement::ANY_IDENTIFIER), 
         static_cast<HeaderPatternElement>(BasePatternElement::LITERAL_PAREN_L), 
         static_cast<HeaderPatternElement>(BasePatternElement::LITERAL_PAREN_R)},
        {
            ContextualTokenTemplate(EContextualToken::FUNCTION_CALL, {0, 2, 3, 4}, "function declaration"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "function name spacing"),
            ContextualTokenTemplate(EContextualToken::TYPE_REFERENCE, {0}, "func keyword")
        },
        100
    );
    register_pattern(simple_function_declaration);
    
    // Pattern: func identifier() -> return_type
    // Example: func calculate() -> int
    HeaderContextualizationPattern function_with_return_type(
        "function_with_return_type",
        {HeaderPatternElement::KEYWORD_FUNC,
         static_cast<HeaderPatternElement>(BasePatternElement::REQUIRED_WHITESPACE),
         static_cast<HeaderPatternElement>(BasePatternElement::ANY_IDENTIFIER),
         static_cast<HeaderPatternElement>(BasePatternElement::LITERAL_PAREN_L),
         static_cast<HeaderPatternElement>(BasePatternElement::LITERAL_PAREN_R),
         static_cast<HeaderPatternElement>(BasePatternElement::OPTIONAL_WHITESPACE),
         HeaderPatternElement::RETURN_TYPE_ARROW,
         static_cast<HeaderPatternElement>(BasePatternElement::OPTIONAL_WHITESPACE),
         static_cast<HeaderPatternElement>(BasePatternElement::ANY_IDENTIFIER)},
        {
            ContextualTokenTemplate(EContextualToken::FUNCTION_CALL, {0, 2, 3, 4, 6, 8}, "function with return type"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "function name spacing"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {5}, "pre-arrow spacing"),
            ContextualTokenTemplate(EContextualToken::OPERATOR, {6}, "return type arrow"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {7}, "post-arrow spacing"),
            ContextualTokenTemplate(EContextualToken::TYPE_REFERENCE, {8}, "return type")
        },
        120
    );
    register_pattern(function_with_return_type);
    
    LOG_DEBUG("Function declaration patterns registered");
}

void HeaderContextualizer::setup_type_declaration_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    LOG_DEBUG("Setting up type declaration patterns");
    
    // Pattern: class identifier
    // Example: class MyClass
    HeaderContextualizationPattern class_declaration(
        "class_declaration",
        {HeaderPatternElement::KEYWORD_CLASS,
         static_cast<HeaderPatternElement>(BasePatternElement::REQUIRED_WHITESPACE),
         static_cast<HeaderPatternElement>(BasePatternElement::ANY_IDENTIFIER)},
        {
            ContextualTokenTemplate(EContextualToken::TYPE_REFERENCE, {0, 2}, "class definition"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "class name spacing")
        },
        100
    );
    register_pattern(class_declaration);
    
    // Pattern: struct identifier
    // Example: struct Point
    HeaderContextualizationPattern struct_declaration(
        "struct_declaration", 
        {HeaderPatternElement::KEYWORD_STRUCT,
         static_cast<HeaderPatternElement>(BasePatternElement::REQUIRED_WHITESPACE),
         static_cast<HeaderPatternElement>(BasePatternElement::ANY_IDENTIFIER)},
        {
            ContextualTokenTemplate(EContextualToken::TYPE_REFERENCE, {0, 2}, "struct definition"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "struct name spacing")
        },
        100
    );
    register_pattern(struct_declaration);
    
    LOG_DEBUG("Type declaration patterns registered");
}

void HeaderContextualizer::setup_template_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    LOG_DEBUG("Setting up template patterns");
    
    // TODO: Implement template patterns when template parsing is ready
    // For now, just log that this section exists
    
    LOG_DEBUG("Template patterns setup (placeholder)");
}

void HeaderContextualizer::setup_namespace_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    LOG_DEBUG("Setting up namespace patterns");
    
    // Pattern: namespace identifier
    // Example: namespace utils
    HeaderContextualizationPattern namespace_declaration(
        "namespace_declaration",
        {HeaderPatternElement::KEYWORD_NAMESPACE,
         static_cast<HeaderPatternElement>(BasePatternElement::REQUIRED_WHITESPACE),
         static_cast<HeaderPatternElement>(BasePatternElement::ANY_IDENTIFIER)},
        {
            ContextualTokenTemplate(EContextualToken::TYPE_REFERENCE, {0, 2}, "namespace definition"),
            ContextualTokenTemplate(EContextualToken::WHITESPACE, {1}, "namespace name spacing")
        },
        100
    );
    register_pattern(namespace_declaration);
    
    LOG_DEBUG("Namespace patterns registered");
}

void HeaderContextualizer::setup_visibility_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    LOG_DEBUG("Setting up visibility patterns");
    
    // TODO: Implement visibility patterns when needed
    
    LOG_DEBUG("Visibility patterns setup (placeholder)");
}

void HeaderContextualizer::setup_inheritance_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    LOG_DEBUG("Setting up inheritance patterns");
    
    // TODO: Implement inheritance patterns when needed
    
    LOG_DEBUG("Inheritance patterns setup (placeholder)");
}

void HeaderContextualizer::setup_import_export_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    LOG_DEBUG("Setting up import/export patterns");
    
    // TODO: Implement import/export patterns when needed
    
    LOG_DEBUG("Import/export patterns setup (placeholder)");
}

PatternMatchResult HeaderContextualizer::try_match_pattern(const std::vector<Token>& tokens, 
                                                          size_t start_pos, 
                                                          const BaseContextualizationPattern<HeaderPatternElement>& pattern) {
    // Check if pattern contains complex N:M elements that need specialized handling
    for (const auto& element : pattern.token_pattern) {
        switch (element) {
            case HeaderPatternElement::FUNCTION_PARAMETERS:
                return try_match_function_parameters(tokens, start_pos);
            case HeaderPatternElement::TYPE_BODY:
                return try_match_type_body(tokens, start_pos);
            case HeaderPatternElement::TEMPLATE_PARAMETERS:
                return try_match_template_parameters(tokens, start_pos);
            case HeaderPatternElement::NAMESPACE_PATH:
                return try_match_namespace_path(tokens, start_pos);
            case HeaderPatternElement::INHERITANCE_LIST:
                return try_match_inheritance_list(tokens, start_pos);
            case HeaderPatternElement::FUNCTION_SIGNATURE:
                return try_match_function_signature(tokens, start_pos);
            default:
                // Continue to base class implementation for simple patterns
                break;
        }
    }
    
    // Use base class implementation for simple patterns
    return BaseContextualizer<HeaderPatternElement>::try_match_pattern(tokens, start_pos, pattern);
}

PatternMatchResult HeaderContextualizer::try_match_function_parameters(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    
    if (start_pos >= tokens.size() || tokens[start_pos]._token != EToken::LEFT_PAREN) {
        return PatternMatchResult::failure("Expected opening parenthesis for function parameters");
    }
    
    size_t pos = start_pos + 1;  // Skip opening paren
    std::vector<uint32_t> parameter_token_indices;
    parameter_token_indices.push_back(tokens[start_pos]._tokenIndex);
    
    // Find matching closing parenthesis, collecting parameter tokens
    int paren_depth = 1;
    while (pos < tokens.size() && paren_depth > 0) {
        if (tokens[pos]._token == EToken::LEFT_PAREN) {
            paren_depth++;
        } else if (tokens[pos]._token == EToken::RIGHT_PAREN) {
            paren_depth--;
        }
        
        parameter_token_indices.push_back(tokens[pos]._tokenIndex);
        pos++;
    }
    
    if (paren_depth > 0) {
        return PatternMatchResult::failure("Unmatched parenthesis in function parameters");
    }
    
    // Generate contextual tokens for the parameter list
    std::vector<ContextualToken> contextual_tokens;
    
    ContextualToken param_list_token;
    param_list_token._contextualToken = EContextualToken::FUNCTION_CALL;
    param_list_token._parentTokenIndices = parameter_token_indices;
    contextual_tokens.push_back(param_list_token);
    
    LOG_DEBUG("Matched function parameters consuming {} tokens", pos - start_pos);
    return PatternMatchResult::success(pos - start_pos, contextual_tokens);
}

PatternMatchResult HeaderContextualizer::try_match_type_body(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    
    if (start_pos >= tokens.size() || tokens[start_pos]._token != EToken::LEFT_BRACE) {
        return PatternMatchResult::failure("Expected opening brace for type body");
    }
    
    size_t pos = start_pos + 1;  // Skip opening brace
    std::vector<uint32_t> body_token_indices;
    body_token_indices.push_back(tokens[start_pos]._tokenIndex);
    
    // Find matching closing brace, collecting body tokens
    int brace_depth = 1;
    while (pos < tokens.size() && brace_depth > 0) {
        if (tokens[pos]._token == EToken::LEFT_BRACE) {
            brace_depth++;
        } else if (tokens[pos]._token == EToken::RIGHT_BRACE) {
            brace_depth--;
        }
        
        body_token_indices.push_back(tokens[pos]._tokenIndex);
        pos++;
    }
    
    if (brace_depth > 0) {
        return PatternMatchResult::failure("Unmatched brace in type body");
    }
    
    // Generate contextual tokens for the type body
    std::vector<ContextualToken> contextual_tokens;
    
    ContextualToken body_token;
    body_token._contextualToken = EContextualToken::TYPE_REFERENCE;
    body_token._parentTokenIndices = body_token_indices;
    contextual_tokens.push_back(body_token);
    
    LOG_DEBUG("Matched type body consuming {} tokens", pos - start_pos);
    return PatternMatchResult::success(pos - start_pos, contextual_tokens);
}

PatternMatchResult HeaderContextualizer::try_match_template_parameters(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    
    if (start_pos >= tokens.size() || tokens[start_pos]._token != EToken::LESS_THAN) {
        return PatternMatchResult::failure("Expected opening angle bracket for template parameters");
    }
    
    size_t pos = start_pos + 1;  // Skip opening angle bracket
    std::vector<uint32_t> template_token_indices;
    template_token_indices.push_back(tokens[start_pos]._tokenIndex);
    
    // Find matching closing angle bracket, collecting template parameter tokens
    int angle_depth = 1;
    while (pos < tokens.size() && angle_depth > 0) {
        if (tokens[pos]._token == EToken::LESS_THAN) {
            angle_depth++;
        } else if (tokens[pos]._token == EToken::GREATER_THAN) {
            angle_depth--;
        }
        
        template_token_indices.push_back(tokens[pos]._tokenIndex);
        pos++;
    }
    
    if (angle_depth > 0) {
        return PatternMatchResult::failure("Unmatched angle bracket in template parameters");
    }
    
    // Generate contextual tokens for the template parameters
    std::vector<ContextualToken> contextual_tokens;
    
    ContextualToken template_token;
    template_token._contextualToken = EContextualToken::TYPE_REFERENCE;
    template_token._parentTokenIndices = template_token_indices;
    contextual_tokens.push_back(template_token);
    
    LOG_DEBUG("Matched template parameters consuming {} tokens", pos - start_pos);
    return PatternMatchResult::success(pos - start_pos, contextual_tokens);
}

PatternMatchResult HeaderContextualizer::try_match_namespace_path(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    
    if (start_pos >= tokens.size() || tokens[start_pos]._token != EToken::IDENTIFIER) {
        return PatternMatchResult::failure("Expected identifier for namespace path");
    }
    
    size_t pos = start_pos;
    std::vector<uint32_t> namespace_token_indices;
    
    // Match pattern: identifier (:: identifier)*
    while (pos < tokens.size()) {
        if (tokens[pos]._token == EToken::IDENTIFIER) {
            namespace_token_indices.push_back(tokens[pos]._tokenIndex);
            pos++;
            
            // Check for scope resolution operator
            if (pos < tokens.size() && tokens[pos]._token == EToken::SCOPE_RESOLUTION) {
                namespace_token_indices.push_back(tokens[pos]._tokenIndex);
                pos++;
            } else {
                break; // End of namespace path
            }
        } else {
            break;
        }
    }
    
    if (namespace_token_indices.empty()) {
        return PatternMatchResult::failure("No valid namespace path found");
    }
    
    // Generate contextual tokens for the namespace path
    std::vector<ContextualToken> contextual_tokens;
    
    ContextualToken namespace_token;
    namespace_token._contextualToken = EContextualToken::TYPE_REFERENCE;
    namespace_token._parentTokenIndices = namespace_token_indices;
    contextual_tokens.push_back(namespace_token);
    
    LOG_DEBUG("Matched namespace path consuming {} tokens", pos - start_pos);
    return PatternMatchResult::success(pos - start_pos, contextual_tokens);
}

PatternMatchResult HeaderContextualizer::try_match_inheritance_list(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    
    if (start_pos >= tokens.size() || tokens[start_pos]._token != EToken::COLON) {
        return PatternMatchResult::failure("Expected colon for inheritance list");
    }
    
    size_t pos = start_pos + 1;  // Skip colon
    std::vector<uint32_t> inheritance_token_indices;
    inheritance_token_indices.push_back(tokens[start_pos]._tokenIndex);
    
    // Match inheritance list until we hit a brace or other terminator
    while (pos < tokens.size()) {
        if (tokens[pos]._token == EToken::LEFT_BRACE || 
            tokens[pos]._token == EToken::SEMICOLON) {
            break; // End of inheritance list
        }
        
        inheritance_token_indices.push_back(tokens[pos]._tokenIndex);
        pos++;
    }
    
    if (inheritance_token_indices.size() <= 1) {
        return PatternMatchResult::failure("Empty inheritance list");
    }
    
    // Generate contextual tokens for the inheritance list
    std::vector<ContextualToken> contextual_tokens;
    
    ContextualToken inheritance_token;
    inheritance_token._contextualToken = EContextualToken::TYPE_REFERENCE;
    inheritance_token._parentTokenIndices = inheritance_token_indices;
    contextual_tokens.push_back(inheritance_token);
    
    LOG_DEBUG("Matched inheritance list consuming {} tokens", pos - start_pos);
    return PatternMatchResult::success(pos - start_pos, contextual_tokens);
}

PatternMatchResult HeaderContextualizer::try_match_function_signature(const std::vector<Token>& tokens, size_t start_pos) {
    auto logger = cprime::LoggerFactory::get_logger("header_contextualizer");
    
    // A function signature is: identifier + parameters + optional return type
    if (start_pos >= tokens.size() || tokens[start_pos]._token != EToken::IDENTIFIER) {
        return PatternMatchResult::failure("Expected function name identifier");
    }
    
    size_t pos = start_pos;
    std::vector<uint32_t> signature_token_indices;
    
    // Function name
    signature_token_indices.push_back(tokens[pos]._tokenIndex);
    pos++;
    
    // Parameters
    PatternMatchResult param_result = try_match_function_parameters(tokens, pos);
    if (!param_result.matched) {
        return PatternMatchResult::failure("Failed to match function parameters in signature");
    }
    
    // Add parameter tokens to signature
    for (const auto& ctx_token : param_result.contextual_tokens) {
        signature_token_indices.insert(signature_token_indices.end(),
                                      ctx_token._parentTokenIndices.begin(),
                                      ctx_token._parentTokenIndices.end());
    }
    pos += param_result.tokens_consumed;
    
    // Optional return type (-> type)
    if (pos < tokens.size() && tokens[pos]._token == EToken::ARROW) {
        signature_token_indices.push_back(tokens[pos]._tokenIndex);
        pos++;
        
        // Return type
        if (pos < tokens.size() && token_matches_element(tokens[pos], HeaderPatternElement::RETURN_TYPE)) {
            signature_token_indices.push_back(tokens[pos]._tokenIndex);
            pos++;
        }
    }
    
    // Generate contextual tokens for the complete function signature
    std::vector<ContextualToken> contextual_tokens;
    
    ContextualToken signature_token;
    signature_token._contextualToken = EContextualToken::FUNCTION_CALL;
    signature_token._parentTokenIndices = signature_token_indices;
    contextual_tokens.push_back(signature_token);
    
    LOG_DEBUG("Matched function signature consuming {} tokens", pos - start_pos);
    return PatternMatchResult::success(pos - start_pos, contextual_tokens);
}

} // namespace cprime::layer2_contextualization