#pragma once

#include <vector>
#include <map>
#include <string>
#include "contextualizationError.h"
#include "enum/contextualizationError.h"

namespace cprime {

// Forward declarations
class StringTable;
struct RawToken;
struct Scope;

class ErrorHandler {
public:
    ErrorHandler();
    ~ErrorHandler() = default;

    // Configuration
    void set_severity_policy(ContextualizationErrorType error_type, ErrorSeverity severity);
    ErrorSeverity get_severity_policy(ContextualizationErrorType error_type) const;
    
    // Error registration during compilation
    void register_contextualization_error(const ContextualizationError& error);
    
    // Error resolution after layer completion
    void resolve_source_locations(const std::vector<Scope>& scopes,
                                 const std::map<std::string, std::vector<RawToken>>& streams,
                                 const StringTable& string_table);
    
    // Error reporting
    void report_errors() const;
    std::vector<ContextualizationError> get_errors() const;
    
    // Statistics
    uint32_t get_error_count() const;
    uint32_t get_warning_count() const;
    uint32_t get_suppressed_count() const;
    
    // Clear state
    void clear();

private:
    std::vector<ContextualizationError> errors_;
    std::map<ContextualizationErrorType, ErrorSeverity> severity_policies_;
    
    // Helper methods
    void initialize_default_policies();
    SourceLocation resolve_token_location(uint32_t token_index,
                                         const std::vector<Scope>& scopes,
                                         const std::map<std::string, std::vector<RawToken>>& streams,
                                         const StringTable& string_table) const;
    std::string format_error_message(const ContextualizationError& error) const;
    std::string get_source_context(const SourceLocation& location,
                                  const std::map<std::string, std::vector<RawToken>>& streams) const;
};

} // namespace cprime