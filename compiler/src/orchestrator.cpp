#include "orchestrator.h"
#include "layer0/input_processor.h"
#include "layer1/tokenizer.h"
#include "layer2/structure_builder.h"
#include "errorHandler/error_handler.h"

namespace cprime {

CompilerOrchestrator::CompilerOrchestrator(CompilationParameters params)
    : params_(std::move(params)), logger_(CPRIME_LOGGER("ORCHESTRATOR")) {
    
    setup_logging();
    
    if (!validate_parameters()) {
        throw std::runtime_error("Invalid compilation parameters provided to orchestrator");
    }
    
    CPRIME_LOG_INFO(logger_, "CompilerOrchestrator initialized");
    CPRIME_LOG_DEBUG(logger_, "Parameters: {}", params_.to_string());
}

bool CompilerOrchestrator::run() {
    log_compilation_start();
    
    bool layers_success = true;
    
    // Layer 0: Input Processing
    if (layers_success) {
        layers_success = run_layer0();
    }
    
    // Layer 1: Tokenization (continue even if previous failed to collect more errors)
    if (layers_success) {
        layers_success = run_layer1();
    }
    
    // Layer 2: Structure Building (continue even if previous failed to collect more errors)  
    if (layers_success) {
        layers_success = run_layer2();
    }
    
    // Future layers will be added here:
    // if (layers_success) { layers_success = run_layer3(); }  // Contextualization  
    // if (layers_success) { layers_success = run_layer4(); }  // RAII injection
    
    // Always run ErrorHandler (orthogonal component) - processes all collected errors
    bool final_success = run_error_handler();
    
    log_compilation_end(final_success);
    
    // Final success depends on ErrorHandler's analysis of all collected errors
    return final_success;
}

bool CompilerOrchestrator::run_layer0() {
    log_layer_start("Layer 0 (Input Processing)");
    
    auto result = InputProcessor::process_input_files(params_, context_);
    
    if (!result.success()) {
        handle_layer_failure("Layer 0", result.error());
        log_layer_end("Layer 0", false);
        return false;
    }
    
    // Initialize scope vector after Layer 0 completes
    context_.initialize_scope_vector();
    
    // Initialize error handling system
    context_.initialize_error_system();
    
    // Log success details
    CPRIME_LOG_INFO(logger_, "Layer 0 completed: {} input streams processed", 
                    context_.input_streams.size());
    
    for (const auto& [stream_id, stream] : context_.input_streams) {
        CPRIME_LOG_DEBUG(logger_, "  Stream '{}': {} characters", 
                        stream_id, stream.str().length());
    }
    
    CPRIME_LOG_DEBUG(logger_, "Scope vector initialized with root scope");
    
    log_layer_end("Layer 0", true);
    return true;
}

bool CompilerOrchestrator::run_layer1() {
    log_layer_start("Layer 1 (Tokenization)");
    
    auto result = Tokenizer::tokenize_all_streams(context_);
    
    if (!result.success()) {
        handle_layer_failure("Layer 1", result.error());
        log_layer_end("Layer 1", false);
        return false;
    }
    
    // Log success details
    const auto& root_scope = context_.get_root_scope();
    CPRIME_LOG_INFO(logger_, "Layer 1 completed: {} token streams processed", 
                    root_scope.token_streams.size());
    
    size_t total_tokens = 0;
    for (const auto& [stream_id, tokens] : root_scope.token_streams) {
        CPRIME_LOG_DEBUG(logger_, "  Stream '{}': {} tokens", stream_id, tokens.size());
        total_tokens += tokens.size();
    }
    
    CPRIME_LOG_INFO(logger_, "Total tokens generated: {}", total_tokens);
    
    log_layer_end("Layer 1", true);
    return true;
}

bool CompilerOrchestrator::run_layer2() {
    log_layer_start("Layer 2 (Structure Building)");
    
    auto result = StructureBuilder::build_scope_structure(context_);
    
    if (!result.success()) {
        handle_layer_failure("Layer 2", result.error());
        log_layer_end("Layer 2", false);
        return false;
    }
    
    // Log success details
    CPRIME_LOG_INFO(logger_, "Layer 2 completed: {} scopes built", context_.scopes.size());
    
    size_t total_instructions = 0;
    size_t total_signature_tokens = 0;
    
    for (size_t i = 0; i < context_.scopes.size(); ++i) {
        const auto& scope = context_.scopes[i];
        CPRIME_LOG_DEBUG(logger_, "  Scope [{}]: type={}, instructions={}, sig_tokens={}", 
                        i, static_cast<int>(scope.type), 
                        scope.instruction_groups.size(),
                        scope.signature_tokens.size());
        
        total_instructions += scope.instruction_groups.size();
        total_signature_tokens += scope.signature_tokens.size();
    }
    
    CPRIME_LOG_INFO(logger_, "Total instructions: {}, signature tokens: {}", 
                    total_instructions, total_signature_tokens);
    
    log_layer_end("Layer 2", true);
    return true;
}

bool CompilerOrchestrator::run_error_handler() {
    CPRIME_LOG_INFO(logger_, "--- Running ErrorHandler (Orthogonal Component) ---");
    
    auto result = ErrorHandler::process_all_errors(context_);
    
    if (!result.success()) {
        CPRIME_LOG_ERROR(logger_, "ErrorHandler determined compilation failed: {}", result.error());
        return false;
    }
    
    CPRIME_LOG_INFO(logger_, "ErrorHandler completed - compilation assessment complete");
    return true;
}

bool CompilerOrchestrator::validate_parameters() {
    if (!params_.validate()) {
        CPRIME_LOG_ERROR(logger_, "Compilation parameters validation failed");
        return false;
    }
    
    // Additional orchestrator-specific validation
    for (const auto& file : params_.input_files) {
        if (file.empty()) {
            CPRIME_LOG_ERROR(logger_, "Empty file path in input files");
            return false;
        }
    }
    
    if (params_.output_file.empty()) {
        CPRIME_LOG_ERROR(logger_, "Output file path is empty");
        return false;
    }
    
    return true;
}

void CompilerOrchestrator::setup_logging() {
    // Set logger level based on parameters
    if (params_.debug_mode) {
        logger_.set_level(LogLevel::Debug);
        LoggerFactory::set_global_level(LogLevel::Debug);
    } else if (params_.verbose) {
        logger_.set_level(LogLevel::Info);
        LoggerFactory::set_global_level(LogLevel::Info);
    } else {
        logger_.set_level(LogLevel::Warning);
        LoggerFactory::set_global_level(LogLevel::Warning);
    }
}

void CompilerOrchestrator::log_compilation_start() {
    CPRIME_LOG_INFO(logger_, "=== CPrime Compilation Started ===");
    CPRIME_LOG_INFO(logger_, "Input files: {}", params_.input_files.size());
    
    for (const auto& file : params_.input_files) {
        CPRIME_LOG_INFO(logger_, "  - {}", file.string());
    }
    
    CPRIME_LOG_INFO(logger_, "Output file: {}", params_.output_file.string());
    CPRIME_LOG_DEBUG(logger_, "Verbose: {}", params_.verbose ? "true" : "false");
    CPRIME_LOG_DEBUG(logger_, "Debug mode: {}", params_.debug_mode ? "true" : "false");
}

void CompilerOrchestrator::log_compilation_end(bool success) {
    if (success) {
        CPRIME_LOG_INFO(logger_, "=== CPrime Compilation Completed Successfully ===");
    } else {
        CPRIME_LOG_ERROR(logger_, "=== CPrime Compilation Failed ===");
    }
    
    // Log final context state
    CPRIME_LOG_DEBUG(logger_, "Final compilation context:");
    CPRIME_LOG_DEBUG(logger_, "{}", context_.get_debug_info());
}

void CompilerOrchestrator::log_layer_start(const std::string& layer_name) {
    CPRIME_LOG_INFO(logger_, "--- Starting {} ---", layer_name);
}

void CompilerOrchestrator::log_layer_end(const std::string& layer_name, bool success) {
    if (success) {
        CPRIME_LOG_INFO(logger_, "--- {} Completed Successfully ---", layer_name);
    } else {
        CPRIME_LOG_ERROR(logger_, "--- {} Failed ---", layer_name);
    }
}

void CompilerOrchestrator::handle_layer_failure(const std::string& layer_name, const std::string& error_message) {
    CPRIME_LOG_ERROR(logger_, "{} failed: {}", layer_name, error_message);
    
    // Future: Could add more sophisticated error recovery here
    // For now, just log and let the caller handle the failure
}

} // namespace cprime