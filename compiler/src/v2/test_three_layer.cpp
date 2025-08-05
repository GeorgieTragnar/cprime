#include "raw_token.h"
#include "semantic_token.h"
#include "semantic_translator.h"
#include "context_stack.h"
#include <iostream>
#include <string>

using namespace cprime::v2;

void test_raw_tokenization() {
    std::cout << "=== Testing Raw Tokenization ===\n";
    
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
    
    try {
        RawTokenizer tokenizer(test_code);
        auto raw_tokens = tokenizer.tokenize();
        
        std::cout << "Successfully tokenized " << raw_tokens.size() << " raw tokens:\n";
        
        for (size_t i = 0; i < std::min(raw_tokens.size(), size_t(20)); ++i) {
            std::cout << "  " << raw_tokens[i].to_string() << "\n";
        }
        
        if (raw_tokens.size() > 20) {
            std::cout << "  ... (" << (raw_tokens.size() - 20) << " more tokens)\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "Raw tokenization failed: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

void test_context_stack() {
    std::cout << "=== Testing Context Stack ===\n";
    
    ContextStack context_stack;
    
    // Test basic context operations
    context_stack.push(ParseContext::class_definition("Connection", true));
    context_stack.push(ParseContext::access_rights_declaration("UserOps", true));
    
    std::cout << "Context stack depth: " << context_stack.depth() << "\n";
    std::cout << "Current context: " << (context_stack.current() ? context_stack.current()->to_string() : "none") << "\n";
    std::cout << "Is in class definition: " << (context_stack.is_in_class_definition() ? "yes" : "no") << "\n";
    std::cout << "Is in access rights declaration: " << (context_stack.is_in_access_rights_declaration() ? "yes" : "no") << "\n";
    std::cout << "Current class name: " << context_stack.current_class_name() << "\n";
    
    // Test context resolver
    ContextResolver resolver(context_stack);
    auto runtime_interpretation = resolver.resolve_runtime_keyword();
    std::cout << "Runtime keyword interpretation: " << resolver.interpretation_to_string(runtime_interpretation) << "\n";
    
    context_stack.dump_stack();
    std::cout << "\n";
}

void test_semantic_translation() {
    std::cout << "=== Testing Semantic Translation ===\n";
    
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
    
    try {
        // Step 1: Raw tokenization
        RawTokenizer tokenizer(test_code);
        auto raw_token_stream = tokenizer.tokenize_to_stream();
        
        std::cout << "Raw tokens generated: " << raw_token_stream.size() << "\n";
        
        // Step 2: Semantic translation
        SemanticTranslator translator(std::move(raw_token_stream));
        auto semantic_tokens = translator.translate();
        
        std::cout << "Semantic tokens generated: " << semantic_tokens.size() << "\n";
        
        if (translator.has_errors()) {
            std::cout << "Translation errors:\n";
            for (const auto& error : translator.get_errors()) {
                std::cout << "  Error at " << error.line << ":" << error.column 
                         << " - " << error.message << "\n";
                std::cout << "  Context: " << error.context_path << "\n";
            }
        }
        
        // Display semantic tokens
        std::cout << "\nSemantic tokens:\n";
        for (size_t i = 0; i < std::min(semantic_tokens.size(), size_t(15)); ++i) {
            std::cout << "  [" << i << "] " << semantic_tokens[i].to_string() << "\n";
        }
        
        if (semantic_tokens.size() > 15) {
            std::cout << "  ... (" << (semantic_tokens.size() - 15) << " more tokens)\n";
        }
        
        // Test specific semantic token types
        size_t access_right_count = 0;
        size_t defer_count = 0;
        size_t union_count = 0;
        
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
                default:
                    break;
            }
        }
        
        std::cout << "\nSemantic token analysis:\n";
        std::cout << "  Access rights declarations: " << access_right_count << "\n";
        std::cout << "  Defer statements: " << defer_count << "\n";
        std::cout << "  Union declarations: " << union_count << "\n";
        
    } catch (const std::exception& e) {
        std::cout << "Semantic translation failed: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

void test_feature_registry() {
    std::cout << "=== Testing Feature Registry ===\n";
    
    SemanticFeatureRegistry registry;
    
    // Test implementation status queries
    std::cout << "RuntimeAccessRightDeclaration status: " 
              << (registry.is_implemented(SemanticTokenType::RuntimeAccessRightDeclaration) ? "Implemented" : "Not implemented") << "\n";
    std::cout << "Identifier status: " 
              << (registry.is_implemented(SemanticTokenType::Identifier) ? "Implemented" : "Not implemented") << "\n";
    
    // Generate status report
    registry.generate_status_report();
    
    std::cout << "\n";
}

void test_complete_pipeline() {
    std::cout << "=== Testing Complete Three-Layer Pipeline ===\n";
    
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
        
        // Usage with type parameters
        let admin_conn: DatabaseConnection<runtime AdminOps> = create_admin_connection();
    )";
    
    try {
        std::cout << "Processing complex CPrime code...\n";
        
        // Layer 1: Raw tokenization
        RawTokenizer tokenizer(complex_code);
        auto raw_stream = tokenizer.tokenize_to_stream();
        std::cout << "Layer 1 complete: " << raw_stream.size() << " raw tokens\n";
        
        // Layer 2: Semantic translation
        SemanticTranslator translator(std::move(raw_stream));
        auto semantic_stream = translator.translate_to_stream();
        std::cout << "Layer 2 complete: " << semantic_stream.size() << " semantic tokens\n";
        
        if (translator.has_errors()) {
            std::cout << "Errors during translation: " << translator.get_errors().size() << "\n";
        }
        
        // Analyze the semantic tokens
        auto runtime_access_rights = semantic_stream.filter_by_type(SemanticTokenType::RuntimeAccessRightDeclaration);
        auto compile_time_access_rights = semantic_stream.filter_by_type(SemanticTokenType::CompileTimeAccessRightDeclaration);
        auto defer_statements = semantic_stream.filter_by_type(SemanticTokenType::RaiiDefer);
        auto runtime_unions = semantic_stream.filter_by_type(SemanticTokenType::RuntimeUnion);
        
        std::cout << "\nSemantic analysis results:\n";
        std::cout << "  Runtime access rights: " << runtime_access_rights.size() << "\n";
        std::cout << "  Compile-time access rights: " << compile_time_access_rights.size() << "\n";
        std::cout << "  RAII defer statements: " << defer_statements.size() << "\n";
        std::cout << "  Runtime unions: " << runtime_unions.size() << "\n";
        
        // Show some example semantic tokens
        std::cout << "\nExample semantic tokens:\n";
        for (const auto& token : runtime_access_rights) {
            std::cout << "  " << token.to_string() << "\n";
        }
        for (const auto& token : defer_statements) {
            std::cout << "  " << token.to_string() << "\n";
        }
        
        std::cout << "\n✓ Three-layer pipeline completed successfully!\n";
        
        // Layer 3 would be LLVM IR generation (not implemented yet)
        std::cout << "Layer 3 (LLVM IR generation): Not yet implemented\n";
        
    } catch (const std::exception& e) {
        std::cout << "Pipeline failed: " << e.what() << "\n";
    }
    
    std::cout << "\n";
}

int main() {
    std::cout << "CPrime Compiler V2 - Three-Layer Architecture Test\n";
    std::cout << "==================================================\n\n";
    
    test_raw_tokenization();
    test_context_stack();
    test_semantic_translation();
    test_feature_registry();
    test_complete_pipeline();
    
    std::cout << "All tests completed!\n";
    return 0;
}