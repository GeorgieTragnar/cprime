#include "orchestrator.h"
#include "layer0/input_processor.h"
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
    
    // TODO: Add Layer 1 (Tokenization)
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
    
    // TODO: Store input_streams in CompilationContext
    // TODO: Initialize scope vector after Layer 0 completes
    // TODO: Initialize error handling system
    
    // Success logging with stream details
    auto& logger = logger_; // Alias for LOG macros
    LOG_INFO("Layer 0 completed: {} input streams processed", input_streams.size());
    
    for (const auto& [stream_id, stream] : input_streams) {
        LOG_DEBUG("  Stream '{}': {} characters", stream_id, stream.str().length());
    }
    
    log_layer_end("Layer 0", true);
    return true;
}

// TODO: Implement Layer 1 tokenization
// bool CompilerOrchestrator::run_layer1() { ... }

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