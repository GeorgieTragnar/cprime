#include "raw_token.h"
#include "semantic_token.h"
#include "semantic_translator.h"
#include "context_stack.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <getopt.h>

using namespace cprime::v2;

struct CLIOptions {
    bool dump_tokens = false;
    bool debug_context = false;
    bool show_help = false;
    std::string input_file;
    std::string output_file;
};

void print_help(const char* program_name) {
    std::cout << "CPrime V2 Compiler - Development CLI\n";
    std::cout << "Usage: " << program_name << " [OPTIONS] [input_file]\n\n";
    std::cout << "OPTIONS:\n";
    std::cout << "  --dump-tokens        Dump raw tokens to output\n";
    std::cout << "  --debug-context      Show context stack and keyword resolution\n";
    std::cout << "  -o, --output FILE    Write output to FILE instead of stdout\n";
    std::cout << "  -h, --help           Show this help message\n\n";
    std::cout << "EXAMPLES:\n";
    std::cout << "  " << program_name << " --dump-tokens source.cp\n";
    std::cout << "  " << program_name << " --debug-context -o debug.txt source.cp\n";
    std::cout << "  echo 'class Foo {}' | " << program_name << " --dump-tokens\n\n";
    std::cout << "If no input file is provided, reads from stdin.\n";
    std::cout << "If no output file is provided, writes to stdout.\n";
}

CLIOptions parse_arguments(int argc, char* argv[]) {
    CLIOptions options;
    
    static struct option long_options[] = {
        {"dump-tokens", no_argument, 0, 't'},
        {"debug-context", no_argument, 0, 'd'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "tdo:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 't':
                options.dump_tokens = true;
                break;
            case 'd':
                options.debug_context = true;
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

int main(int argc, char* argv[]) {
    CLIOptions options = parse_arguments(argc, argv);
    
    if (options.show_help) {
        print_help(argv[0]);
        return 0;
    }
    
    // If no flags specified, show help
    if (!options.dump_tokens && !options.debug_context) {
        std::cerr << "Error: No operation specified. Use --help for usage information.\n";
        return 1;
    }
    
    try {
        std::string source = read_input(options);
        
        if (source.empty()) {
            std::cerr << "Warning: Input is empty\n";
            return 0;
        }
        
        std::ofstream file_stream;
        std::ostream& out = get_output_stream(options, file_stream);
        
        if (options.dump_tokens) {
            RawTokenizer tokenizer(source);
            auto tokens = tokenizer.tokenize();
            dump_raw_tokens(tokens, out);
        }
        
        if (options.debug_context) {
            process_with_debug_context(source, out);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}