#pragma once

#include "../commons/compilation_context.h"
#include "../commons/common_types.h"
#include "../commons/logger.h"
#include <string>
#include <vector>
#include <sstream>

namespace cprime {

/**
 * ErrorHandler - Orthogonal error processing component.
 * 
 * Design Philosophy:
 * - NOT a sequential layer (not Layer 5)
 * - Orthogonal to the main processing layers (1-4) 
 * - Runs after all sequential layers complete
 * - Processes all collected errors for comprehensive reporting
 * 
 * Responsibilities:
 * - Process all errors collected during layers 1-4
 * - Correlate errors back to source positions via TokenizerState
 * - Generate comprehensive, user-friendly error reports
 * - Classify and prioritize errors
 * - Suggest fixes where possible
 * - Determine overall compilation success/failure
 */
class ErrorHandler {
public:
    /**
     * Main ErrorHandler entry point.
     * Processes all collected errors and generates comprehensive reports.
     * 
     * @param context Compilation context with collected errors
     * @return VoidResult indicating overall compilation success/failure
     */
    static VoidResult process_all_errors(CompilationContext& context);
    
private:
    /**
     * Error processing state.
     */
    struct ProcessingState {
        CompilationContext& context;
        Logger logger;
        
        // Report generation
        std::stringstream report_output;
        size_t errors_processed;
        size_t warnings_processed;
        
        explicit ProcessingState(CompilationContext& ctx) 
            : context(ctx), logger(CPRIME_LOGGER("ERROR_HANDLER")), errors_processed(0), warnings_processed(0) {}
    };
    
    /**
     * Generate comprehensive error report.
     */
    static void generate_error_report(ProcessingState& state);
    
    /**
     * Process errors by layer for organized reporting.
     */
    static void process_errors_by_layer(ProcessingState& state);
    
    /**
     * Process a single layer's errors.
     */
    static void process_layer_errors(ProcessingState& state, ErrorLayer layer);
    
    /**
     * Format and output a single error with full context.
     */
    static void format_error(ProcessingState& state, const LayerError& error);
    
    /**
     * Generate error header with source position.
     */
    static std::string generate_error_header(const ProcessingState& state, const LayerError& error);
    
    /**
     * Generate source code context for error.
     */
    static std::string generate_source_context(const ProcessingState& state, const LayerError& error);
    
    /**
     * Generate suggestions section.
     */
    static std::string generate_suggestions(const LayerError& error);
    
    /**
     * Generate summary statistics.
     */
    static void generate_summary(ProcessingState& state);
    
    /**
     * Output final report to appropriate channels.
     */
    static void output_report(const ProcessingState& state);
    
    /**
     * Determine if compilation should succeed based on errors.
     */
    static bool should_compilation_succeed(const CompilationContext& context);
    
    /**
     * Color/formatting helpers for terminal output.
     */
    static std::string colorize_severity(ErrorSeverity severity, const std::string& text);
    static std::string format_source_line(const std::string& line, uint32_t line_number, bool highlight);
    
    /**
     * Sort errors for optimal reporting order.
     */
    static std::vector<LayerError> sort_errors_for_reporting(const std::vector<LayerError>& errors);
    
    /**
     * Validate error references are consistent.
     */
    static void validate_error_references(const ProcessingState& state);
};

} // namespace cprime