#include "error_handler.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace cprime {

VoidResult ErrorHandler::process_all_errors(CompilationContext& context) {
    ProcessingState state(context);
    
    CPRIME_LOG_INFO(state.logger, "=== ErrorHandler Processing Started ===");
    CPRIME_LOG_DEBUG(state.logger, "Collected {} errors from all layers", 
                     context.error_collector.get_total_error_count());
    
    // Validate error references before processing
    validate_error_references(state);
    
    // Generate comprehensive error report
    generate_error_report(state);
    
    // Output the report
    output_report(state);
    
    // Generate final statistics
    generate_summary(state);
    
    // Determine compilation result
    bool compilation_succeeds = should_compilation_succeed(context);
    
    CPRIME_LOG_INFO(state.logger, "ErrorHandler completed - Compilation {}", 
                    compilation_succeeds ? "SUCCEEDS" : "FAILS");
    
    return compilation_succeeds ? success() : failure<bool>("Compilation failed due to errors");
}

void ErrorHandler::generate_error_report(ProcessingState& state) {
    const auto& collector = state.context.error_collector;
    
    if (collector.empty()) {
        state.report_output << "No errors or warnings found.\n";
        return;
    }
    
    state.report_output << "\n=== COMPILATION REPORT ===\n\n";
    
    // Process errors organized by layer for logical flow
    process_errors_by_layer(state);
}

void ErrorHandler::process_errors_by_layer(ProcessingState& state) {
    // Process each layer's errors in order
    const ErrorLayer layers[] = {
        ErrorLayer::Layer0, ErrorLayer::Layer1, 
        ErrorLayer::Layer2, ErrorLayer::Layer3, ErrorLayer::Layer4
    };
    
    for (ErrorLayer layer : layers) {
        auto layer_errors = state.context.error_collector.get_errors_by_layer(layer);
        if (!layer_errors.empty()) {
            process_layer_errors(state, layer);
        }
    }
}

void ErrorHandler::process_layer_errors(ProcessingState& state, ErrorLayer layer) {
    auto layer_errors = state.context.error_collector.get_errors_by_layer(layer);
    
    // Sort errors for optimal reporting order
    layer_errors = sort_errors_for_reporting(layer_errors);
    
    // Add layer header
    state.report_output << "--- " << layer_errors[0].get_layer_name() << " ---\n\n";
    
    // Process each error in this layer
    for (const auto& error : layer_errors) {
        format_error(state, error);
        state.report_output << "\n";
        
        if (error.severity >= ErrorSeverity::Error) {
            state.errors_processed++;
        } else {
            state.warnings_processed++;
        }
    }
}

void ErrorHandler::format_error(ProcessingState& state, const LayerError& error) {
    // Generate error header with position
    state.report_output << generate_error_header(state, error) << "\n";
    
    // Add main error message
    state.report_output << "  " << error.message << "\n";
    
    // Add detailed description if available
    if (!error.detailed_description.empty()) {
        state.report_output << "  Details: " << error.detailed_description << "\n";
    }
    
    // Generate source context
    std::string source_context = generate_source_context(state, error);
    if (!source_context.empty()) {
        state.report_output << "\n" << source_context << "\n";
    }
    
    // Add suggestions
    std::string suggestions = generate_suggestions(error);
    if (!suggestions.empty()) {
        state.report_output << suggestions << "\n";
    }
}

std::string ErrorHandler::generate_error_header(const ProcessingState& state, const LayerError& error) {
    std::string header;
    
    // Get source position if available
    if (!error.related_token_indices.empty()) {
        auto position = state.context.error_collector.get_token_position(error.related_token_indices[0]);
        if (!position.file_path.empty()) {
            header = position.to_string();
        }
    }
    
    // Fallback to scope information
    if (header.empty() && error.related_scope) {
        header = "Scope[" + std::to_string(error.related_scope->scope_index) + "]";
    }
    
    // Add severity and error code
    header += ": " + colorize_severity(error.severity, error.get_severity_name());
    header += " [" + std::to_string(error.error_code) + "]";
    
    return header;
}

std::string ErrorHandler::generate_source_context(const ProcessingState& state, const LayerError& error) {
    if (error.related_token_indices.empty()) {
        return "";
    }
    
    return state.context.error_collector.get_source_context(error, 2);
}

std::string ErrorHandler::generate_suggestions(const LayerError& error) {
    if (error.suggested_fixes.empty()) {
        return "";
    }
    
    std::string suggestions = "  Suggestions:\n";
    for (const auto& suggestion : error.suggested_fixes) {
        suggestions += "    - " + suggestion + "\n";
    }
    
    return suggestions;
}

void ErrorHandler::generate_summary(ProcessingState& state) {
    auto stats = state.context.error_collector.get_statistics();
    
    state.report_output << "\n=== SUMMARY ===\n";
    state.report_output << stats.to_string() << "\n";
    
    if (stats.has_blocking_errors) {
        state.report_output << "Compilation FAILED due to errors.\n";
    } else {
        state.report_output << "Compilation can proceed.\n";
        if (stats.warning_count > 0) {
            state.report_output << "Note: " << stats.warning_count << " warnings found.\n";
        }
    }
    
    CPRIME_LOG_INFO(state.logger, "Processing summary: {} errors, {} warnings", 
                    state.errors_processed, state.warnings_processed);
}

void ErrorHandler::output_report(const ProcessingState& state) {
    std::string report = state.report_output.str();
    
    // Output to console
    std::cout << report << std::endl;
    
    // Also log the summary for debugging
    auto stats = state.context.error_collector.get_statistics();
    CPRIME_LOG_DEBUG(state.logger, "Error report generated: {} characters", report.length());
    CPRIME_LOG_DEBUG(state.logger, "{}", stats.to_string());
}

bool ErrorHandler::should_compilation_succeed(const CompilationContext& context) {
    return context.error_collector.compilation_should_succeed();
}

std::string ErrorHandler::colorize_severity(ErrorSeverity severity, const std::string& text) {
    // For now, just return the text as-is
    // In a full implementation, could add ANSI color codes for terminal output
    switch (severity) {
        case ErrorSeverity::Fatal:
        case ErrorSeverity::Error:
            return text; // Could be red
        case ErrorSeverity::Warning:
            return text; // Could be yellow
        case ErrorSeverity::Info:
            return text; // Could be blue
        default:
            return text;
    }
}

std::string ErrorHandler::format_source_line(const std::string& line, uint32_t line_number, bool highlight) {
    std::stringstream ss;
    ss << std::setw(4) << line_number << " | " << line;
    return ss.str();
}

std::vector<LayerError> ErrorHandler::sort_errors_for_reporting(const std::vector<LayerError>& errors) {
    std::vector<LayerError> sorted_errors = errors;
    
    // Sort by severity (most severe first), then by position if available
    std::sort(sorted_errors.begin(), sorted_errors.end(), 
        [](const LayerError& a, const LayerError& b) {
            // First, sort by severity (higher severity first)
            if (a.severity != b.severity) {
                return a.severity > b.severity;
            }
            
            // Then by token position if available
            if (!a.related_token_indices.empty() && !b.related_token_indices.empty()) {
                return a.related_token_indices[0] < b.related_token_indices[0];
            }
            
            // Finally by error code
            return a.error_code < b.error_code;
        });
    
    return sorted_errors;
}

void ErrorHandler::validate_error_references(const ProcessingState& state) {
    const auto& collector = state.context.error_collector;
    const auto& tokenizer_state = state.context.tokenizer_state;
    
    size_t invalid_references = 0;
    
    for (const auto& error : collector.get_all_errors()) {
        // Validate token references
        for (size_t token_index : error.related_token_indices) {
            if (token_index >= tokenizer_state.get_token_count()) {
                invalid_references++;
                CPRIME_LOG_WARN(state.logger, "Invalid token reference {} in error (max: {})", 
                               token_index, tokenizer_state.get_token_count());
            }
        }
        
        // Validate scope references
        if (error.related_scope && error.related_scope->scope_index >= state.context.scopes.size()) {
            invalid_references++;
            CPRIME_LOG_WARN(state.logger, "Invalid scope reference {} in error (max: {})", 
                           error.related_scope->scope_index, state.context.scopes.size());
        }
    }
    
    if (invalid_references > 0) {
        CPRIME_LOG_WARN(state.logger, "Found {} invalid error references - some errors may lack context", 
                       invalid_references);
    }
}

} // namespace cprime