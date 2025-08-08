#include "validation_pipeline.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace cprime {

// ============================================================================
// ValidationPipeline Implementation
// ============================================================================

ValidationPipeline::ValidationPipeline(const Config& config)
    : config_(config) {
    reset_statistics();
}

validation::ValidationResult ValidationPipeline::validate_complete_pipeline(
    const std::vector<RawToken>& raw_tokens,
    const std::vector<SemanticToken>& semantic_tokens,
    std::shared_ptr<ast::CompilationUnit> ast,
    SymbolTable& symbol_table
) {
    reset_statistics();
    start_timing();
    
    validation::ValidationResult complete_result;
    
    // Layer 1: Token sequence validation
    if (config_.enable_layer1_validation) {
        auto start = std::chrono::steady_clock::now();
        auto layer1_result = validate_layer1(raw_tokens);
        record_layer_timing(1, start);
        
        complete_result.merge(filter_result(layer1_result));
        collect_statistics(layer1_result);
        
        if (config_.fail_fast && !layer1_result.success()) {
            return complete_result;
        }
    }
    
    // Layer 2: Context completeness validation
    if (config_.enable_layer2_validation) {
        auto start = std::chrono::steady_clock::now();
        auto layer2_result = validate_layer2(semantic_tokens);
        record_layer_timing(2, start);
        
        complete_result.merge(filter_result(layer2_result));
        collect_statistics(layer2_result);
        
        if (config_.fail_fast && !layer2_result.success()) {
            return complete_result;
        }
    }
    
    // Layer 3: AST structure validation
    if (config_.enable_layer3_validation) {
        auto start = std::chrono::steady_clock::now();
        auto layer3_result = validate_layer3(ast, symbol_table);
        record_layer_timing(3, start);
        
        complete_result.merge(filter_result(layer3_result));
        collect_statistics(layer3_result);
        
        if (config_.fail_fast && !layer3_result.success()) {
            return complete_result;
        }
    }
    
    // Layer 4: RAII constraint validation
    if (config_.enable_layer4_validation) {
        auto start = std::chrono::steady_clock::now();
        auto layer4_result = validate_layer4(ast, symbol_table);
        record_layer_timing(4, start);
        
        complete_result.merge(filter_result(layer4_result));
        collect_statistics(layer4_result);
    }
    
    statistics_.all_layers_passed = complete_result.success();
    
    return complete_result;
}

validation::ValidationResult ValidationPipeline::validate_layer1(const std::vector<RawToken>& raw_tokens) {
    if (!layer1_validator_) {
        layer1_validator_ = std::make_unique<layer1validation::TokenSequenceValidator>(raw_tokens);
    }
    
    return layer1_validator_->validate();
}

validation::ValidationResult ValidationPipeline::validate_layer2(const std::vector<SemanticToken>& semantic_tokens) {
    if (!layer2_validator_) {
        layer2_validator_ = std::make_unique<layer2validation::ContextValidator>(semantic_tokens);
    }
    
    return layer2_validator_->validate();
}

validation::ValidationResult ValidationPipeline::validate_layer3(std::shared_ptr<ast::CompilationUnit> ast, SymbolTable& symbol_table) {
    if (!layer3_validator_) {
        layer3_validator_ = std::make_unique<layer3validation::ASTStructureValidator>(ast, symbol_table);
    }
    
    return layer3_validator_->validate();
}

validation::ValidationResult ValidationPipeline::validate_layer4(std::shared_ptr<ast::CompilationUnit> ast, SymbolTable& symbol_table) {
    if (!layer4_validator_) {
        layer4_validator_ = std::make_unique<layer4validation::RAIIConstraintValidator>(ast, symbol_table);
    }
    
    return layer4_validator_->validate();
}

void ValidationPipeline::start_timing() {
    if (config_.enable_performance_timing) {
        // Timing is handled per-layer in validate_complete_pipeline
    }
}

void ValidationPipeline::record_layer_timing(int layer, const std::chrono::steady_clock::time_point& start) {
    if (!config_.enable_performance_timing) return;
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    switch (layer) {
        case 1: statistics_.layer1_time = duration; break;
        case 2: statistics_.layer2_time = duration; break;
        case 3: statistics_.layer3_time = duration; break;
        case 4: statistics_.layer4_time = duration; break;
    }
    
    statistics_.total_time += duration;
}

void ValidationPipeline::collect_statistics(const validation::ValidationResult& result) {
    statistics_.total_errors += result.error_count();
    statistics_.total_warnings += result.warning_count();
    
    // Count info messages
    for (const auto& diagnostic : result.get_diagnostics()) {
        if (diagnostic.severity == validation::Severity::Info) {
            statistics_.total_info_messages++;
        }
    }
}

void ValidationPipeline::reset_statistics() {
    statistics_ = ValidationStatistics{};
}

validation::ValidationResult ValidationPipeline::filter_result(const validation::ValidationResult& result) {
    validation::ValidationResult filtered;
    
    for (const auto& diagnostic : result.get_diagnostics()) {
        switch (diagnostic.severity) {
            case validation::Severity::Error:
                filtered.add_diagnostic(diagnostic);
                break;
            case validation::Severity::Warning:
                if (config_.collect_warnings) {
                    filtered.add_diagnostic(diagnostic);
                }
                break;
            case validation::Severity::Info:
                if (config_.collect_info_messages) {
                    filtered.add_diagnostic(diagnostic);
                }
                break;
        }
    }
    
    return filtered;
}

// ============================================================================
// ValidationPipelineFactory Implementation
// ============================================================================

ValidationPipeline ValidationPipelineFactory::create_fast_validation() {
    return ValidationPipeline(create_config(true, true, false, false, true, false, false, false));
}

ValidationPipeline ValidationPipelineFactory::create_complete_validation() {
    return ValidationPipeline(create_config(true, true, true, true, false, true, true, true));
}

ValidationPipeline ValidationPipelineFactory::create_raii_focused_validation() {
    return ValidationPipeline(create_config(true, false, true, true, false, true, false, false));
}

ValidationPipeline ValidationPipelineFactory::create_development_validation() {
    return ValidationPipeline(create_config(true, true, true, true, false, true, true, true));
}

ValidationPipeline ValidationPipelineFactory::create_production_validation() {
    return ValidationPipeline(create_config(true, true, true, true, true, false, false, false));
}

ValidationPipeline::Config ValidationPipelineFactory::create_config(
    bool layer1, bool layer2, bool layer3, bool layer4,
    bool fail_fast, bool warnings, bool info, bool timing
) {
    ValidationPipeline::Config config;
    config.enable_layer1_validation = layer1;
    config.enable_layer2_validation = layer2;
    config.enable_layer3_validation = layer3;
    config.enable_layer4_validation = layer4;
    config.fail_fast = fail_fast;
    config.collect_warnings = warnings;
    config.collect_info_messages = info;
    config.enable_performance_timing = timing;
    return config;
}

// ============================================================================
// ValidationResultReporter Implementation
// ============================================================================

ValidationResultReporter::ValidationResultReporter(OutputFormat format)
    : format_(format) {
}

std::string ValidationResultReporter::generate_report(
    const validation::ValidationResult& result,
    const ValidationPipeline::ValidationStatistics& statistics
) {
    switch (format_) {
        case OutputFormat::Plain:
            return format_plain(result, statistics);
        case OutputFormat::Colored:
            return format_colored(result, statistics);
        case OutputFormat::JSON:
            return format_json(result, statistics);
        default:
            return format_plain(result, statistics);
    }
}

std::string ValidationResultReporter::generate_summary(const ValidationPipeline::ValidationStatistics& statistics) {
    std::stringstream ss;
    
    ss << "=== CPrime Validation Summary ===\n";
    ss << "Errors: " << statistics.total_errors << "\n";
    ss << "Warnings: " << statistics.total_warnings << "\n";
    ss << "Info: " << statistics.total_info_messages << "\n";
    ss << "Status: " << (statistics.all_layers_passed ? "PASSED" : "FAILED") << "\n";
    
    if (statistics.total_time.count() > 0) {
        ss << "Total time: " << format_timing(statistics.total_time) << "\n";
    }
    
    return ss.str();
}

std::string ValidationResultReporter::generate_layer_breakdown(const ValidationPipeline::ValidationStatistics& statistics) {
    std::stringstream ss;
    
    ss << "=== Validation Layer Breakdown ===\n";
    ss << "Layer 1 (Token Sequence): " << format_timing(statistics.layer1_time) << "\n";
    ss << "Layer 2 (Context): " << format_timing(statistics.layer2_time) << "\n";
    ss << "Layer 3 (AST Structure): " << format_timing(statistics.layer3_time) << "\n";
    ss << "Layer 4 (RAII Constraints): " << format_timing(statistics.layer4_time) << "\n";
    ss << "Total: " << format_timing(statistics.total_time) << "\n";
    
    return ss.str();
}

std::string ValidationResultReporter::format_plain(const validation::ValidationResult& result, const ValidationPipeline::ValidationStatistics& /* stats */) {
    std::stringstream ss;
    
    for (const auto& diagnostic : result.get_diagnostics()) {
        ss << severity_to_string(diagnostic.severity, false) << " at " 
           << diagnostic.location.to_string() << ": " << diagnostic.message << "\n";
        
        if (diagnostic.suggestion.has_value() && !diagnostic.suggestion->empty()) {
            ss << "  suggestion: " << *diagnostic.suggestion << "\n";
        }
    }
    
    if (result.get_diagnostics().empty()) {
        ss << "All validations passed.\n";
    }
    
    return ss.str();
}

std::string ValidationResultReporter::format_colored(const validation::ValidationResult& result, const ValidationPipeline::ValidationStatistics& /* stats */) {
    std::stringstream ss;
    
    // ANSI color codes
    const std::string red = "\033[31m";
    const std::string yellow = "\033[33m";
    const std::string blue = "\033[34m";
    const std::string green = "\033[32m";
    const std::string reset = "\033[0m";
    
    for (const auto& diagnostic : result.get_diagnostics()) {
        std::string color;
        switch (diagnostic.severity) {
            case validation::Severity::Error: color = red; break;
            case validation::Severity::Warning: color = yellow; break;
            case validation::Severity::Info: color = blue; break;
        }
        
        ss << color << severity_to_string(diagnostic.severity, true) << reset 
           << " at " << diagnostic.location.to_string() << ": " << diagnostic.message << "\n";
        
        if (diagnostic.suggestion.has_value() && !diagnostic.suggestion->empty()) {
            ss << "  " << green << "suggestion:" << reset << " " << *diagnostic.suggestion << "\n";
        }
    }
    
    if (result.get_diagnostics().empty()) {
        ss << green << "✓ All validations passed." << reset << "\n";
    }
    
    return ss.str();
}

std::string ValidationResultReporter::format_json(const validation::ValidationResult& result, const ValidationPipeline::ValidationStatistics& stats) {
    std::stringstream ss;
    
    ss << "{\n";
    ss << "  \"validation_result\": {\n";
    ss << "    \"success\": " << (result.success() ? "true" : "false") << ",\n";
    ss << "    \"error_count\": " << result.error_count() << ",\n";
    ss << "    \"warning_count\": " << result.warning_count() << ",\n";
    ss << "    \"diagnostics\": [\n";
    
    const auto& diagnostics = result.get_diagnostics();
    for (size_t i = 0; i < diagnostics.size(); ++i) {
        const auto& diag = diagnostics[i];
        ss << "      {\n";
        ss << "        \"severity\": \"" << severity_to_string(diag.severity, false) << "\",\n";
        ss << "        \"message\": \"" << diag.message << "\",\n";
        ss << "        \"location\": {\n";
        ss << "          \"line\": " << diag.location.line << ",\n";
        ss << "          \"column\": " << diag.location.column << "\n";
        ss << "        }";
        
        if (diag.suggestion.has_value()) {
            ss << ",\n        \"suggestion\": \"" << *diag.suggestion << "\"";
        }
        
        ss << "\n      }";
        if (i < diagnostics.size() - 1) ss << ",";
        ss << "\n";
    }
    
    ss << "    ]\n";
    ss << "  },\n";
    ss << "  \"statistics\": {\n";
    ss << "    \"total_time_ms\": " << stats.total_time.count() << ",\n";
    ss << "    \"layer_times_ms\": {\n";
    ss << "      \"layer1\": " << stats.layer1_time.count() << ",\n";
    ss << "      \"layer2\": " << stats.layer2_time.count() << ",\n";
    ss << "      \"layer3\": " << stats.layer3_time.count() << ",\n";
    ss << "      \"layer4\": " << stats.layer4_time.count() << "\n";
    ss << "    }\n";
    ss << "  }\n";
    ss << "}\n";
    
    return ss.str();
}

std::string ValidationResultReporter::severity_to_string(validation::Severity severity, bool colored) {
    switch (severity) {
        case validation::Severity::Error: return colored ? "error" : "error";
        case validation::Severity::Warning: return colored ? "warning" : "warning";  
        case validation::Severity::Info: return colored ? "info" : "info";
    }
    return "unknown";
}

std::string ValidationResultReporter::format_timing(std::chrono::milliseconds timing) {
    if (timing.count() == 0) return "0ms";
    return std::to_string(timing.count()) + "ms";
}

// ============================================================================
// ValidationIntegration Implementation
// ============================================================================

validation::ValidationResult ValidationIntegration::validate_at_tokenization(
    const std::string& /* source_code */,
    const std::vector<RawToken>& tokens
) {
    auto pipeline = ValidationPipelineFactory::create_fast_validation();
    return pipeline.validate_layer1(tokens);
}

validation::ValidationResult ValidationIntegration::validate_at_semantic_analysis(
    const std::vector<RawToken>& raw_tokens,
    const std::vector<SemanticToken>& semantic_tokens
) {
    auto pipeline = ValidationPipelineFactory::create_fast_validation();
    validation::ValidationResult result;
    
    result.merge(pipeline.validate_layer1(raw_tokens));
    result.merge(pipeline.validate_layer2(semantic_tokens));
    
    return result;
}

validation::ValidationResult ValidationIntegration::validate_at_ast_construction(
    const std::vector<RawToken>& raw_tokens,
    const std::vector<SemanticToken>& semantic_tokens,
    std::shared_ptr<ast::CompilationUnit> ast,
    SymbolTable& symbol_table
) {
    auto pipeline = ValidationPipelineFactory::create_complete_validation();
    return pipeline.validate_complete_pipeline(raw_tokens, semantic_tokens, ast, symbol_table);
}

validation::ValidationResult ValidationIntegration::validate_before_codegen(
    const std::vector<RawToken>& raw_tokens,
    const std::vector<SemanticToken>& semantic_tokens,
    std::shared_ptr<ast::CompilationUnit> ast,
    SymbolTable& symbol_table
) {
    auto pipeline = ValidationPipelineFactory::create_raii_focused_validation();
    return pipeline.validate_complete_pipeline(raw_tokens, semantic_tokens, ast, symbol_table);
}

bool ValidationIntegration::should_continue_compilation(const validation::ValidationResult& result) {
    // Only continue if there are no errors (warnings are okay)
    return result.success();
}

std::vector<std::string> ValidationIntegration::to_compiler_errors(const validation::ValidationResult& result) {
    std::vector<std::string> errors;
    
    for (const auto& diagnostic : result.get_diagnostics()) {
        if (diagnostic.severity == validation::Severity::Error) {
            std::string error = "Error at " + diagnostic.location.to_string() + ": " + diagnostic.message;
            if (diagnostic.suggestion.has_value()) {
                error += " (suggestion: " + *diagnostic.suggestion + ")";
            }
            errors.push_back(error);
        }
    }
    
    return errors;
}

ValidationPipeline ValidationIntegration::get_pipeline_for_stage(const std::string& stage) {
    if (stage == "tokenization") {
        return ValidationPipelineFactory::create_fast_validation();
    } else if (stage == "semantic_analysis") {
        return ValidationPipelineFactory::create_development_validation();
    } else if (stage == "ast_construction") {
        return ValidationPipelineFactory::create_complete_validation();
    } else if (stage == "pre_codegen") {
        return ValidationPipelineFactory::create_raii_focused_validation();
    }
    
    return ValidationPipelineFactory::create_complete_validation();
}

} // namespace cprime