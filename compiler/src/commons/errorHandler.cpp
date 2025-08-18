#include "errorHandler.h"
#include "logger.h"
#include "dirty/string_table.h"
#include "token.h"
#include "rawToken.h"
#include "scope.h"
#include <iostream>
#include <sstream>

namespace cprime {

ErrorHandler::ErrorHandler() {
    initialize_default_policies();
}

void ErrorHandler::initialize_default_policies() {
    // Default severity policies for development mode
    severity_policies_[ContextualizationErrorType::UNSUPPORTED_TOKEN_PATTERN] = ErrorSeverity::ERROR;
    severity_policies_[ContextualizationErrorType::AMBIGUOUS_OPERATOR_CONTEXT] = ErrorSeverity::WARNING;
    severity_policies_[ContextualizationErrorType::UNRESOLVED_IDENTIFIER] = ErrorSeverity::ERROR;
    severity_policies_[ContextualizationErrorType::INVALID_EXPRESSION_STRUCTURE] = ErrorSeverity::ERROR;
    severity_policies_[ContextualizationErrorType::MISSING_TYPE_INFORMATION] = ErrorSeverity::WARNING;
    severity_policies_[ContextualizationErrorType::INCOMPLETE_STATEMENT] = ErrorSeverity::ERROR;
    severity_policies_[ContextualizationErrorType::INVALID_FUNCTION_CALL] = ErrorSeverity::ERROR;
    severity_policies_[ContextualizationErrorType::TYPE_MISMATCH] = ErrorSeverity::ERROR;
    severity_policies_[ContextualizationErrorType::UNDECLARED_VARIABLE] = ErrorSeverity::ERROR;
}

void ErrorHandler::set_severity_policy(ContextualizationErrorType error_type, ErrorSeverity severity) {
    severity_policies_[error_type] = severity;
}

ErrorSeverity ErrorHandler::get_severity_policy(ContextualizationErrorType error_type) const {
    auto it = severity_policies_.find(error_type);
    if (it != severity_policies_.end()) {
        return it->second;
    }
    return ErrorSeverity::ERROR; // Default to error for unknown types
}

void ErrorHandler::register_contextualization_error(const ContextualizationError& error) {
    auto logger = LoggerFactory::get_logger("errorHandler");
    
    ErrorSeverity severity = get_severity_policy(error.error_type);
    
    // Only store errors that are not suppressed
    if (severity != ErrorSeverity::SUPPRESS) {
        errors_.push_back(error);
        LOG_DEBUG("Registered contextualization error: type={}, scope={}, instruction={}, tokens={}", 
                 static_cast<uint32_t>(error.error_type), 
                 error.scope_index, 
                 error.instruction_index,
                 error.token_indices.size());
    } else {
        LOG_DEBUG("Suppressed contextualization error: type={}", static_cast<uint32_t>(error.error_type));
    }
}

void ErrorHandler::resolve_source_locations(const std::vector<Scope>& scopes,
                                           const std::map<std::string, std::vector<RawToken>>& streams,
                                           const StringTable& string_table) {
    auto logger = LoggerFactory::get_logger("errorHandler");
    LOG_INFO("Resolving source locations for {} errors", errors_.size());
    
    for (auto& error : errors_) {
        if (!error.token_indices.empty()) {
            // Use the first problematic token for location
            uint32_t primary_token_index = error.token_indices[0];
            error.source_location = resolve_token_location(primary_token_index, scopes, streams, string_table);
            
            LOG_DEBUG("Resolved error location: line={}, column={}, offset={}", 
                     error.source_location.line,
                     error.source_location.column,
                     error.source_location.offset);
        }
    }
}

SourceLocation ErrorHandler::resolve_token_location(uint32_t token_index,
                                                   const std::vector<Scope>& scopes,
                                                   const std::map<std::string, std::vector<RawToken>>& streams,
                                                   const StringTable& string_table) const {
    SourceLocation location;
    
    // Find the token in the raw token streams
    for (const auto& [stream_name, raw_tokens] : streams) {
        if (token_index < raw_tokens.size()) {
            const auto& raw_token = raw_tokens[token_index];
            
            location.file_name = stream_name;
            location.line = raw_token._line;
            location.column = raw_token._column;
            location.offset = raw_token._position;
            
            return location;
        }
    }
    
    // Fallback if token not found
    location.file_name = "unknown";
    location.line = 0;
    location.column = 0;
    location.offset = 0;
    
    return location;
}

void ErrorHandler::report_errors() const {
    auto logger = LoggerFactory::get_logger("errorHandler");
    
    if (errors_.empty()) {
        LOG_INFO("No contextualization errors to report");
        return;
    }
    
    LOG_INFO("Reporting {} contextualization errors", errors_.size());
    
    for (const auto& error : errors_) {
        ErrorSeverity severity = get_severity_policy(error.error_type);
        std::string severity_str = (severity == ErrorSeverity::ERROR) ? "ERROR" : "WARNING";
        
        std::string message = format_error_message(error);
        
        if (severity == ErrorSeverity::ERROR) {
            LOG_ERROR("{}", message);
        } else {
            LOG_WARN("{}", message);
        }
        
        // Also output to console for immediate visibility
        std::cout << "[" << severity_str << "] " << message << std::endl;
    }
}

std::string ErrorHandler::format_error_message(const ContextualizationError& error) const {
    std::ostringstream oss;
    
    // Basic error information
    oss << error.source_location.file_name 
        << ":" << error.source_location.line 
        << ":" << error.source_location.column;
    
    // Error type description
    switch (error.error_type) {
        case ContextualizationErrorType::UNSUPPORTED_TOKEN_PATTERN:
            oss << " Unsupported token pattern";
            break;
        case ContextualizationErrorType::AMBIGUOUS_OPERATOR_CONTEXT:
            oss << " Ambiguous operator context";
            break;
        case ContextualizationErrorType::UNRESOLVED_IDENTIFIER:
            oss << " Unresolved identifier";
            break;
        case ContextualizationErrorType::INVALID_EXPRESSION_STRUCTURE:
            oss << " Invalid expression structure";
            break;
        case ContextualizationErrorType::MISSING_TYPE_INFORMATION:
            oss << " Missing type information";
            break;
        case ContextualizationErrorType::INCOMPLETE_STATEMENT:
            oss << " Incomplete statement";
            break;
        case ContextualizationErrorType::INVALID_FUNCTION_CALL:
            oss << " Invalid function call";
            break;
        case ContextualizationErrorType::TYPE_MISMATCH:
            oss << " Type mismatch";
            break;
        case ContextualizationErrorType::UNDECLARED_VARIABLE:
            oss << " Undeclared variable";
            break;
        default:
            oss << " Unknown contextualization error";
            break;
    }
    
    // Add extra information if provided
    if (!error.extra_info.empty()) {
        oss << ": " << error.extra_info;
    }
    
    // Add instruction context
    std::string instruction_type_str;
    switch (error.instruction_type) {
        case InstructionType::HEADER:
            instruction_type_str = "header";
            break;
        case InstructionType::BODY:
            instruction_type_str = "body";
            break;
        case InstructionType::FOOTER:
            instruction_type_str = "footer";
            break;
    }
    
    oss << " (in " << instruction_type_str << " instruction at scope " << error.scope_index << ")";
    
    return oss.str();
}

std::string ErrorHandler::get_source_context(const SourceLocation& location,
                                            const std::map<std::string, std::vector<RawToken>>& streams) const {
    // TODO: Implement source context extraction for rich error reporting
    // This would show the problematic line with highlighting
    return "";
}

std::vector<ContextualizationError> ErrorHandler::get_errors() const {
    return errors_;
}

uint32_t ErrorHandler::get_error_count() const {
    uint32_t count = 0;
    for (const auto& error : errors_) {
        if (get_severity_policy(error.error_type) == ErrorSeverity::ERROR) {
            count++;
        }
    }
    return count;
}

uint32_t ErrorHandler::get_warning_count() const {
    uint32_t count = 0;
    for (const auto& error : errors_) {
        if (get_severity_policy(error.error_type) == ErrorSeverity::WARNING) {
            count++;
        }
    }
    return count;
}

uint32_t ErrorHandler::get_suppressed_count() const {
    // Suppressed errors are not stored, so we can't count them easily
    // This would require tracking during registration
    return 0;
}

void ErrorHandler::clear() {
    errors_.clear();
}

} // namespace cprime