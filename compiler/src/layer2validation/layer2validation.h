#pragma once

#include <string>
#include <sstream>
#include <vector>
#include "../commons/scope.h"
#include "../commons/instruction.h"
#include "../commons/enum/token.h"
#include "../layer1validation/layer1validation.h"

// Layer 2 validation interface for structure building
// This header provides serialization functions for Scope and Instruction structures

namespace cprime {
namespace layer2validation {


/**
 * Serialize Token to human-readable format
 */
inline std::string serialize_token(const Token& token) {
    std::ostringstream oss;
    oss << "{stream:" << token._stringstreamId 
        << ",idx:" << token._tokenIndex 
        << ",type:" << layer1_sublayers::validation::etoken_to_string(token._token) << "}";
    return oss.str();
}

/**
 * Serialize Instruction to human-readable format
 */
inline std::string serialize_instruction(const Instruction& instruction, int indent = 0) {
    std::ostringstream oss;
    std::string indent_str(indent * 2, ' ');
    
    oss << indent_str << "Instruction {\n";
    oss << indent_str << "  tokens: [";
    
    for (size_t i = 0; i < instruction._tokens.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << serialize_token(instruction._tokens[i]);
    }
    
    oss << "]\n";
    oss << indent_str << "  contextual_tokens: " << instruction._contextualTokens.size() << "\n";
    oss << indent_str << "  contexts: " << instruction._contexts.size() << "\n";
    oss << indent_str << "}";
    
    return oss.str();
}

/**
 * Serialize Scope to human-readable format
 */
inline std::string serialize_scope(const Scope& scope, uint32_t scope_index, int indent = 0) {
    std::ostringstream oss;
    std::string indent_str(indent * 2, ' ');
    
    oss << indent_str << "Scope[" << scope_index << "] {\n";
    oss << indent_str << "  parent: " << scope._parentScopeIndex << "\n";
    
    oss << indent_str << "  header: ";
    if (scope._header._tokens.empty()) {
        oss << "EMPTY\n";
    } else {
        oss << "\n" << serialize_instruction(scope._header, indent + 2) << "\n";
    }
    
    oss << indent_str << "  footer: ";
    if (scope._footer._tokens.empty()) {
        oss << "EMPTY\n";
    } else {
        oss << "\n" << serialize_instruction(scope._footer, indent + 2) << "\n";
    }
    
    oss << indent_str << "  instructions: ";
    if (scope._instructions.empty()) {
        oss << "EMPTY\n";
    } else {
        oss << "\n";
        for (size_t i = 0; i < scope._instructions.size(); ++i) {
            const auto& instruction_variant = scope._instructions[i];
            oss << indent_str << "  [" << i << "] ";
            
            if (std::holds_alternative<Instruction>(instruction_variant)) {
                const auto& instr = std::get<Instruction>(instruction_variant);
                if (instr._tokens.empty()) {
                    oss << "EMPTY_INSTRUCTION\n";
                } else {
                    oss << "\n" << serialize_instruction(instr, indent + 2) << "\n";
                }
            } else {
                uint32_t nested_scope = std::get<uint32_t>(instruction_variant);
                oss << "NESTED_SCOPE[" << nested_scope << "]\n";
            }
        }
    }
    
    oss << indent_str << "  contexts: " << scope._contexts.size() << "\n";
    oss << indent_str << "}";
    
    return oss.str();
}

/**
 * Serialize entire scope vector to human-readable format
 */
inline std::string serialize_scope_vector(const std::vector<Scope>& scopes) {
    std::ostringstream oss;
    
    oss << "=== SCOPE STRUCTURE DUMP ===\n";
    oss << "Total scopes: " << scopes.size() << "\n\n";
    
    for (size_t i = 0; i < scopes.size(); ++i) {
        oss << serialize_scope(scopes[i], static_cast<uint32_t>(i), 0);
        if (i < scopes.size() - 1) {
            oss << "\n\n";
        }
    }
    
    oss << "\n=== END SCOPE STRUCTURE ===";
    return oss.str();
}

/**
 * Serialize scope statistics for quick overview
 */
inline std::string serialize_scope_stats(const std::vector<Scope>& scopes) {
    std::ostringstream oss;
    
    oss << "Scope Statistics:\n";
    oss << "  Total: " << scopes.size();
    
    return oss.str();
}

} // namespace layer2validation
} // namespace cprime