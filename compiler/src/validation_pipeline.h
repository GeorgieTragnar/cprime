#pragma once

#include "validation_common.h"
#include "layer1/raw_token.h"
#include "layer2/semantic_token.h"
#include "layer3/ast.h"
#include "layer3/symbol_table.h"

#include "layer1validation/token_sequence_validator.h"
#include "layer2validation/context_validator.h"
#include "layer3validation/ast_structure_validator.h"
#include "layer4validation/raii_constraint_validator.h"

#include <memory>
#include <string>
#include <chrono>

namespace cprime {

/**
 * Complete validation pipeline that orchestrates all validation layers.
 * Provides the main entry point for comprehensive CPrime code validation.
 * 
 * Architecture:
 * - Layer 1 Validation: Token sequence syntax validation
 * - Layer 2 Validation: Context completeness validation
 * - Layer 3 Validation: AST structure and symbol resolution validation
 * - Layer 4 Validation: RAII constraints and constructor/destructor pairing
 */
class ValidationPipeline {
public:
    /**
     * Validation configuration options.
     */
    struct Config {
        bool enable_layer1_validation;
        bool enable_layer2_validation;
        bool enable_layer3_validation;
        bool enable_layer4_validation;
        
        bool fail_fast;              // Stop at first error layer
        bool collect_warnings;        // Include warnings in results
        bool collect_info_messages;  // Include info messages
        
        bool enable_performance_timing;  // Measure validation performance
        
        // Default constructor
        Config() : enable_layer1_validation(true), enable_layer2_validation(true),
                  enable_layer3_validation(true), enable_layer4_validation(true),
                  fail_fast(false), collect_warnings(true), collect_info_messages(false),
                  enable_performance_timing(false) {}
    };
    
    explicit ValidationPipeline(const Config& config = Config());
    
    /**
     * Main validation entry point.
     * Validates complete compilation pipeline from tokens through RAII constraints.
     */
    validation::ValidationResult validate_complete_pipeline(
        const std::vector<RawToken>& raw_tokens,
        const std::vector<SemanticToken>& semantic_tokens,
        std::shared_ptr<ast::CompilationUnit> ast,
        SymbolTable& symbol_table
    );
    
    /**
     * Individual layer validation methods.
     * Can be called independently for focused validation.
     */
    validation::ValidationResult validate_layer1(const std::vector<RawToken>& raw_tokens);
    validation::ValidationResult validate_layer2(const std::vector<SemanticToken>& semantic_tokens);
    validation::ValidationResult validate_layer3(std::shared_ptr<ast::CompilationUnit> ast, SymbolTable& symbol_table);
    validation::ValidationResult validate_layer4(std::shared_ptr<ast::CompilationUnit> ast, SymbolTable& symbol_table);
    
    /**
     * Performance and statistics.
     */
    struct ValidationStatistics {
        std::chrono::milliseconds layer1_time{0};
        std::chrono::milliseconds layer2_time{0};
        std::chrono::milliseconds layer3_time{0};
        std::chrono::milliseconds layer4_time{0};
        std::chrono::milliseconds total_time{0};
        
        size_t total_errors = 0;
        size_t total_warnings = 0;
        size_t total_info_messages = 0;
        
        bool all_layers_passed = false;
    };
    
    const ValidationStatistics& get_statistics() const { return statistics_; }
    
    /**
     * Configuration management.
     */
    void set_config(const Config& config) { config_ = config; }
    const Config& get_config() const { return config_; }
    
private:
    Config config_;
    ValidationStatistics statistics_;
    
    // Validation layer instances
    std::unique_ptr<layer1validation::TokenSequenceValidator> layer1_validator_;
    std::unique_ptr<layer2validation::ContextValidator> layer2_validator_;
    std::unique_ptr<layer3validation::ASTStructureValidator> layer3_validator_;
    std::unique_ptr<layer4validation::RAIIConstraintValidator> layer4_validator_;
    
    // Performance timing helpers
    void start_timing();
    void record_layer_timing(int layer, const std::chrono::steady_clock::time_point& start);
    
    // Statistics collection
    void collect_statistics(const validation::ValidationResult& result);
    void reset_statistics();
    
    // Result filtering based on config
    validation::ValidationResult filter_result(const validation::ValidationResult& result);
};

/**
 * Validation pipeline factory for common use cases.
 */
class ValidationPipelineFactory {
public:
    /**
     * Create validation pipeline for different scenarios.
     */
    
    // Fast validation - minimal checks for quick feedback
    static ValidationPipeline create_fast_validation();
    
    // Complete validation - all checks enabled for thorough analysis
    static ValidationPipeline create_complete_validation();
    
    // RAII-focused validation - emphasizes constructor/destructor checking
    static ValidationPipeline create_raii_focused_validation();
    
    // Development validation - includes warnings and info messages
    static ValidationPipeline create_development_validation();
    
    // Production validation - errors only, optimized for speed
    static ValidationPipeline create_production_validation();
    
private:
    static ValidationPipeline::Config create_config(
        bool layer1, bool layer2, bool layer3, bool layer4,
        bool fail_fast, bool warnings, bool info, bool timing
    );
};

/**
 * Validation result reporter.
 * Formats validation results for different output formats.
 */
class ValidationResultReporter {
public:
    enum class OutputFormat {
        Plain,      // Simple text output
        Colored,    // ANSI color-coded output
        JSON,       // JSON format for tools
        XML,        // XML format for IDEs
        Markdown    // Markdown format for documentation
    };
    
    explicit ValidationResultReporter(OutputFormat format = OutputFormat::Colored);
    
    /**
     * Generate formatted report from validation results.
     */
    std::string generate_report(
        const validation::ValidationResult& result,
        const ValidationPipeline::ValidationStatistics& statistics
    );
    
    /**
     * Generate summary report.
     */
    std::string generate_summary(const ValidationPipeline::ValidationStatistics& statistics);
    
    /**
     * Generate per-layer breakdown.
     */
    std::string generate_layer_breakdown(const ValidationPipeline::ValidationStatistics& statistics);
    
private:
    OutputFormat format_;
    
    // Format-specific implementations
    std::string format_plain(const validation::ValidationResult& result, const ValidationPipeline::ValidationStatistics& stats);
    std::string format_colored(const validation::ValidationResult& result, const ValidationPipeline::ValidationStatistics& stats);
    std::string format_json(const validation::ValidationResult& result, const ValidationPipeline::ValidationStatistics& stats);
    
    // Helper methods
    std::string severity_to_string(validation::Severity severity, bool colored = false);
    std::string format_timing(std::chrono::milliseconds timing);
};

/**
 * Validation integration helper.
 * Simplifies integration of validation pipeline into compilation workflow.
 */
class ValidationIntegration {
public:
    /**
     * Validate source code at specific compilation stage.
     */
    static validation::ValidationResult validate_at_tokenization(
        const std::string& source_code,
        const std::vector<RawToken>& tokens
    );
    
    static validation::ValidationResult validate_at_semantic_analysis(
        const std::vector<RawToken>& raw_tokens,
        const std::vector<SemanticToken>& semantic_tokens
    );
    
    static validation::ValidationResult validate_at_ast_construction(
        const std::vector<RawToken>& raw_tokens,
        const std::vector<SemanticToken>& semantic_tokens,
        std::shared_ptr<ast::CompilationUnit> ast,
        SymbolTable& symbol_table
    );
    
    static validation::ValidationResult validate_before_codegen(
        const std::vector<RawToken>& raw_tokens,
        const std::vector<SemanticToken>& semantic_tokens,
        std::shared_ptr<ast::CompilationUnit> ast,
        SymbolTable& symbol_table
    );
    
    /**
     * Check if compilation should continue based on validation results.
     */
    static bool should_continue_compilation(const validation::ValidationResult& result);
    
    /**
     * Convert validation results to compiler error format.
     */
    static std::vector<std::string> to_compiler_errors(const validation::ValidationResult& result);
    
private:
    static ValidationPipeline get_pipeline_for_stage(const std::string& stage);
};

} // namespace cprime