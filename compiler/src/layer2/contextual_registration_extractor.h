#pragma once

#include "../commons/type_registry.h"
#include "../commons/function_registry.h"
#include "../commons/scope.h"
#include "../commons/contextualToken.h"
#include "../commons/rawToken.h"
#include "../commons/dirty/string_table.h"
#include <map>
#include <string>
#include <vector>

namespace cprime::layer2_contextualization {

/**
 * Extracts type and function information from contextual tokens
 * and registers them in the appropriate registries.
 * 
 * This class separates registration concerns from contextualization,
 * allowing clean pattern recognition followed by registration extraction.
 */
class ContextualRegistrationExtractor {
private:
    TypeRegistry& type_registry_;
    FunctionRegistry& function_registry_;
    StringTable& string_table_;
    
    // Current processing context
    uint32_t current_scope_index_;
    uint32_t current_instruction_index_;
    
public:
    ContextualRegistrationExtractor(TypeRegistry& type_registry,
                                   FunctionRegistry& function_registry,
                                   StringTable& string_table);
    
    // Main extraction method called from sublayer2d
    void extract_and_register_from_scopes(const std::vector<Scope>& scopes,
                                         const std::map<std::string, std::vector<RawToken>>& streams);
    
private:
    // Process individual scope
    void process_scope(const Scope& scope, uint32_t scope_index,
                      const std::map<std::string, std::vector<RawToken>>& streams);
    
    // Process individual instruction within scope
    void process_instruction(const Instruction& instruction, 
                           const std::vector<StringIndex>& namespace_path,
                           const std::map<std::string, std::vector<RawToken>>& streams);
    
    // Extract from different contextual token types
    void process_contextual_token(const ContextualToken& token,
                                 const std::vector<StringIndex>& namespace_path,
                                 const std::map<std::string, std::vector<RawToken>>& streams);
    
    // Specific contextual token processors
    void process_variable_declaration(const ContextualToken& token,
                                    const std::vector<StringIndex>& namespace_path,
                                    const std::map<std::string, std::vector<RawToken>>& streams);
    
    void process_function_call(const ContextualToken& token,
                             const std::vector<StringIndex>& namespace_path,
                             const std::map<std::string, std::vector<RawToken>>& streams);
    
    void process_type_reference(const ContextualToken& token,
                               const std::vector<StringIndex>& namespace_path,
                               const std::map<std::string, std::vector<RawToken>>& streams);
    
    void process_assignment(const ContextualToken& token,
                           const std::vector<StringIndex>& namespace_path,
                           const std::map<std::string, std::vector<RawToken>>& streams);
    
    void process_control_flow(const ContextualToken& token,
                             const std::vector<StringIndex>& namespace_path,
                             const std::map<std::string, std::vector<RawToken>>& streams);
    
    // Token extraction helpers
    StringIndex extract_type_name_from_declaration(const ContextualToken& token,
                                                  const std::map<std::string, std::vector<RawToken>>& streams);
    
    StringIndex extract_variable_name_from_declaration(const ContextualToken& token,
                                                      const std::map<std::string, std::vector<RawToken>>& streams);
    
    StringIndex extract_function_name_from_call(const ContextualToken& token,
                                               const std::map<std::string, std::vector<RawToken>>& streams);
    
    std::vector<StringIndex> extract_argument_types_from_call(const ContextualToken& token,
                                                             const std::map<std::string, std::vector<RawToken>>& streams);
    
    // Namespace resolution helpers
    std::vector<StringIndex> parse_namespace_context(const Scope& scope);
    
    // Token to string conversion helpers
    std::string get_token_content(uint32_t token_index, 
                                 const std::map<std::string, std::vector<RawToken>>& streams);
    
    StringIndex get_token_string_index(uint32_t token_index,
                                      const std::map<std::string, std::vector<RawToken>>& streams);
    
    // Find the stream and local index for a global token index
    std::pair<std::string, uint32_t> find_token_location(uint32_t global_token_index,
                                                         const std::map<std::string, std::vector<RawToken>>& streams);
    
    // Debug helpers
    void debug_log_extraction_context(const std::string& context,
                                     const ContextualToken& token,
                                     const std::vector<StringIndex>& namespace_path);
};

} // namespace cprime::layer2_contextualization