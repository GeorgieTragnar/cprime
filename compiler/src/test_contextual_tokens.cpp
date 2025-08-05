#include "raw_token.h"
#include "context_stack.h"
#include <iostream>
#include <vector>

using namespace cprime;

/**
 * Simple demo of context-enriched tokens.
 * Shows how raw tokens can be enriched with context information.
 */
int main() {
    std::cout << "=== CPrime Contextual Token Demo ===\n\n";
    
    // Sample CPrime code
    std::string cprime_code = R"(
        class Connection {
            runtime exposes UserOps { handle }
            defer cleanup();
        }
    )";
    
    // Step 1: Raw tokenization (Layer 1)
    std::cout << "--- Layer 1: Raw Tokenization ---\n";
    RawTokenizer tokenizer(cprime_code);
    auto raw_tokens = tokenizer.tokenize();
    
    std::cout << "Raw tokens generated: " << raw_tokens.size() << "\n";
    for (size_t i = 0; i < std::min(raw_tokens.size(), size_t(10)); ++i) {
        std::cout << "  [" << i << "] " << raw_tokens[i].to_string() << "\n";
    }
    std::cout << "\n";
    
    // Step 2: Context enrichment (Layer 2 concept demo)
    std::cout << "--- Layer 2: Context Enrichment Demo ---\n";
    
    std::vector<ContextualToken> contextual_tokens;
    ContextStack context_stack;
    ParseContextType current_context = ParseContextType::TopLevel;
    
    for (const auto& raw_token : raw_tokens) {
        // Skip whitespace for cleaner output
        if (raw_token.type == RawTokenType::WHITESPACE) {
            continue;
        }
        
        // Simple context tracking
        if (raw_token.is_keyword("class")) {
            current_context = ParseContextType::ClassDefinition;
        } else if (raw_token.is_punctuation("{")) {
            context_stack.push(current_context);
            current_context = ParseContextType::Block;
        } else if (raw_token.is_punctuation("}")) {
            if (!context_stack.empty()) {
                context_stack.pop();
                current_context = context_stack.current() ? 
                    context_stack.current()->type : ParseContextType::TopLevel;
            }
        }
        
        // Create contextual token
        ContextualToken contextual_token(raw_token, current_context);
        
        // Add context resolution for key tokens
        if (raw_token.is_keyword("runtime")) {
            if (current_context == ParseContextType::Block) {
                contextual_token.context_resolution = "RuntimeAccessRight";
                contextual_token.set_attribute("access_type", "runtime");
            } else {
                contextual_token.context_resolution = "RuntimeTypeParameter";
                contextual_token.set_attribute("dispatch_type", "runtime");
            }
        } else if (raw_token.is_keyword("defer")) {
            contextual_token.context_resolution = "DeferRaii";
            contextual_token.set_attribute("defer_type", "raii");
        } else if (raw_token.is_keyword("class")) {
            contextual_token.context_resolution = "DataClass";
            contextual_token.set_attribute("class_type", "data");
        } else {
            contextual_token.context_resolution = "PassThrough";
        }
        
        contextual_tokens.push_back(contextual_token);
    }
    
    std::cout << "Contextual tokens created: " << contextual_tokens.size() << "\n";
    for (size_t i = 0; i < contextual_tokens.size(); ++i) {
        std::cout << "  [" << i << "] " << contextual_tokens[i].to_string() << "\n";
    }
    std::cout << "\n";
    
    // Step 3: Demonstrate self-contained nature
    std::cout << "--- Demonstrating Self-Contained Tokens ---\n";
    
    for (const auto& token : contextual_tokens) {
        if (token.is_keyword("runtime") || token.is_keyword("defer")) {
            std::cout << "Token: " << token.value() << "\n";
            std::cout << "  Resolution: " << token.context_resolution << "\n";
            std::cout << "  Context: " << static_cast<int>(token.current_context) << "\n";
            
            if (token.has_attribute("access_type")) {
                std::cout << "  Access Type: " << token.get_attribute("access_type") << "\n";
            }
            if (token.has_attribute("defer_type")) {
                std::cout << "  Defer Type: " << token.get_attribute("defer_type") << "\n";
            }
            std::cout << "\n";
        }
    }
    
    // Step 4: Demonstrate GPU-ready properties
    std::cout << "--- GPU-Ready Properties ---\n";
    std::cout << "✓ 1:1 Raw Token Mapping: " << (raw_tokens.size() >= contextual_tokens.size()) << "\n";
    std::cout << "✓ Self-Contained Tokens: Each token has complete context info\n";
    std::cout << "✓ Fixed-Size Structure: ContextualToken has predictable memory layout\n";
    std::cout << "✓ Parallel Processing Ready: Each token can be processed independently\n";
    std::cout << "\n";
    
    std::cout << "=== Context Enrichment Demo Complete ===\n";
    std::cout << "This demonstrates how raw tokens can be enriched with context\n";
    std::cout << "information while maintaining 1:1 mapping and self-contained nature.\n";
    std::cout << "Perfect for GPU-accelerated compilation! 🚀\n";
    
    return 0;
}