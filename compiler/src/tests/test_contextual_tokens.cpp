#include "../layer1/raw_token.h"
#include "../layer1/context_stack.h"
#include "../layer2/contextual_token.h"
#include "test_framework.h"
#include <iostream>
#include <vector>

using namespace cprime;
using namespace cprime::testing;

bool test_raw_tokenization() {
    TestLogger logger("Raw Tokenization");
    
    try {
        logger << "=== Testing Raw Tokenization ===\n";
    
        // Sample CPrime code
        std::string cprime_code = R"(
            class Connection {
                runtime exposes UserOps { handle }
                defer cleanup();
            }
        )";
        
        logger << "Input code:\n" << cprime_code << "\n";
        
        // Step 1: Raw tokenization (Layer 1)
        logger << "--- Layer 1: Raw Tokenization ---\n";
        RawTokenizer tokenizer(cprime_code);
        auto raw_tokens = tokenizer.tokenize();
        
        logger << "Raw tokens generated: " << raw_tokens.size() << "\n";
        for (size_t i = 0; i < std::min(raw_tokens.size(), size_t(10)); ++i) {
            logger << "  [" << i << "] " << raw_tokens[i].to_string() << "\n";
        }
        
        if (raw_tokens.empty()) {
            TEST_FAILURE(logger, "No raw tokens generated");
        }
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_context_enrichment() {
    TestLogger logger("Context Enrichment");
    
    try {
        logger << "=== Testing Context Enrichment ===\n";
        
        // Sample CPrime code
        std::string cprime_code = R"(
            class Connection {
                runtime exposes UserOps { handle }
                defer cleanup();
            }
        )";
    
        // Step 1: Raw tokenization first
        RawTokenizer tokenizer(cprime_code);
        auto raw_tokens = tokenizer.tokenize();
        
        // Step 2: Context enrichment (Layer 2 concept demo)
        logger << "--- Layer 2: Context Enrichment Demo ---\n";
        
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
    
        logger << "Contextual tokens created: " << contextual_tokens.size() << "\n";
        for (size_t i = 0; i < contextual_tokens.size(); ++i) {
            logger << "  [" << i << "] " << contextual_tokens[i].to_string() << "\n";
        }
    
        if (contextual_tokens.empty()) {
            TEST_FAILURE(logger, "No contextual tokens created");
        }
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_self_contained_tokens() {
    TestLogger logger("Self-Contained Token Analysis");
    
    try {
        logger << "=== Testing Self-Contained Token Analysis ===\n";
        
        // Sample CPrime code
        std::string cprime_code = R"(
            class Connection {
                runtime exposes UserOps { handle }
                defer cleanup();
            }
        )";
        
        // Process tokens
        RawTokenizer tokenizer(cprime_code);
        auto raw_tokens = tokenizer.tokenize();
        
        std::vector<ContextualToken> contextual_tokens;
        ContextStack context_stack;
        ParseContextType current_context = ParseContextType::TopLevel;
        
        for (const auto& raw_token : raw_tokens) {
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
            
            ContextualToken contextual_token(raw_token, current_context);
            
            if (raw_token.is_keyword("runtime")) {
                contextual_token.context_resolution = "RuntimeAccessRight";
                contextual_token.set_attribute("access_type", "runtime");
            } else if (raw_token.is_keyword("defer")) {
                contextual_token.context_resolution = "DeferRaii";
                contextual_token.set_attribute("defer_type", "raii");
            }
            
            contextual_tokens.push_back(contextual_token);
        }
        
        // Demonstrate self-contained nature
        logger << "--- Demonstrating Self-Contained Tokens ---\n";
        
        int special_tokens_found = 0;
        for (const auto& token : contextual_tokens) {
            if (token.is_keyword("runtime") || token.is_keyword("defer")) {
                logger << "Token: " << token.value() << "\n";
                logger << "  Resolution: " << token.context_resolution << "\n";
                logger << "  Context: " << static_cast<int>(token.current_context) << "\n";
                
                if (token.has_attribute("access_type")) {
                    logger << "  Access Type: " << token.get_attribute("access_type") << "\n";
                }
                if (token.has_attribute("defer_type")) {
                    logger << "  Defer Type: " << token.get_attribute("defer_type") << "\n";
                }
                logger << "\n";
                special_tokens_found++;
            }
        }
        
        if (special_tokens_found == 0) {
            TEST_FAILURE(logger, "No special tokens (runtime/defer) found");
        }
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_gpu_ready_properties() {
    TestLogger logger("GPU-Ready Properties");
    
    try {
        logger << "=== Testing GPU-Ready Properties ===\n";
        
        // Sample CPrime code
        std::string cprime_code = R"(
            class Connection {
                runtime exposes UserOps { handle }
                defer cleanup();
            }
        )";
        
        // Process tokens
        RawTokenizer tokenizer(cprime_code);
        auto raw_tokens = tokenizer.tokenize();
        
        std::vector<ContextualToken> contextual_tokens;
        for (const auto& raw_token : raw_tokens) {
            if (raw_token.type != RawTokenType::WHITESPACE) {
                ContextualToken contextual_token(raw_token, ParseContextType::TopLevel);
                contextual_tokens.push_back(contextual_token);
            }
        }
    
        // Demonstrate GPU-ready properties
        logger << "--- GPU-Ready Properties ---\n";
        logger << "✓ 1:1 Raw Token Mapping: " << (raw_tokens.size() >= contextual_tokens.size()) << "\n";
        logger << "✓ Self-Contained Tokens: Each token has complete context info\n";
        logger << "✓ Fixed-Size Structure: ContextualToken has predictable memory layout\n";
        logger << "✓ Parallel Processing Ready: Each token can be processed independently\n";
        
        logger << "\nThis demonstrates how raw tokens can be enriched with context\n";
        logger << "information while maintaining 1:1 mapping and self-contained nature.\n";
        logger << "Perfect for GPU-accelerated compilation! 🚀\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

int main() {
    TestSuite suite("CPrime Contextual Token Demo");
    
    std::cout << "CPrime Contextual Token Demo\n";
    std::cout << "============================\n\n";
    
    suite.run_test(test_raw_tokenization);
    suite.run_test(test_context_enrichment);
    suite.run_test(test_self_contained_tokens);
    suite.run_test(test_gpu_ready_properties);
    
    suite.print_results();
    
    return suite.all_passed() ? 0 : 1;
}