#include "../layer1/raw_token.h"
#include "test_framework.h"
#include <iostream>
#include <string>

using namespace cprime;
using namespace cprime::testing;

bool test_basic_tokenization() {
    TestLogger logger("Basic Raw Tokenization");
    
    try {
        logger << "=== Testing Basic Raw Tokenization ===\n";
        
        std::string test_code = R"(
            class Connection {
                handle: DbHandle,
                buffer: [u8; 4096],
            }
        )";
        
        logger << "Input code:\n" << test_code << "\n";
        
        RawTokenizer tokenizer(test_code);
        auto raw_tokens = tokenizer.tokenize();
        
        logger << "Successfully tokenized " << raw_tokens.size() << " raw tokens\n";
        
        if (raw_tokens.empty()) {
            TEST_FAILURE(logger, "No tokens generated from input code");
        }
        
        // Log first few tokens for debugging
        for (size_t i = 0; i < std::min(raw_tokens.size(), size_t(10)); ++i) {
            logger << "  [" << i << "] " << raw_tokens[i].to_string() << "\n";
        }
        
        if (raw_tokens.size() > 10) {
            logger << "  ... (" << (raw_tokens.size() - 10) << " more tokens)\n";
        }
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_complex_syntax_tokenization() {
    TestLogger logger("Complex Syntax Tokenization");
    
    try {
        logger << "=== Testing Complex Syntax Tokenization ===\n";
        
        std::string test_code = R"(
            class Connection {
                handle: DbHandle,
                buffer: [u8; 4096],
                
                runtime exposes UserOps { handle, buffer }
                exposes AdminOps { handle }
            }
            
            functional class FileOps {
                fn read(data: &mut FileData) -> Result<usize> {
                    defer FileOps::destruct(&mut data);
                    // implementation
                }
            }
            
            union runtime ConnectionSpace {
                UserConn(Connection<UserOps>),
                AdminConn(Connection<AdminOps>),
            }
        )";
        
        logger << "Testing complex CPrime syntax with:\n";
        logger << "- Data classes with access rights\n";
        logger << "- Functional classes with methods\n";
        logger << "- Runtime unions with type parameters\n";
        logger << "- Defer statements\n\n";
        
        RawTokenizer tokenizer(test_code);
        auto raw_tokens = tokenizer.tokenize();
        
        logger << "Successfully tokenized " << raw_tokens.size() << " raw tokens\n";
        
        if (raw_tokens.empty()) {
            TEST_FAILURE(logger, "No tokens generated from complex syntax");
        }
        
        // Count different token types for validation
        int keyword_count = 0;
        int identifier_count = 0;
        int punctuation_count = 0;
        
        for (const auto& token : raw_tokens) {
            switch (token.type) {
                case RawTokenType::KEYWORD:
                    keyword_count++;
                    break;
                case RawTokenType::IDENTIFIER:
                    identifier_count++;
                    break;
                case RawTokenType::PUNCTUATION:
                    punctuation_count++;
                    break;
                default:
                    break;
            }
        }
        
        logger << "Token type distribution:\n";
        logger << "  Keywords: " << keyword_count << "\n";
        logger << "  Identifiers: " << identifier_count << "\n"; 
        logger << "  Punctuation: " << punctuation_count << "\n";
        
        if (keyword_count == 0) {
            TEST_FAILURE(logger, "No keywords found in complex syntax");
        }
        
        if (identifier_count == 0) {
            TEST_FAILURE(logger, "No identifiers found in complex syntax");
        }
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_error_handling() {
    TestLogger logger("Error Handling");
    
    try {
        logger << "=== Testing Tokenization Error Handling ===\n";
        
        // Test empty input
        std::string empty_code = "";
        RawTokenizer empty_tokenizer(empty_code);
        auto empty_tokens = empty_tokenizer.tokenize();
        
        logger << "Empty input generated " << empty_tokens.size() << " tokens\n";
        
        // Test whitespace-only input
        std::string whitespace_code = "   \n\t  \n  ";
        RawTokenizer ws_tokenizer(whitespace_code);
        auto ws_tokens = ws_tokenizer.tokenize();
        
        logger << "Whitespace-only input generated " << ws_tokens.size() << " tokens\n";
        
        // Test single character input
        std::string single_char = "{";
        RawTokenizer single_tokenizer(single_char);
        auto single_tokens = single_tokenizer.tokenize();
        
        logger << "Single character '{' generated " << single_tokens.size() << " tokens\n";
        
        if (!single_tokens.empty()) {
            logger << "  Token: " << single_tokens[0].to_string() << "\n";
        }
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

int main() {
    TestSuite suite("Raw Tokenization Tests");
    
    std::cout << "CPrime Raw Tokenization Tests\n";
    std::cout << "=============================\n\n";
    
    suite.run_test(test_basic_tokenization);
    suite.run_test(test_complex_syntax_tokenization);
    suite.run_test(test_error_handling);
    
    suite.print_results();
    
    return suite.all_passed() ? 0 : 1;
}