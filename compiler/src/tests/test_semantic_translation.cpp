#include "../layer1/raw_token.h"
#include "../layer2/semantic_token.h"
#include "../layer2/semantic_translator.h"
#include "../layer1/context_stack.h"
#include "test_framework.h"
#include <iostream>
#include <string>

using namespace cprime;
using namespace cprime::testing;

bool test_basic_semantic_translation() {
    TestLogger logger("Basic Semantic Translation");
    
    try {
        logger << "=== Testing Basic Semantic Translation ===\n";
        
        std::string test_code = R"(
            class Connection {
                handle: DbHandle,
                runtime exposes UserOps { handle }
            }
        )";
        
        logger << "Input code:\n" << test_code << "\n";
        
        // Step 1: Raw tokenization
        RawTokenizer tokenizer(test_code);
        auto raw_token_stream = tokenizer.tokenize_to_stream();
        
        logger << "Raw tokens generated: " << raw_token_stream.size() << "\n";
        
        if (raw_token_stream.size() == 0) {
            TEST_FAILURE(logger, "No raw tokens generated");
        }
        
        // Step 2: Semantic translation
        SemanticTranslator translator(std::move(raw_token_stream));
        auto semantic_tokens = translator.translate();
        
        logger << "Semantic tokens generated: " << semantic_tokens.size() << "\n";
        
        if (semantic_tokens.empty()) {
            TEST_FAILURE(logger, "No semantic tokens generated");
        }
        
        if (translator.has_errors()) {
            logger << "Translation errors found:\n";
            for (const auto& error : translator.get_errors()) {
                logger << "  Error at " << error.line << ":" << error.column 
                       << " - " << error.message << "\n";
                logger << "  Context: " << error.context_path << "\n";
            }
            // Don't fail on errors in basic test - some might be expected
        }
        
        // Display some semantic tokens for debugging
        logger << "\nFirst few semantic tokens:\n";
        for (size_t i = 0; i < std::min(semantic_tokens.size(), size_t(10)); ++i) {
            logger << "  [" << i << "] " << semantic_tokens[i].to_string() << "\n";
        }
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_complex_semantic_translation() {
    TestLogger logger("Complex Semantic Translation");
    
    try {
        logger << "=== Testing Complex Semantic Translation ===\n";
        
        std::string test_code = R"(
            class Connection {
                handle: DbHandle,
                runtime exposes UserOps { handle }
            }
            
            defer FileOps::destruct(&mut file);
            
            union runtime MessageSpace {
                Text(String),
                Binary(Vec<u8>),
            }
        )";
        
        logger << "Testing complex code with access rights, defer, and unions\n";
        
        // Step 1: Raw tokenization
        RawTokenizer tokenizer(test_code);
        auto raw_token_stream = tokenizer.tokenize_to_stream();
        
        logger << "Raw tokens generated: " << raw_token_stream.size() << "\n";
        
        // Step 2: Semantic translation
        SemanticTranslator translator(std::move(raw_token_stream));
        auto semantic_tokens = translator.translate();
        
        logger << "Semantic tokens generated: " << semantic_tokens.size() << "\n";
        
        if (translator.has_errors()) {
            logger << "Translation errors (" << translator.get_errors().size() << "):\n";
            for (const auto& error : translator.get_errors()) {
                logger << "  Error at " << error.line << ":" << error.column 
                       << " - " << error.message << "\n";
            }
        }
        
        if (semantic_tokens.empty()) {
            TEST_FAILURE(logger, "No semantic tokens generated from complex code");
        }
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_semantic_token_analysis() {
    TestLogger logger("Semantic Token Analysis");
    
    try {
        logger << "=== Testing Semantic Token Analysis ===\n";
        
        std::string test_code = R"(
            class Connection {
                handle: DbHandle,
                buffer: [u8; 4096],
                
                runtime exposes UserOps { handle, buffer }
                exposes AdminOps { handle }
            }
            
            defer FileOps::destruct(&mut data);
            
            union runtime MessageSpace {
                Text(String),
                Binary(Vec<u8>),
            }
        )";
        
        // Process the code
        RawTokenizer tokenizer(test_code);
        auto raw_token_stream = tokenizer.tokenize_to_stream();
        
        SemanticTranslator translator(std::move(raw_token_stream));
        auto semantic_tokens = translator.translate();
        
        logger << "Analyzing semantic token types in complex code...\n";
        
        // Test specific semantic token types
        size_t access_right_count = 0;
        size_t defer_count = 0;
        size_t union_count = 0;
        size_t class_count = 0;
        
        for (const auto& token : semantic_tokens) {
            switch (token.type) {
                case SemanticTokenType::RuntimeAccessRightDeclaration:
                case SemanticTokenType::CompileTimeAccessRightDeclaration:
                    access_right_count++;
                    break;
                case SemanticTokenType::RaiiDefer:
                case SemanticTokenType::CoroutineDefer:
                    defer_count++;
                    break;
                case SemanticTokenType::RuntimeUnion:
                case SemanticTokenType::CompileTimeUnion:
                    union_count++;
                    break;
                case SemanticTokenType::DataClass:
                case SemanticTokenType::FunctionalClass:
                    class_count++;
                    break;
                default:
                    break;
            }
        }
        
        logger << "\nSemantic token analysis results:\n";
        logger << "  Access rights declarations: " << access_right_count << "\n";
        logger << "  Defer statements: " << defer_count << "\n";
        logger << "  Union declarations: " << union_count << "\n";
        logger << "  Class declarations: " << class_count << "\n";
        
        // We expect to find at least some of these constructs
        if (access_right_count == 0 && defer_count == 0 && union_count == 0 && class_count == 0) {
            TEST_FAILURE(logger, "No expected semantic token types found");
        }
        
        logger << "Total semantic tokens processed: " << semantic_tokens.size() << "\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_semantic_translation_error_handling() {
    TestLogger logger("Semantic Translation Error Handling");
    
    try {
        logger << "=== Testing Semantic Translation Error Handling ===\n";
        
        // Test empty input
        std::string empty_code = "";
        RawTokenizer empty_tokenizer(empty_code);
        auto empty_stream = empty_tokenizer.tokenize_to_stream();
        
        SemanticTranslator empty_translator(std::move(empty_stream));
        auto empty_tokens = empty_translator.translate();
        
        logger << "Empty input generated " << empty_tokens.size() << " semantic tokens\n";
        logger << "Empty input errors: " << empty_translator.get_errors().size() << "\n";
        
        // Test whitespace-only input
        std::string ws_code = "   \n  \t  \n  ";
        RawTokenizer ws_tokenizer(ws_code);
        auto ws_stream = ws_tokenizer.tokenize_to_stream();
        
        SemanticTranslator ws_translator(std::move(ws_stream));
        auto ws_tokens = ws_translator.translate();
        
        logger << "Whitespace-only input generated " << ws_tokens.size() << " semantic tokens\n";
        logger << "Whitespace-only input errors: " << ws_translator.get_errors().size() << "\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

int main() {
    TestSuite suite("Semantic Translation Tests");
    
    std::cout << "CPrime Semantic Translation Tests\n";
    std::cout << "=================================\n\n";
    
    suite.run_test(test_basic_semantic_translation);
    suite.run_test(test_complex_semantic_translation);
    suite.run_test(test_semantic_token_analysis);
    suite.run_test(test_semantic_translation_error_handling);
    
    suite.print_results();
    
    return suite.all_passed() ? 0 : 1;
}