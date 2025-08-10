#include "orchestrator.h"
#include "layer0/input_processor.h"
#include "commons/logger.h"

namespace cprime {

CompilerOrchestrator::CompilerOrchestrator(CompilationParameters params)
    : params_(std::move(params)), logger_(cprime::LoggerFactory::get_logger("orchestrator")) {
    
    if (!validate_parameters()) {
        throw std::runtime_error("Invalid compilation parameters provided to orchestrator");
    }
    
    logger_.debug("CompilerOrchestrator initialized with {} input files", params_.input_files.size());
    logger_.debug("Output file: {}", params_.output_file.string());
    logger_.debug("Verbose: {}, Debug: {}", params_.verbose, params_.debug_mode);
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
        logger_.error("Layer 0 failed: No input streams processed");
        log_layer_end("Layer 0", false);
        return false;
    }
    
    // TODO: Store input_streams in CompilationContext
    // TODO: Initialize scope vector after Layer 0 completes
    // TODO: Initialize error handling system
    
    // Success logging with stream details
    logger_.info("Layer 0 completed: {} input streams processed", input_streams.size());
    
    for (const auto& [stream_id, stream] : input_streams) {
        logger_.debug("  Stream '{}': {} characters", stream_id, stream.str().length());
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
    if (!params_.validate()) {
        logger_.error("Error: Compilation parameters validation failed");
        return false;
    }
    
    // Additional orchestrator-specific validation
    for (const auto& file : params_.input_files) {
        if (file.empty()) {
            logger_.error("Error: Empty file path in input files");
            return false;
        }
    }
    
    if (params_.output_file.empty()) {
        logger_.error("Error: Output file path is empty");
        return false;
    }
    
    logger_.debug("Compilation parameters validated successfully");
    return true;
}

// TODO: Implement proper logging system setup
// void CompilerOrchestrator::setup_logging() { ... }

void CompilerOrchestrator::log_compilation_start() {
    logger_.info("=== CPrime Compilation Started ===");
    logger_.info("Input files: {}", params_.input_files.size());
    
    for (const auto& file : params_.input_files) {
        logger_.info("  - {}", file.string());
    }
    
    logger_.info("Output file: {}", params_.output_file.string());
    if (params_.verbose) {
        logger_.debug("Verbose: {}", params_.verbose ? "true" : "false");
        logger_.debug("Debug mode: {}", params_.debug_mode ? "true" : "false");
    }
}

void CompilerOrchestrator::log_compilation_end(bool success) {
    if (success) {
        logger_.info("=== CPrime Compilation Completed Successfully ===");
    } else {
        logger_.error("=== CPrime Compilation Failed ===");
    }
    
    // TODO: Log final context state when CompilationContext is implemented
}

void CompilerOrchestrator::log_layer_start(const std::string& layer_name) {
    logger_.debug("--- Starting {} ---", layer_name);
}

void CompilerOrchestrator::log_layer_end(const std::string& layer_name, bool success) {
    if (success) {
        logger_.debug("--- {} Completed Successfully ---", layer_name);
    } else {
        logger_.error("--- {} Failed ---", layer_name);
    }
}

// TODO: Implement proper error handling system
// void CompilerOrchestrator::handle_layer_failure(...) { ... }

} // namespace cprime