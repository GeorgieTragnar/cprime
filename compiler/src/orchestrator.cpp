#include "orchestrator.h"
#include "layer0/input_processor.h"
#include "layer1/layer1.h"
#include "commons/logger.h"

namespace cprime {

CompilerOrchestrator::CompilerOrchestrator(CompilationParameters params)
    : params_(std::move(params)), logger_(cprime::LoggerFactory::get_logger("orchestrator")) {
    
    if (!validate_parameters()) {
        throw std::runtime_error("Invalid compilation parameters provided to orchestrator");
    }
    
    auto& logger = logger_; // Alias for LOG macros
    LOG_DEBUG("CompilerOrchestrator initialized with {} input files", params_.input_files.size());
    LOG_DEBUG("Output file: {}", params_.output_file.string());
    LOG_DEBUG("Verbose: {}, Debug: {}", params_.verbose, params_.debug_mode);
}

bool CompilerOrchestrator::run() {
    log_compilation_start();
    
    // Layer 0: Input Processing
    bool success = run_layer0();
    
    // Layer 1: Tokenization
    if (success) {
        success = run_layer1();
    }
    
    // TODO: Add Layer 2 (Structure Building)
    // TODO: Add Layer 3 (Contextualization)
    // TODO: Add Layer 4 (RAII injection)
    // TODO: Add ErrorHandler execution
    
    log_compilation_end(success);
    
    return success;
}

bool CompilerOrchestrator::run_layer0() {
    log_layer_start("Layer 0 (Input Processing)");
    
    auto input_streams = InputProcessor::process_input_files(params_);
    
    if (input_streams.empty()) {
        auto& logger = logger_; // Alias for LOG macros
        LOG_ERROR("Layer 0 failed: No input streams processed");
        log_layer_end("Layer 0", false);
        return false;
    }
    
    // Store input streams for Layer 1
    input_streams_ = std::move(input_streams);
    
    // Success logging with stream details
    auto& logger = logger_; // Alias for LOG macros
    LOG_INFO("Layer 0 completed: {} input streams processed", input_streams_.size());
    
    for (const auto& [stream_id, stream] : input_streams_) {
        LOG_DEBUG("  Stream '{}': {} characters", stream_id, stream.str().length());
    }
    
    log_layer_end("Layer 0", true);
    return true;
}

bool CompilerOrchestrator::run_layer1() {
    log_layer_start("Layer 1 (Tokenization)");
    
    if (input_streams_.empty()) {
        auto& logger = logger_; // Alias for LOG macros
        LOG_ERROR("Layer 1 failed: No input streams from Layer 0");
        log_layer_end("Layer 1", false);
        return false;
    }
    
    auto& logger = logger_; // Alias for LOG macros
    LOG_DEBUG("Starting tokenization of {} input streams", input_streams_.size());
    
    // Tokenize each input stream
    for (auto& [stream_id, stream] : input_streams_) {
        LOG_DEBUG("Tokenizing stream: {}", stream_id);
        
        try {
            // Create ExecAliasRegistry for exec alias detection
            ExecAliasRegistry exec_alias_registry;
            
            // Call the new standardized layer1 function
            auto tokens = layer1(stream, string_table_, exec_alias_registry);
            
            LOG_INFO("Stream '{}' tokenized: {} tokens generated", stream_id, tokens.size());
            
            // TODO: Store tokens in CompilationContext or pass to next layer
            // For now, just log some statistics
            size_t literal_count = 0, identifier_count = 0, keyword_count = 0;
            for (const auto& token : tokens) {
                switch (token._raw_token) {
                    case ERawToken::LITERAL: literal_count++; break;
                    case ERawToken::IDENTIFIER: identifier_count++; break;
                    case ERawToken::KEYWORD: keyword_count++; break;
                    default: break;
                }
            }
            
            LOG_DEBUG("  Literals: {}, Identifiers: {}, Keywords: {}", 
                     literal_count, identifier_count, keyword_count);
            
        } catch (const std::exception& e) {
            LOG_ERROR("Tokenization failed for stream '{}': {}", stream_id, e.what());
            log_layer_end("Layer 1", false);
            return false;
        }
    }
    
    LOG_INFO("Layer 1 completed: All streams tokenized successfully");
    LOG_DEBUG("String table now contains {} unique strings", string_table_.size());
    
    log_layer_end("Layer 1", true);
    return true;
}

// TODO: Implement Layer 2 structure building
// bool CompilerOrchestrator::run_layer2() { ... }

// TODO: Implement ErrorHandler execution
// bool CompilerOrchestrator::run_error_handler() { ... }

bool CompilerOrchestrator::validate_parameters() {
    auto& logger = logger_; // Alias for LOG macros
    if (!params_.validate()) {
        LOG_ERROR("Error: Compilation parameters validation failed");
        return false;
    }
    
    // Additional orchestrator-specific validation
    for (const auto& file : params_.input_files) {
        if (file.empty()) {
            LOG_ERROR("Error: Empty file path in input files");
            return false;
        }
    }
    
    if (params_.output_file.empty()) {
        LOG_ERROR("Error: Output file path is empty");
        return false;
    }
    
    LOG_DEBUG("Compilation parameters validated successfully");
    return true;
}

// TODO: Implement proper logging system setup
// void CompilerOrchestrator::setup_logging() { ... }

void CompilerOrchestrator::log_compilation_start() {
    auto& logger = logger_; // Alias for LOG macros
    LOG_INFO("=== CPrime Compilation Started ===");
    LOG_INFO("Input files: {}", params_.input_files.size());
    
    for (const auto& file : params_.input_files) {
        LOG_INFO("  - {}", file.string());
    }
    
    LOG_INFO("Output file: {}", params_.output_file.string());
    if (params_.verbose) {
        LOG_DEBUG("Verbose: {}", params_.verbose ? "true" : "false");
        LOG_DEBUG("Debug mode: {}", params_.debug_mode ? "true" : "false");
    }
}

void CompilerOrchestrator::log_compilation_end(bool success) {
    auto& logger = logger_; // Alias for LOG macros
    if (success) {
        LOG_INFO("=== CPrime Compilation Completed Successfully ===");
    } else {
        LOG_ERROR("=== CPrime Compilation Failed ===");
    }
    
    // TODO: Log final context state when CompilationContext is implemented
}

void CompilerOrchestrator::log_layer_start(const std::string& layer_name) {
    auto& logger = logger_; // Alias for LOG macros
    LOG_DEBUG("--- Starting {} ---", layer_name);
}

void CompilerOrchestrator::log_layer_end(const std::string& layer_name, bool success) {
    auto& logger = logger_; // Alias for LOG macros
    if (success) {
        LOG_DEBUG("--- {} Completed Successfully ---", layer_name);
    } else {
        LOG_ERROR("--- {} Failed ---", layer_name);
    }
}

// TODO: Implement proper error handling system
// void CompilerOrchestrator::handle_layer_failure(...) { ... }

} // namespace cprime