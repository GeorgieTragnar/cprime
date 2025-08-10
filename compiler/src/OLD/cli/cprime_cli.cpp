#include "../common/logger.h"
#include "../common/common_types.h"
#include "../layer1/raw_token.h"
#include "../layer2/semantic_token.h"
#include "../layer2/semantic_translator.h"
#include "../layer1/context_stack.h"
#include "../layer3/ast_builder.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <getopt.h>
#include <filesystem>

using namespace cprime;

struct CLIOptions {
    bool dump_tokens = false;
    bool debug_context = false;
    bool build_ast = false;
    bool dump_ast = false;
    bool dump_symbols = false;
    bool pipeline_status = false;
    bool show_help = false;
    std::string input_file;
    std::string output_file;
};

void print_help(const char* program_name) {
    std::cout << cprime::VersionInfo::get_full_version_string() << " - Development CLI\n";
    std::cout << "Usage: " << program_name << " [OPTIONS] [input_file]\n\n";
    std::cout << "OPTIONS:\n";
    std::cout << "  --dump-tokens        Dump raw tokens to output\n";
    std::cout << "  --debug-context      Show context stack and keyword resolution\n";
    std::cout << "  --build-ast          Build AST from context-enriched tokens\n";
    std::cout << "  --dump-ast           Show AST structure\n";
    std::cout << "  --dump-symbols       Show symbol table\n";
    std::cout << "  --pipeline-status    Show compilation pipeline status\n";
    std::cout << "  -o, --output FILE    Write output to FILE instead of stdout\n";
    std::cout << "  -h, --help           Show this help message\n\n";
    std::cout << "EXAMPLES:\n";
    std::cout << "  " << program_name << " --dump-tokens source.cp\n";
    std::cout << "  " << program_name << " --debug-context --build-ast source.cp\n";
    std::cout << "  " << program_name << " --pipeline-status\n";
    std::cout << "  echo 'class Foo {}' | " << program_name << " --build-ast --dump-ast\n\n";
    std::cout << "If no input file is provided, reads from stdin.\n";
    std::cout << "If no output file is provided, writes to stdout.\n";
}

CLIOptions parse_arguments(int argc, char* argv[]) {
    CLIOptions options;
    
    static struct option long_options[] = {
        {"dump-tokens", no_argument, 0, 't'},
        {"debug-context", no_argument, 0, 'd'},
        {"build-ast", no_argument, 0, 'b'},
        {"dump-ast", no_argument, 0, 'a'},
        {"dump-symbols", no_argument, 0, 's'},
        {"pipeline-status", no_argument, 0, 'p'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "tdbaspo:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 't':
                options.dump_tokens = true;
                break;
            case 'd':
                options.debug_context = true;
                break;
            case 'b':
                options.build_ast = true;
                break;
            case 'a':
                options.dump_ast = true;
                break;
            case 's':
                options.dump_symbols = true;
                break;
            case 'p':
                options.pipeline_status = true;
                break;
            case 'o':
                options.output_file = optarg;
                break;
            case 'h':
                options.show_help = true;
                break;
            case '?':
                // getopt_long already printed an error message
                exit(1);
            default:
                abort();
        }
    }
    
    // Get input file if provided
    if (optind < argc) {
        options.input_file = argv[optind];
    }
    
    return options;
}

std::string read_input(const CLIOptions& options) {
    std::string content;
    
    if (options.input_file.empty()) {
        // Read from stdin
        std::string line;
        while (std::getline(std::cin, line)) {
            content += line + "\n";
        }
    } else {
        // Read from file
        std::ifstream file(options.input_file);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open input file '" << options.input_file << "'\n";
            exit(1);
        }
        
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }
        file.close();
    }
    
    return content;
}

std::ostream& get_output_stream(const CLIOptions& options, std::ofstream& file_stream) {
    if (options.output_file.empty()) {
        return std::cout;
    } else {
        file_stream.open(options.output_file);
        if (!file_stream.is_open()) {
            std::cerr << "Error: Cannot open output file '" << options.output_file << "'\n";
            exit(1);
        }
        return file_stream;
    }
}

void dump_raw_tokens(const std::vector<RawToken>& tokens, std::ostream& out) {
    out << "=== Raw Token Dump ===\n";
    out << "Total tokens: " << tokens.size() << "\n\n";
    
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        out << "[" << i << "] " << token.to_string() << "\n";
    }
    out << "\n";
}

void process_with_debug_context(const std::string& source, std::ostream& out) {
    out << "=== Debug Context Processing ===\n\n";
    
    // Layer 1: Raw tokenization
    out << "--- Layer 1: Raw Tokenization ---\n";
    RawTokenizer tokenizer(source);
    auto raw_stream = tokenizer.tokenize_to_stream();
    auto& raw_tokens = raw_stream.get_tokens();
    
    out << "Raw tokens generated: " << raw_tokens.size() << "\n";
    for (size_t i = 0; i < std::min(raw_tokens.size(), size_t(10)); ++i) {
        out << "  [" << i << "] " << raw_tokens[i].to_string() << "\n";
    }
    if (raw_tokens.size() > 10) {
        out << "  ... (" << (raw_tokens.size() - 10) << " more tokens)\n";
    }
    out << "\n";
    
    // Layer 2: Semantic translation with context debugging
    out << "--- Layer 2: Semantic Translation with Context Debugging ---\n";
    
    // Create context stack for debugging
    ContextStack context_stack;
    ContextResolver resolver(context_stack);
    
    out << "Starting semantic translation...\n\n";
    
    // Process first few tokens to show context resolution
    raw_stream.set_position(0);
    int token_count = 0;
    const int max_debug_tokens = 20;
    
    while (!raw_stream.is_at_end() && token_count < max_debug_tokens) {
        const auto& current_token = raw_stream.current();
        
        // Skip whitespace and comments for cleaner output
        if (current_token.type == RawTokenType::WHITESPACE || 
            current_token.type == RawTokenType::COMMENT) {
            raw_stream.advance();
            continue;
        }
        
        out << "Processing token [" << token_count << "]: " << current_token.to_string() << "\n";
        
        // Show current context
        if (context_stack.current()) {
            out << "  Current context: " << context_stack.current()->to_string() << "\n";
        } else {
            out << "  Current context: none\n";
        }
        
        // If it's a keyword, show how it would be resolved
        if (current_token.type == RawTokenType::KEYWORD) {
            if (current_token.value == "runtime") {
                auto interpretation = resolver.resolve_runtime_keyword();
                out << "  Keyword 'runtime' resolved as: " 
                    << resolver.interpretation_to_string(interpretation) << "\n";
            } else if (current_token.value == "defer") {
                auto interpretation = resolver.resolve_defer_keyword();
                out << "  Keyword 'defer' resolved as: " 
                    << resolver.interpretation_to_string(interpretation) << "\n";
            } else if (current_token.value == "exposes") {
                auto interpretation = resolver.resolve_exposes_keyword();
                out << "  Keyword 'exposes' resolved as: " 
                    << resolver.interpretation_to_string(interpretation) << "\n";
            } else {
                out << "  Keyword '" << current_token.value << "' (context resolution not implemented in CLI)\n";
            }
        }
        
        // Simulate context changes for common keywords
        if (current_token.is_keyword("class")) {
            context_stack.push(ParseContextType::ClassDefinition);
            out << "  -> Pushed ClassDefinition context\n";
        } else if (current_token.is_punctuation("{")) {
            if (context_stack.current() && 
                context_stack.current()->type == ParseContextType::ClassDefinition) {
                context_stack.push(ParseContextType::Block);
                out << "  -> Pushed Block context (class body)\n";
            }
        } else if (current_token.is_punctuation("}")) {
            if (context_stack.current()) {
                context_stack.pop();
                out << "  -> Popped context\n";
            }
        }
        
        out << "\n";
        raw_stream.advance();
        token_count++;
    }
    
    if (!raw_stream.is_at_end()) {
        out << "... (remaining tokens not shown in debug output)\n\n";
    }
    
    // Show final context stack state
    out << "Final context stack depth: " << context_stack.depth() << "\n";
    if (context_stack.current()) {
        out << "Final context: " << context_stack.current()->to_string() << "\n";
    }
}

void build_ast_from_source(const std::string& source, std::ostream& out, 
                          bool dump_ast = false, bool dump_symbols = false) {
    out << "=== AST Building Pipeline ===\n\n";
    
    // Layer 1: Raw tokenization
    out << "--- Layer 1: Raw Tokenization ---\n";
    RawTokenizer tokenizer(source);
    auto raw_tokens = tokenizer.tokenize();
    out << "Generated " << raw_tokens.size() << " raw tokens\n\n";
    
    // Layer 2: Context enrichment (simplified)
    out << "--- Layer 2: Context Enrichment ---\n";
    std::vector<ContextualToken> contextual_tokens;
    ContextStack context_stack;
    ParseContextType current_context = ParseContextType::TopLevel;
    
    for (const auto& raw_token : raw_tokens) {
        // Skip whitespace for cleaner processing
        if (raw_token.type == RawTokenType::WHITESPACE) {
            continue;
        }
        
        // Basic context tracking
        if (raw_token.is_keyword("class")) {
            current_context = ParseContextType::ClassDefinition;
        } else if (raw_token.is_punctuation("{")) {
            context_stack.push(ParseContext(current_context));
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
        
        // Add basic context resolution
        if (raw_token.is_keyword("class")) {
            contextual_token.context_resolution = "ClassDeclaration";
            contextual_token.set_attribute("class_type", "data");
        } else if (raw_token.is_keyword("runtime")) {
            contextual_token.context_resolution = "RuntimeAccessRight";
            contextual_token.set_attribute("access_type", "runtime");
        } else if (raw_token.is_keyword("exposes")) {
            contextual_token.context_resolution = "AccessRightDeclaration";
        }
        
        contextual_tokens.push_back(contextual_token);
    }
    
    out << "Generated " << contextual_tokens.size() << " contextual tokens\n\n";
    
    // Layer 3: AST Building
    out << "--- Layer 3: AST Building ---\n";
    ContextualTokenStream stream(contextual_tokens);
    ASTBuilder builder;
    
    auto ast = builder.build(stream);
    
    if (builder.has_errors()) {
        out << "❌ AST building failed with errors:\n";
        for (const auto& error : builder.get_errors()) {
            out << "  Line " << error.location.line << ":" << error.location.column
                << " - " << error.message << "\n";
        }
        return;
    }
    
    out << "✅ AST built successfully!\n";
    
    if (dump_ast && ast) {
        out << "\n--- AST Structure ---\n";
        out << ast->to_string() << "\n";
        
        out << "\nAST Summary:\n";
        out << "  CompilationUnit with " << ast->get_declarations().size() << " declarations\n";
        
        for (const auto& decl : ast->get_declarations()) {
            if (auto class_decl = std::dynamic_pointer_cast<ast::ClassDecl>(decl)) {
                out << "    - Class: " << class_decl->get_name() << "\n";
                out << "      Members: " << class_decl->get_members().size() << "\n";
                out << "      Access Rights: " << class_decl->get_access_rights().size() << "\n";
            }
        }
    }
    
    if (dump_symbols) {
        out << "\n--- Symbol Table ---\n";
        builder.get_symbol_table().dump();
    }
}

void show_pipeline_status(std::ostream& out) {
    out << "=== CPrime Compiler Pipeline Status ===\n\n";
    
    out << std::left;
    out << std::setw(20) << "Layer" << std::setw(30) << "Component" << std::setw(15) << "Status" << "Description\n";
    out << std::string(80, '-') << "\n";
    
    out << std::setw(20) << "Layer 1" << std::setw(30) << "Raw Tokenizer" 
        << std::setw(15) << "✅ Complete" << "C++ compatible tokenization\n";
    
    out << std::setw(20) << "Layer 2" << std::setw(30) << "Context Enricher" 
        << std::setw(15) << "✅ Complete" << "1:1 token mapping with context\n";
    
    out << std::setw(20) << "Layer 3" << std::setw(30) << "AST Builder" 
        << std::setw(15) << "🔧 Partial" << "Pure construction, basic impl\n";
    
    out << std::setw(20) << "Layer 4a" << std::setw(30) << "Semantic Validator" 
        << std::setw(15) << "📋 Planned" << "Type checking & validation\n";
    
    out << std::setw(20) << "Layer 4b" << std::setw(30) << "Optimizer" 
        << std::setw(15) << "📋 Planned" << "Parallel optimization passes\n";
    
    out << std::setw(20) << "Layer 5" << std::setw(30) << "Code Generator" 
        << std::setw(15) << "📋 Planned" << "LLVM IR generation\n";
    
    out << "\nArchitecture Features:\n";
    out << "  ✅ Context-enriched tokens with perfect source mapping\n";
    out << "  ✅ Self-contained tokens for parallel processing\n";
    out << "  ✅ GPU-ready compilation architecture\n";
    out << "  ✅ Immutable AST design\n";
    out << "  🔧 Parallel validation and optimization\n";
    out << "  📋 LLVM integration\n";
    
    out << "\nNext Steps:\n";
    out << "  1. Complete AST builder implementation\n";
    out << "  2. Implement semantic validator\n";
    out << "  3. Add parallel optimizer\n";
    out << "  4. Integrate LLVM code generation\n";
    out << "  5. GPU kernel implementation\n";
}

int main(int argc, char* argv[]) {
    // Create logs directory if it doesn't exist
    std::filesystem::create_directories("logs");
    
    // Initialize CLI logger
    auto logger = CPRIME_LOGGER("cli");
    logger->set_level(spdlog::level::debug);
    
    CLIOptions options = parse_arguments(argc, argv);
    
    if (options.show_help) {
        print_help(argv[0]);
        return 0;
    }
    
    // Handle pipeline status first (doesn't need input)
    if (options.pipeline_status) {
        std::ofstream file_stream;
        std::ostream& out = get_output_stream(options, file_stream);
        show_pipeline_status(out);
        return 0;
    }
    
    // If no other flags specified, show help
    if (!options.dump_tokens && !options.debug_context && !options.build_ast && 
        !options.dump_ast && !options.dump_symbols) {
        std::cerr << "Error: No operation specified. Use --help for usage information.\n";
        return 1;
    }
    
    try {
        std::string source = read_input(options);
        
        if (source.empty()) {
            CPRIME_LOG_WARN("Input is empty");
            return 0;
        }
        
        CPRIME_LOG_DEBUG("Processing source input, {} characters", source.length());
        
        std::ofstream file_stream;
        std::ostream& out = get_output_stream(options, file_stream);
        
        if (options.dump_tokens) {
            CPRIME_LOG_DEBUG("Dumping raw tokens");
            RawTokenizer tokenizer(source);
            auto tokens = tokenizer.tokenize();
            dump_raw_tokens(tokens, out);
        }
        
        if (options.debug_context) {
            CPRIME_LOG_DEBUG("Processing with debug context");
            process_with_debug_context(source, out);
        }
        
        if (options.build_ast || options.dump_ast || options.dump_symbols) {
            CPRIME_LOG_DEBUG("Building AST from source");
            build_ast_from_source(source, out, options.dump_ast, options.dump_symbols);
        }
        
    } catch (const std::exception& e) {
        CPRIME_LOG_ERROR("CLI Error: {}", e.what());
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}