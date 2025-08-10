#pragma once

#include "error_types.h"
#include "tokenizer_state.h"
#include <vector>
#include <unordered_map>
#include <functional>

namespace cprime {

/**
 * ErrorCollector - Central error collection owned by CompilationContext.
 * 
 * Responsibilities:
 * - Collect errors from all layers during processing
 * - Provide error correlation with TokenizerState
 * - Support error filtering and classification
 * - Enable continuation of processing after non-fatal errors
 * 
 * Design: Owned by CompilationContext, used by all layers and ErrorHandler.
 */
class ErrorCollector {
private:
    std::vector<LayerError> collected_errors_;
    const TokenizerState* tokenizer_state_;  // Reference for source correlation
    
    // Statistics tracking
    size_t error_count_by_severity_[4] = {0}; // Index matches ErrorSeverity enum
    size_t error_count_by_layer_[5] = {0};    // Index matches ErrorLayer enum (0-4)
    
public:
    explicit ErrorCollector(const TokenizerState* tokenizer_state = nullptr)
        : tokenizer_state_(tokenizer_state) {}
    
    /**
     * Set the tokenizer state reference for source correlation.
     */
    void set_tokenizer_state(const TokenizerState* tokenizer_state) {
        tokenizer_state_ = tokenizer_state;
    }
    
    /**
     * Add an error to the collection.
     */
    void add_error(LayerError error) {
        // Update statistics
        error_count_by_severity_[static_cast<size_t>(error.severity)]++;
        error_count_by_layer_[static_cast<size_t>(error.source_layer)]++;
        
        // Store the error
        collected_errors_.push_back(std::move(error));
    }
    
    /**
     * Add multiple errors at once.
     */
    void add_errors(const std::vector<LayerError>& errors) {
        for (const auto& error : errors) {
            add_error(error);
        }
    }
    
    /**
     * Get all collected errors.
     */
    const std::vector<LayerError>& get_all_errors() const {
        return collected_errors_;
    }
    
    /**
     * Get errors by severity level.
     */
    std::vector<LayerError> get_errors_by_severity(ErrorSeverity severity) const {
        std::vector<LayerError> filtered;
        for (const auto& error : collected_errors_) {
            if (error.severity == severity) {
                filtered.push_back(error);
            }
        }
        return filtered;
    }
    
    /**
     * Get errors by source layer.
     */
    std::vector<LayerError> get_errors_by_layer(ErrorLayer layer) const {
        std::vector<LayerError> filtered;
        for (const auto& error : collected_errors_) {
            if (error.source_layer == layer) {
                filtered.push_back(error);
            }
        }
        return filtered;
    }
    
    /**
     * Get errors that reference a specific token.
     */
    std::vector<LayerError> get_errors_for_token(size_t token_index) const {
        std::vector<LayerError> filtered;
        for (const auto& error : collected_errors_) {
            for (size_t ref_token : error.related_token_indices) {
                if (ref_token == token_index) {
                    filtered.push_back(error);
                    break;
                }
            }
        }
        return filtered;
    }
    
    /**
     * Get errors that reference a specific scope.
     */
    std::vector<LayerError> get_errors_for_scope(size_t scope_index) const {
        std::vector<LayerError> filtered;
        for (const auto& error : collected_errors_) {
            if (error.related_scope && error.related_scope->scope_index == scope_index) {
                filtered.push_back(error);
            }
        }
        return filtered;
    }
    
    /**
     * Check if any blocking errors were collected.
     */
    bool has_blocking_errors() const {
        for (const auto& error : collected_errors_) {
            if (error.is_blocking()) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * Get count of errors by severity.
     */
    size_t get_error_count(ErrorSeverity severity) const {
        return error_count_by_severity_[static_cast<size_t>(severity)];
    }
    
    /**
     * Get count of errors by layer.
     */
    size_t get_layer_error_count(ErrorLayer layer) const {
        return error_count_by_layer_[static_cast<size_t>(layer)];
    }
    
    /**
     * Get total error count.
     */
    size_t get_total_error_count() const {
        return collected_errors_.size();
    }
    
    /**
     * Get source position for token referenced in error.
     */
    TokenizerState::SourcePosition get_token_position(size_t token_index) const {
        if (tokenizer_state_) {
            return tokenizer_state_->get_token_position(token_index);
        }
        return TokenizerState::SourcePosition(); // Invalid position
    }
    
    /**
     * Get source context for error location.
     */
    std::string get_source_context(const LayerError& error, size_t context_lines = 2) const {
        if (!tokenizer_state_ || error.related_token_indices.empty()) {
            return "Source context not available";
        }
        
        // Use the first token reference for context
        size_t token_index = error.related_token_indices[0];
        auto position = tokenizer_state_->get_token_position(token_index);
        
        return tokenizer_state_->get_source_context(position, context_lines);
    }
    
    /**
     * Clear all collected errors.
     */
    void clear() {
        collected_errors_.clear();
        std::fill(std::begin(error_count_by_severity_), std::end(error_count_by_severity_), 0);
        std::fill(std::begin(error_count_by_layer_), std::end(error_count_by_layer_), 0);
    }
    
    /**
     * Check if no errors have been collected.
     */
    bool empty() const {
        return collected_errors_.empty();
    }
    
    /**
     * Get compilation status based on collected errors.
     */
    bool compilation_should_succeed() const {
        return !has_blocking_errors();
    }
    
    /**
     * Apply a filter function to get custom error subsets.
     */
    std::vector<LayerError> filter_errors(std::function<bool(const LayerError&)> predicate) const {
        std::vector<LayerError> filtered;
        for (const auto& error : collected_errors_) {
            if (predicate(error)) {
                filtered.push_back(error);
            }
        }
        return filtered;
    }
    
    /**
     * Get summary statistics.
     */
    struct ErrorStatistics {
        size_t total_errors;
        size_t info_count;
        size_t warning_count;
        size_t error_count;
        size_t fatal_count;
        bool has_blocking_errors;
        
        std::string to_string() const {
            std::string result = "Error Summary: ";
            result += std::to_string(total_errors) + " total";
            if (info_count > 0) result += ", " + std::to_string(info_count) + " info";
            if (warning_count > 0) result += ", " + std::to_string(warning_count) + " warnings";
            if (error_count > 0) result += ", " + std::to_string(error_count) + " errors";
            if (fatal_count > 0) result += ", " + std::to_string(fatal_count) + " fatal";
            return result;
        }
    };
    
    ErrorStatistics get_statistics() const {
        return ErrorStatistics{
            collected_errors_.size(),
            get_error_count(ErrorSeverity::Info),
            get_error_count(ErrorSeverity::Warning),
            get_error_count(ErrorSeverity::Error),
            get_error_count(ErrorSeverity::Fatal),
            has_blocking_errors()
        };
    }
};

} // namespace cprime