#include "contextual_registration_extractor.h"
#include "../commons/logger.h"
#include "../commons/enum/contextualToken.h"
#include "../commons/rawToken.h"
#include <algorithm>

namespace cprime::layer2_contextualization {

ContextualRegistrationExtractor::ContextualRegistrationExtractor(TypeRegistry& type_registry,
                                                               FunctionRegistry& function_registry,
                                                               StringTable& string_table)
    : type_registry_(type_registry), function_registry_(function_registry), string_table_(string_table),
      current_scope_index_(0), current_instruction_index_(0) {
}

void ContextualRegistrationExtractor::extract_and_register_from_scopes(const std::vector<Scope>& scopes,
                                                                       const std::map<std::string, std::vector<RawToken>>& streams) {
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    LOG_INFO("Starting contextual registration extraction from {} scopes", scopes.size());
    
    for (uint32_t i = 0; i < scopes.size(); ++i) {
        process_scope(scopes[i], i, streams);
    }
    
    LOG_INFO("Contextual registration extraction complete");
    LOG_INFO("Total types registered: {}, instantiated: {}", 
             type_registry_.get_total_registered_types(),
             type_registry_.get_total_instantiated_types());
    LOG_INFO("Total functions registered: {}, called: {}", 
             function_registry_.get_total_registered_functions(),
             function_registry_.get_total_called_functions());
}

void ContextualRegistrationExtractor::process_scope(const Scope& scope, uint32_t scope_index,
                                                   const std::map<std::string, std::vector<RawToken>>& streams) {
    current_scope_index_ = scope_index;
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    // Parse namespace context from scope
    std::vector<StringIndex> namespace_path = parse_namespace_context(scope);
    
    LOG_DEBUG("Processing scope {} with namespace path (size: {})", scope_index, namespace_path.size());
    
    // Ensure all namespaces in the path are registered
    for (size_t i = 0; i < namespace_path.size(); ++i) {
        StringIndex parent = (i == namespace_path.size() - 1) ? StringIndex{UINT32_MAX} : namespace_path[i + 1];
        type_registry_.register_namespace(namespace_path[i], parent);
        function_registry_.register_namespace(namespace_path[i], parent);
    }
    
    // Process header instruction
    current_instruction_index_ = 0; // Header is instruction 0
    process_instruction(scope._header, namespace_path, streams);
    
    // Process body instructions
    for (uint32_t i = 0; i < scope._instructions.size(); ++i) {
        current_instruction_index_ = i + 1; // Instructions start at 1 (after header)
        
        if (std::holds_alternative<uint32_t>(scope._instructions[i])) {
            // Nested scope - don't process here, it will be processed when we reach that scope
            continue;
        }
        
        const Instruction& instruction = std::get<Instruction>(scope._instructions[i]);
        process_instruction(instruction, namespace_path, streams);
    }
    
    // Process footer instruction
    if (std::holds_alternative<Instruction>(scope._footer)) {
        current_instruction_index_ = scope._instructions.size() + 1; // Footer comes last
        const Instruction& footer_instruction = std::get<Instruction>(scope._footer);
        process_instruction(footer_instruction, namespace_path, streams);
    }
}

void ContextualRegistrationExtractor::process_instruction(const Instruction& instruction,
                                                         const std::vector<StringIndex>& namespace_path,
                                                         const std::map<std::string, std::vector<RawToken>>& streams) {
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    LOG_DEBUG("Processing instruction with {} contextual tokens", instruction._contextualTokens.size());
    
    // Process each contextual token in the instruction
    for (const auto& contextual_token : instruction._contextualTokens) {
        process_contextual_token(contextual_token, namespace_path, streams);
    }
}

void ContextualRegistrationExtractor::process_contextual_token(const ContextualToken& token,
                                                              const std::vector<StringIndex>& namespace_path,
                                                              const std::map<std::string, std::vector<RawToken>>& streams) {
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    debug_log_extraction_context("Processing contextual token", token, namespace_path);
    
    switch (token._contextualToken) {
        case EContextualToken::VARIABLE_DECLARATION:
            process_variable_declaration(token, namespace_path, streams);
            break;
            
        case EContextualToken::FUNCTION_CALL:
            process_function_call(token, namespace_path, streams);
            break;
            
        case EContextualToken::TYPE_REFERENCE:
            process_type_reference(token, namespace_path, streams);
            break;
            
        case EContextualToken::ASSIGNMENT:
            process_assignment(token, namespace_path, streams);
            break;
            
        case EContextualToken::CONTROL_FLOW:
            process_control_flow(token, namespace_path, streams);
            break;
            
        case EContextualToken::WHITESPACE:
        case EContextualToken::FORMATTING:
        case EContextualToken::OPERATOR:
        case EContextualToken::LITERAL_VALUE:
            // These don't contribute to type/function registration
            break;
            
        case EContextualToken::INVALID:
            LOG_DEBUG("Skipping invalid contextual token");
            break;
            
        default:
            LOG_DEBUG("Unhandled contextual token type: {}", static_cast<uint32_t>(token._contextualToken));
            break;
    }
}

void ContextualRegistrationExtractor::process_variable_declaration(const ContextualToken& token,
                                                                  const std::vector<StringIndex>& namespace_path,
                                                                  const std::map<std::string, std::vector<RawToken>>& streams) {
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    // Extract type name from the declaration
    StringIndex type_name = extract_type_name_from_declaration(token, streams);
    if (type_name.value == UINT32_MAX) {
        LOG_DEBUG("Could not extract type name from variable declaration");
        return;
    }
    
    // Mark the type as instantiated
    type_registry_.mark_type_instantiated(type_name, namespace_path);
    
    LOG_DEBUG("Registered type instantiation from variable declaration: {}", 
              string_table_.get_string(type_name));
}

void ContextualRegistrationExtractor::process_function_call(const ContextualToken& token,
                                                           const std::vector<StringIndex>& namespace_path,
                                                           const std::map<std::string, std::vector<RawToken>>& streams) {
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    // Extract function name from the call
    StringIndex function_name = extract_function_name_from_call(token, streams);
    if (function_name.value == UINT32_MAX) {
        LOG_DEBUG("Could not extract function name from function call");
        return;
    }
    
    // Extract argument types
    std::vector<StringIndex> arg_types = extract_argument_types_from_call(token, streams);
    
    // Create call site information
    CallSite call_site(current_scope_index_, current_instruction_index_, arg_types);
    
    // Mark the function as called
    function_registry_.mark_function_called(function_name, namespace_path, call_site);
    
    LOG_DEBUG("Registered function call: {} with {} arguments", 
              string_table_.get_string(function_name), arg_types.size());
}

void ContextualRegistrationExtractor::process_type_reference(const ContextualToken& token,
                                                           const std::vector<StringIndex>& namespace_path,
                                                           const std::map<std::string, std::vector<RawToken>>& streams) {
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    // For type references, we primarily care about marking types as instantiated
    // The actual type declaration should have been registered during header processing
    
    if (token._parentTokenIndices.empty()) {
        LOG_DEBUG("Type reference token has no parent tokens");
        return;
    }
    
    // Extract the first token as the type name (simple approach for now)
    StringIndex type_name = get_token_string_index(token._parentTokenIndices[0], streams);
    if (type_name.value == UINT32_MAX) {
        LOG_DEBUG("Could not extract type name from type reference");
        return;
    }
    
    // Mark the type as instantiated
    type_registry_.mark_type_instantiated(type_name, namespace_path);
    
    LOG_DEBUG("Registered type instantiation from type reference: {}", 
              string_table_.get_string(type_name));
}

void ContextualRegistrationExtractor::process_assignment(const ContextualToken& token,
                                                        const std::vector<StringIndex>& namespace_path,
                                                        const std::map<std::string, std::vector<RawToken>>& streams) {
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    // Assignments might involve type information on the right-hand side
    // For now, we'll implement basic handling
    
    LOG_DEBUG("Processing assignment - basic implementation");
    // TODO: Implement more sophisticated assignment analysis
}

void ContextualRegistrationExtractor::process_control_flow(const ContextualToken& token,
                                                          const std::vector<StringIndex>& namespace_path,
                                                          const std::map<std::string, std::vector<RawToken>>& streams) {
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    // Control flow tokens might contain return types or other type information
    // For now, we'll implement basic handling
    
    LOG_DEBUG("Processing control flow - basic implementation");
    // TODO: Implement control flow type extraction (e.g., return statement types)
}

StringIndex ContextualRegistrationExtractor::extract_type_name_from_declaration(const ContextualToken& token,
                                                                                const std::map<std::string, std::vector<RawToken>>& streams) {
    // For variable declarations, the type is typically the first token
    // Pattern: "int x;" -> "int" is at index 0
    
    if (token._parentTokenIndices.empty()) {
        return StringIndex{UINT32_MAX};
    }
    
    return get_token_string_index(token._parentTokenIndices[0], streams);
}

StringIndex ContextualRegistrationExtractor::extract_variable_name_from_declaration(const ContextualToken& token,
                                                                                   const std::map<std::string, std::vector<RawToken>>& streams) {
    // For variable declarations, the variable name is typically after the type
    // Pattern: "int x;" -> "x" is usually at index 2 (accounting for whitespace)
    
    if (token._parentTokenIndices.size() < 3) {
        return StringIndex{UINT32_MAX};
    }
    
    return get_token_string_index(token._parentTokenIndices[2], streams);
}

StringIndex ContextualRegistrationExtractor::extract_function_name_from_call(const ContextualToken& token,
                                                                            const std::map<std::string, std::vector<RawToken>>& streams) {
    // For function calls, the function name is typically the first token
    // Pattern: "print("hello")" -> "print" is at index 0
    
    if (token._parentTokenIndices.empty()) {
        return StringIndex{UINT32_MAX};
    }
    
    return get_token_string_index(token._parentTokenIndices[0], streams);
}

std::vector<StringIndex> ContextualRegistrationExtractor::extract_argument_types_from_call(const ContextualToken& token,
                                                                                          const std::map<std::string, std::vector<RawToken>>& streams) {
    // For now, implement basic argument type extraction
    // This is complex and would need full expression analysis
    
    std::vector<StringIndex> arg_types;
    
    // TODO: Implement sophisticated argument type analysis
    // For now, return empty vector (will be improved in future iterations)
    
    return arg_types;
}

std::vector<StringIndex> ContextualRegistrationExtractor::parse_namespace_context(const Scope& scope) {
    std::vector<StringIndex> namespace_path;
    
    // Parse scope.namespace_context which is a vector of strings
    for (const auto& namespace_part : scope.namespace_context) {
        StringIndex ns_index = string_table_.intern(namespace_part);
        namespace_path.push_back(ns_index);
    }
    
    return namespace_path;
}

std::string ContextualRegistrationExtractor::get_token_content(uint32_t token_index,
                                                              const std::map<std::string, std::vector<RawToken>>& streams) {
    auto [stream_name, local_index] = find_token_location(token_index, streams);
    if (stream_name.empty()) {
        return "";
    }
    
    auto stream_it = streams.find(stream_name);
    if (stream_it == streams.end() || local_index >= stream_it->second.size()) {
        return "";
    }
    
    // Extract string from the RawToken's literal value
    const RawToken& raw_token = stream_it->second[local_index];
    if (std::holds_alternative<StringIndex>(raw_token._literal_value)) {
        StringIndex str_idx = std::get<StringIndex>(raw_token._literal_value);
        return string_table_.get_string(str_idx);
    }
    
    return ""; // For now, return empty for non-string tokens
}

StringIndex ContextualRegistrationExtractor::get_token_string_index(uint32_t token_index,
                                                                   const std::map<std::string, std::vector<RawToken>>& streams) {
    std::string token_content = get_token_content(token_index, streams);
    if (token_content.empty()) {
        return StringIndex{UINT32_MAX};
    }
    
    return string_table_.intern(token_content);
}

std::pair<std::string, uint32_t> ContextualRegistrationExtractor::find_token_location(uint32_t global_token_index,
                                                                                      const std::map<std::string, std::vector<RawToken>>& streams) {
    // For now, implement a simple approach: assume global_token_index maps directly to local indices
    // This is a simplified implementation that needs to be improved later
    // The proper implementation would require understanding how global token indices map to streams
    
    uint32_t current_offset = 0;
    for (const auto& [stream_name, tokens] : streams) {
        if (global_token_index >= current_offset && global_token_index < current_offset + tokens.size()) {
            return {stream_name, global_token_index - current_offset};
        }
        current_offset += tokens.size();
    }
    
    return {"", UINT32_MAX}; // Not found
}

void ContextualRegistrationExtractor::debug_log_extraction_context(const std::string& context,
                                                                   const ContextualToken& token,
                                                                   const std::vector<StringIndex>& namespace_path) {
    auto logger = cprime::LoggerFactory::get_logger("contextual_registration_extractor");
    
    std::string namespace_str;
    for (size_t i = 0; i < namespace_path.size(); ++i) {
        if (i > 0) namespace_str += "::";
        namespace_str += string_table_.get_string(namespace_path[i]);
    }
    
    LOG_DEBUG("{}: type={}, tokens={}, namespace={}", 
              context,
              static_cast<uint32_t>(token._contextualToken),
              token._parentTokenIndices.size(),
              namespace_str.empty() ? "__global__" : namespace_str);
}

} // namespace cprime::layer2_contextualization