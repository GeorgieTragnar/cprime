#include "orchestrator.h"
#include "layer0/input_processor.h"
#include <iostream>

namespace cprime {

CompilerOrchestrator::CompilerOrchestrator(CompilationParameters params)
    : params_(std::move(params)) {
    
    if (!validate_parameters()) {
        throw std::runtime_error("Invalid compilation parameters provided to orchestrator");
    }
    
    // TODO: Initialize logging system
    // TODO: Log initialization details
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
        std::cerr << "Layer 0 failed: No input streams processed" << std::endl;
        log_layer_end("Layer 0", false);
        return false;
    }
    
    // TODO: Store input_streams in CompilationContext
    // TODO: Initialize scope vector after Layer 0 completes
    // TODO: Initialize error handling system
    
    // Basic success logging
    std::cout << "Layer 0 completed: " << input_streams.size() << " input streams processed" << std::endl;
    
    for (const auto& [stream_id, stream] : input_streams) {
        std::cout << "  Stream '" << stream_id << "': " << stream.str().length() << " characters" << std::endl;
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
        std::cerr << "Error: Compilation parameters validation failed" << std::endl;
        return false;
    }
    
    // Additional orchestrator-specific validation
    for (const auto& file : params_.input_files) {
        if (file.empty()) {
            std::cerr << "Error: Empty file path in input files" << std::endl;
            return false;
        }
    }
    
    if (params_.output_file.empty()) {
        std::cerr << "Error: Output file path is empty" << std::endl;
        return false;
    }
    
    return true;
}

// TODO: Implement proper logging system setup
// void CompilerOrchestrator::setup_logging() { ... }

void CompilerOrchestrator::log_compilation_start() {
    std::cout << "=== CPrime Compilation Started ===" << std::endl;
    std::cout << "Input files: " << params_.input_files.size() << std::endl;
    
    for (const auto& file : params_.input_files) {
        std::cout << "  - " << file.string() << std::endl;
    }
    
    std::cout << "Output file: " << params_.output_file.string() << std::endl;
    if (params_.verbose) {
        std::cout << "Verbose: " << (params_.verbose ? "true" : "false") << std::endl;
        std::cout << "Debug mode: " << (params_.debug_mode ? "true" : "false") << std::endl;
    }
}

void CompilerOrchestrator::log_compilation_end(bool success) {
    if (success) {
        std::cout << "=== CPrime Compilation Completed Successfully ===" << std::endl;
    } else {
        std::cout << "=== CPrime Compilation Failed ===" << std::endl;
    }
    
    // TODO: Log final context state when CompilationContext is implemented
}

void CompilerOrchestrator::log_layer_start(const std::string& layer_name) {
    if (params_.verbose) {
        std::cout << "--- Starting " << layer_name << " ---" << std::endl;
    }
}

void CompilerOrchestrator::log_layer_end(const std::string& layer_name, bool success) {
    if (params_.verbose) {
        if (success) {
            std::cout << "--- " << layer_name << " Completed Successfully ---" << std::endl;
        } else {
            std::cout << "--- " << layer_name << " Failed ---" << std::endl;
        }
    }
}

// TODO: Implement proper error handling system
// void CompilerOrchestrator::handle_layer_failure(...) { ... }

} // namespace cprime