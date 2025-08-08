#pragma once

#include <string>
#include <vector>
#include <optional>
#include <algorithm>

namespace cprime::validation {

/**
 * Validation error severity levels.
 */
enum class Severity {
    Error,      // Must fix - compilation cannot continue
    Warning,    // Should fix - code will compile but may have issues
    Info        // Informational - style or optimization suggestions
};

/**
 * Source location for validation errors.
 */
struct SourceLocation {
    size_t line;
    size_t column;
    size_t start_pos;
    size_t end_pos;
    
    SourceLocation(size_t line = 0, size_t column = 0, size_t start = 0, size_t end = 0)
        : line(line), column(column), start_pos(start), end_pos(end) {}
        
    std::string to_string() const {
        return "line " + std::to_string(line) + ", column " + std::to_string(column);
    }
};

/**
 * Individual validation diagnostic.
 */
struct ValidationDiagnostic {
    Severity severity;
    std::string message;
    SourceLocation location;
    std::optional<std::string> suggestion;
    
    ValidationDiagnostic(Severity sev, const std::string& msg, const SourceLocation& loc)
        : severity(sev), message(msg), location(loc) {}
        
    ValidationDiagnostic(Severity sev, const std::string& msg, const SourceLocation& loc, 
                        const std::string& suggestion)
        : severity(sev), message(msg), location(loc), suggestion(suggestion) {}
    
    std::string severity_string() const {
        switch (severity) {
            case Severity::Error: return "error";
            case Severity::Warning: return "warning";
            case Severity::Info: return "info";
        }
        return "unknown";
    }
    
    std::string to_string() const {
        std::string result = severity_string() + " at " + location.to_string() + ": " + message;
        if (suggestion.has_value()) {
            result += "\n  suggestion: " + *suggestion;
        }
        return result;
    }
};

/**
 * Validation result containing all diagnostics from a validation layer.
 */
class ValidationResult {
public:
    ValidationResult() : has_errors_(false) {}
    
    // Add diagnostics
    void add_error(const std::string& message, const SourceLocation& location, 
                  const std::optional<std::string>& suggestion = std::nullopt) {
        diagnostics_.emplace_back(Severity::Error, message, location, suggestion.value_or(""));
        has_errors_ = true;
    }
    
    void add_warning(const std::string& message, const SourceLocation& location,
                    const std::optional<std::string>& suggestion = std::nullopt) {
        diagnostics_.emplace_back(Severity::Warning, message, location, suggestion.value_or(""));
    }
    
    void add_info(const std::string& message, const SourceLocation& location,
                 const std::optional<std::string>& suggestion = std::nullopt) {
        diagnostics_.emplace_back(Severity::Info, message, location, suggestion.value_or(""));
    }
    
    void add_diagnostic(const ValidationDiagnostic& diagnostic) {
        if (diagnostic.severity == Severity::Error) {
            has_errors_ = true;
        }
        diagnostics_.push_back(diagnostic);
    }
    
    // Query results
    bool success() const { return !has_errors_; }
    bool has_errors() const { return has_errors_; }
    bool has_warnings() const {
        return std::any_of(diagnostics_.begin(), diagnostics_.end(),
                          [](const auto& d) { return d.severity == Severity::Warning; });
    }
    
    size_t error_count() const {
        return std::count_if(diagnostics_.begin(), diagnostics_.end(),
                            [](const auto& d) { return d.severity == Severity::Error; });
    }
    
    size_t warning_count() const {
        return std::count_if(diagnostics_.begin(), diagnostics_.end(),
                            [](const auto& d) { return d.severity == Severity::Warning; });
    }
    
    const std::vector<ValidationDiagnostic>& get_diagnostics() const { return diagnostics_; }
    
    // Get diagnostics by severity
    std::vector<ValidationDiagnostic> get_errors() const {
        std::vector<ValidationDiagnostic> errors;
        std::copy_if(diagnostics_.begin(), diagnostics_.end(), std::back_inserter(errors),
                    [](const auto& d) { return d.severity == Severity::Error; });
        return errors;
    }
    
    std::vector<ValidationDiagnostic> get_warnings() const {
        std::vector<ValidationDiagnostic> warnings;
        std::copy_if(diagnostics_.begin(), diagnostics_.end(), std::back_inserter(warnings),
                    [](const auto& d) { return d.severity == Severity::Warning; });
        return warnings;
    }
    
    // Merge another result into this one
    void merge(const ValidationResult& other) {
        diagnostics_.insert(diagnostics_.end(), other.diagnostics_.begin(), other.diagnostics_.end());
        if (other.has_errors_) {
            has_errors_ = true;
        }
    }
    
    // Debug output
    std::string to_string() const {
        if (diagnostics_.empty()) {
            return "Validation passed with no issues.";
        }
        
        std::string result = "Validation results:\n";
        for (const auto& diagnostic : diagnostics_) {
            result += "  " + diagnostic.to_string() + "\n";
        }
        return result;
    }
    
private:
    std::vector<ValidationDiagnostic> diagnostics_;
    bool has_errors_;
};

/**
 * Base class for all validation layers.
 * Provides common functionality and enforces consistent interface.
 */
class BaseValidator {
public:
    virtual ~BaseValidator() = default;
    
    // Each validator must implement validate method
    virtual ValidationResult validate() = 0;
    
    // Optional: Get validator name for debugging
    virtual std::string get_validator_name() const = 0;
    
protected:
    // Helper to create source locations from token positions
    SourceLocation create_location(size_t line, size_t column, size_t start_pos, size_t end_pos) const {
        return SourceLocation(line, column, start_pos, end_pos);
    }
    
    // Helper to create error diagnostics
    ValidationDiagnostic create_error(const std::string& message, const SourceLocation& location,
                                     const std::string& suggestion = "") const {
        if (suggestion.empty()) {
            return ValidationDiagnostic(Severity::Error, message, location);
        }
        return ValidationDiagnostic(Severity::Error, message, location, suggestion);
    }
    
    ValidationDiagnostic create_warning(const std::string& message, const SourceLocation& location,
                                       const std::string& suggestion = "") const {
        if (suggestion.empty()) {
            return ValidationDiagnostic(Severity::Warning, message, location);
        }
        return ValidationDiagnostic(Severity::Warning, message, location, suggestion);
    }
};

} // namespace cprime::validation