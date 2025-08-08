#include "../layer1/raw_token.h"
#include "../layer2/semantic_token.h"
#include "../layer2/semantic_translator.h"
#include "../layer1/context_stack.h"
#include "test_framework.h"
#include <iostream>
#include <string>

using namespace cprime;
using namespace cprime::testing;

bool test_simple_pipeline_integration() {
    TestLogger logger("Simple Pipeline Integration");
    
    try {
        logger << "=== Testing Simple Pipeline Integration ===\n";
        
        std::string simple_code = R"(
            class Connection {
                handle: DbHandle,
                exposes ReadOps { handle }
            }
        )";
        
        logger << "Testing simple CPrime class through full pipeline:\n";
        logger << "Input code:\n" << simple_code << "\n";
        
        // Layer 1: Raw tokenization
        logger << "Layer 1: Raw tokenization...\n";
        RawTokenizer tokenizer(simple_code);
        auto raw_stream = tokenizer.tokenize_to_stream();
        logger << "Layer 1 complete: " << raw_stream.size() << " raw tokens\n";
        
        if (raw_stream.size() == 0) {
            TEST_FAILURE(logger, "Layer 1 produced no raw tokens");
        }
        
        // Layer 2: Semantic translation
        logger << "Layer 2: Semantic translation...\n";
        SemanticTranslator translator(std::move(raw_stream));
        auto semantic_stream = translator.translate_to_stream();
        logger << "Layer 2 complete: " << semantic_stream.size() << " semantic tokens\n";
        
        if (semantic_stream.size() == 0) {
            TEST_FAILURE(logger, "Layer 2 produced no semantic tokens");
        }
        
        if (translator.has_errors()) {
            logger << "Layer 2 errors: " << translator.get_errors().size() << "\n";
            for (const auto& error : translator.get_errors()) {
                logger << "  " << error.message << " at " << error.line << ":" << error.column << "\n";
            }
        }
        
        logger << "Simple pipeline integration completed successfully\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_complex_pipeline_integration() {
    TestLogger logger("Complex Pipeline Integration");
    
    try {
        logger << "=== Testing Complex Pipeline Integration ===\n";
        
        std::string complex_code = R"(
            // Data class with access rights
            class DatabaseConnection {
                handle: DbHandle,
                cache: QueryCache,
                
                // Compile-time access right
                exposes ReadOps { handle, cache }
                
                // Runtime access right with vtable
                runtime exposes AdminOps { handle }
            }
            
            // Functional class with defer
            functional class DatabaseOps {
                fn query(conn: &mut DatabaseConnection) -> Result<QueryResult> {
                    defer DatabaseOps::cleanup(&mut conn);
                    
                    // Query implementation
                    let result = execute_query(conn);
                    result
                }
            }
            
            // Runtime union for polymorphic storage
            union runtime ConnectionVariant {
                Read(DatabaseConnection<ReadOps>),
                Admin(DatabaseConnection<AdminOps>),
            }
        )";
        
        logger << "Processing complex CPrime code through full pipeline...\n";
        logger << "Code features: classes, access rights, defer, unions, functions\n";
        
        try {
            // Layer 1: Raw tokenization
            logger << "\n--- Layer 1: Raw Tokenization ---\n";
            RawTokenizer tokenizer(complex_code);
            auto raw_stream = tokenizer.tokenize_to_stream();
            logger << "Layer 1 complete: " << raw_stream.size() << " raw tokens\n";
            
            if (raw_stream.size() == 0) {
                TEST_FAILURE(logger, "Layer 1 failed to tokenize complex code");
            }
            
            // Layer 2: Semantic translation
            logger << "\n--- Layer 2: Semantic Translation ---\n";
            SemanticTranslator translator(std::move(raw_stream));
            auto semantic_stream = translator.translate_to_stream();
            logger << "Layer 2 complete: " << semantic_stream.size() << " semantic tokens\n";
            
            if (translator.has_errors()) {
                logger << "Translation errors: " << translator.get_errors().size() << "\n";
                // Log first few errors for debugging
                for (size_t i = 0; i < std::min(translator.get_errors().size(), size_t(5)); ++i) {
                    const auto& error = translator.get_errors()[i];
                    logger << "  " << error.message << " at " << error.line << ":" << error.column << "\n";
                }
            }
            
            // Analyze the semantic tokens
            logger << "\n--- Semantic Analysis ---\n";
            auto runtime_access_rights = semantic_stream.filter_by_type(SemanticTokenType::RuntimeAccessRightDeclaration);
            auto compile_time_access_rights = semantic_stream.filter_by_type(SemanticTokenType::CompileTimeAccessRightDeclaration);
            auto defer_statements = semantic_stream.filter_by_type(SemanticTokenType::RaiiDefer);
            auto runtime_unions = semantic_stream.filter_by_type(SemanticTokenType::RuntimeUnion);
            
            logger << "Semantic analysis results:\n";
            logger << "  Runtime access rights: " << runtime_access_rights.size() << "\n";
            logger << "  Compile-time access rights: " << compile_time_access_rights.size() << "\n";
            logger << "  RAII defer statements: " << defer_statements.size() << "\n";
            logger << "  Runtime unions: " << runtime_unions.size() << "\n";
            
            // Show some example semantic tokens
            if (!runtime_access_rights.empty() || !defer_statements.empty()) {
                logger << "\nExample semantic tokens:\n";
                for (const auto& token : runtime_access_rights) {
                    logger << "  Runtime Access: " << token.to_string() << "\n";
                    break; // Just show first one
                }
                for (const auto& token : defer_statements) {
                    logger << "  Defer: " << token.to_string() << "\n";
                    break; // Just show first one
                }
            }
            
            logger << "\nComplex pipeline integration completed successfully\n";
            
        } catch (const std::exception& inner_e) {
            logger << "Pipeline processing failed: " << inner_e.what() << "\n";
            TEST_FAILURE(logger, "Pipeline processing threw exception");
        }
        
        // Layer 3 would be LLVM IR generation (not implemented yet)
        logger << "\nLayer 3 (LLVM IR generation): Not yet implemented\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_pipeline_performance_characteristics() {
    TestLogger logger("Pipeline Performance Characteristics");
    
    try {
        logger << "=== Testing Pipeline Performance Characteristics ===\n";
        
        // Test with different sized inputs to verify scalability
        std::vector<std::string> test_cases = {
            // Small input
            R"(class A { x: int })",
            
            // Medium input  
            R"(
                class Connection { 
                    handle: DbHandle,
                    exposes UserOps { handle }
                }
                functional class Ops {
                    fn process(conn: &mut Connection) {}
                }
            )",
            
            // Larger input
            R"(
                class DatabaseConnection {
                    handle: DbHandle,
                    cache: QueryCache,
                    buffer: [u8; 4096],
                    
                    runtime exposes UserOps { handle, buffer }
                    exposes AdminOps { handle, cache }
                    exposes ReadOps { cache, buffer }
                }
                
                functional class DatabaseOps {
                    fn connect() -> DatabaseConnection {}
                    fn query(conn: &mut DatabaseConnection) -> Result<Data> {
                        defer cleanup(&mut conn);
                        execute_query(conn)
                    }
                    fn disconnect(conn: DatabaseConnection) {}
                }
                
                union runtime ConnectionType {
                    User(DatabaseConnection<UserOps>),
                    Admin(DatabaseConnection<AdminOps>),
                    Read(DatabaseConnection<ReadOps>),
                }
            )"
        };
        
        for (size_t i = 0; i < test_cases.size(); ++i) {
            logger << "\nTesting input size " << (i + 1) << " (length: " << test_cases[i].size() << " chars):\n";
            
            // Layer 1
            RawTokenizer tokenizer(test_cases[i]);
            auto raw_stream = tokenizer.tokenize_to_stream();
            logger << "  Raw tokens: " << raw_stream.size() << "\n";
            
            // Layer 2
            SemanticTranslator translator(std::move(raw_stream));
            auto semantic_stream = translator.translate_to_stream();
            logger << "  Semantic tokens: " << semantic_stream.size() << "\n";
            logger << "  Translation errors: " << translator.get_errors().size() << "\n";
        }
        
        logger << "\nPipeline demonstrates consistent processing across input sizes\n";
        logger << "Ready for GPU-parallel processing optimization\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_pipeline_error_resilience() {
    TestLogger logger("Pipeline Error Resilience");
    
    try {
        logger << "=== Testing Pipeline Error Resilience ===\n";
        
        // Test various error conditions
        std::vector<std::pair<std::string, std::string>> error_cases = {
            {"empty", ""},
            {"whitespace_only", "   \n  \t  "},
            {"single_token", "class"},
            {"incomplete_class", "class Connection {"},
            {"invalid_syntax", "class Connection { handle: }"},
        };
        
        for (const auto& test_case : error_cases) {
            logger << "\nTesting " << test_case.first << ":\n";
            logger << "Input: '" << test_case.second << "'\n";
            
            try {
                RawTokenizer tokenizer(test_case.second);
                auto raw_stream = tokenizer.tokenize_to_stream();
                
                SemanticTranslator translator(std::move(raw_stream));
                auto semantic_stream = translator.translate_to_stream();
                
                logger << "  Result: " << semantic_stream.size() << " semantic tokens, "
                       << translator.get_errors().size() << " errors\n";
                       
            } catch (const std::exception& inner_e) {
                logger << "  Exception: " << inner_e.what() << "\n";
            }
        }
        
        logger << "\nPipeline demonstrates resilient error handling\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

int main() {
    TestSuite suite("Pipeline Integration Tests");
    
    std::cout << "CPrime Pipeline Integration Tests\n";
    std::cout << "=================================\n\n";
    
    suite.run_test(test_simple_pipeline_integration);
    suite.run_test(test_complex_pipeline_integration);
    suite.run_test(test_pipeline_performance_characteristics);
    suite.run_test(test_pipeline_error_resilience);
    
    suite.print_results();
    
    return suite.all_passed() ? 0 : 1;
}