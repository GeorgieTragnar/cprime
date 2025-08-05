#include "semantic_token.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace cprime {

// Helper function to join strings
std::string join_strings(const std::vector<std::string>& strings, const std::string& delimiter) {
    if (strings.empty()) return "";
    
    std::stringstream ss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) ss << delimiter;
        ss << strings[i];
    }
    return ss.str();
}

// Helper function to split strings
std::vector<std::string> split_string(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> result;
    if (str.empty()) return result;
    
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    
    result.push_back(str.substr(start));
    return result;
}

// SemanticToken implementation
std::vector<std::string> SemanticToken::get_granted_fields() const {
    std::string fields_str = get_attribute("granted_fields");
    if (fields_str.empty()) return {};
    return split_string(fields_str, ",");
}

void SemanticToken::set_granted_fields(const std::vector<std::string>& fields) {
    set_attribute("granted_fields", join_strings(fields, ","));
}

std::string SemanticToken::to_string() const {
    std::stringstream ss;
    ss << "SemanticToken(";
    
    // Token type
    switch (type) {
        case SemanticTokenType::RuntimeAccessRightDeclaration: ss << "RuntimeAccessRightDeclaration"; break;
        case SemanticTokenType::CompileTimeAccessRightDeclaration: ss << "CompileTimeAccessRightDeclaration"; break;
        case SemanticTokenType::RuntimeTypeParameter: ss << "RuntimeTypeParameter"; break;
        case SemanticTokenType::CompileTimeTypeParameter: ss << "CompileTimeTypeParameter"; break;
        case SemanticTokenType::RaiiDefer: ss << "RaiiDefer"; break;
        case SemanticTokenType::CoroutineDefer: ss << "CoroutineDefer"; break;
        case SemanticTokenType::RuntimeUnion: ss << "RuntimeUnion"; break;
        case SemanticTokenType::CompileTimeUnion: ss << "CompileTimeUnion"; break;
        case SemanticTokenType::DataClass: ss << "DataClass"; break;
        case SemanticTokenType::FunctionalClass: ss << "FunctionalClass"; break;
        case SemanticTokenType::DangerClass: ss << "DangerClass"; break;
        case SemanticTokenType::Interface: ss << "Interface"; break;
        case SemanticTokenType::Function: ss << "Function"; break;
        case SemanticTokenType::CoroutineFunction: ss << "CoroutineFunction"; break;
        case SemanticTokenType::VariableDeclaration: ss << "VariableDeclaration"; break;
        case SemanticTokenType::RuntimeVariableDeclaration: ss << "RuntimeVariableDeclaration"; break;
        case SemanticTokenType::CustomType: ss << "CustomType"; break;
        case SemanticTokenType::BuiltinType: ss << "BuiltinType"; break;
        case SemanticTokenType::PointerType: ss << "PointerType"; break;
        case SemanticTokenType::ReferenceType: ss << "ReferenceType"; break;
        case SemanticTokenType::ArrayType: ss << "ArrayType"; break;
        case SemanticTokenType::GenericType: ss << "GenericType"; break;
        case SemanticTokenType::BinaryExpression: ss << "BinaryExpression"; break;
        case SemanticTokenType::UnaryExpression: ss << "UnaryExpression"; break;
        case SemanticTokenType::FunctionCall: ss << "FunctionCall"; break;
        case SemanticTokenType::FieldAccess: ss << "FieldAccess"; break;
        case SemanticTokenType::MethodCall: ss << "MethodCall"; break;
        case SemanticTokenType::NumberLiteral: ss << "NumberLiteral"; break;
        case SemanticTokenType::StringLiteral: ss << "StringLiteral"; break;
        case SemanticTokenType::BooleanLiteral: ss << "BooleanLiteral"; break;
        case SemanticTokenType::IfStatement: ss << "IfStatement"; break;
        case SemanticTokenType::WhileLoop: ss << "WhileLoop"; break;
        case SemanticTokenType::ForLoop: ss << "ForLoop"; break;
        case SemanticTokenType::MatchStatement: ss << "MatchStatement"; break;
        case SemanticTokenType::MoveExpression: ss << "MoveExpression"; break;
        case SemanticTokenType::CopyExpression: ss << "CopyExpression"; break;
        case SemanticTokenType::DropStatement: ss << "DropStatement"; break;
        case SemanticTokenType::AwaitExpression: ss << "AwaitExpression"; break;
        case SemanticTokenType::YieldExpression: ss << "YieldExpression"; break;
        case SemanticTokenType::Identifier: ss << "Identifier"; break;
        case SemanticTokenType::Operator: ss << "Operator"; break;
        case SemanticTokenType::Punctuation: ss << "Punctuation"; break;
        case SemanticTokenType::Comment: ss << "Comment"; break;
        case SemanticTokenType::Unknown: ss << "Unknown"; break;
        case SemanticTokenType::Placeholder: ss << "Placeholder"; break;
    }
    
    // Attributes
    if (!attributes.empty()) {
        ss << ", attributes={";
        bool first = true;
        for (const auto& [key, value] : attributes) {
            if (!first) ss << ", ";
            ss << key << "=" << value;
            first = false;
        }
        ss << "}";
    }
    
    // Source location
    ss << ", " << source_line << ":" << source_column;
    
    if (!raw_value.empty()) {
        ss << ", raw=\"" << raw_value << "\"";
    }
    
    ss << ")";
    return ss.str();
}

// Factory methods
SemanticToken SemanticToken::runtime_access_right_declaration(
    const std::string& access_right,
    const std::vector<std::string>& granted_fields,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::RuntimeAccessRightDeclaration, line, column);
    token.set_access_right(access_right);
    token.set_granted_fields(granted_fields);
    return token;
}

SemanticToken SemanticToken::compile_time_access_right_declaration(
    const std::string& access_right,
    const std::vector<std::string>& granted_fields,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::CompileTimeAccessRightDeclaration, line, column);
    token.set_access_right(access_right);
    token.set_granted_fields(granted_fields);
    return token;
}

SemanticToken SemanticToken::runtime_type_parameter(
    const std::string& type_name,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::RuntimeTypeParameter, line, column);
    token.set_type_name(type_name);
    return token;
}

SemanticToken SemanticToken::compile_time_type_parameter(
    const std::string& type_name,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::CompileTimeTypeParameter, line, column);
    token.set_type_name(type_name);
    return token;
}

SemanticToken SemanticToken::raii_defer(
    const std::string& function_call,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::RaiiDefer, line, column);
    token.set_attribute("function_call", function_call);
    return token;
}

SemanticToken SemanticToken::coroutine_defer(
    const std::string& cleanup_expression,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::CoroutineDefer, line, column);
    token.set_attribute("cleanup_expression", cleanup_expression);
    return token;
}

SemanticToken SemanticToken::runtime_union(
    const std::string& union_name,
    const std::vector<std::string>& variants,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::RuntimeUnion, line, column);
    token.set_name(union_name);
    token.set_attribute("variants", join_strings(variants, ","));
    return token;
}

SemanticToken SemanticToken::compile_time_union(
    const std::string& union_name,
    const std::vector<std::string>& variants,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::CompileTimeUnion, line, column);
    token.set_name(union_name);
    token.set_attribute("variants", join_strings(variants, ","));
    return token;
}

SemanticToken SemanticToken::data_class(
    const std::string& class_name,
    const std::vector<std::string>& fields,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::DataClass, line, column);
    token.set_class_name(class_name);
    token.set_attribute("fields", join_strings(fields, ","));
    return token;
}

SemanticToken SemanticToken::functional_class(
    const std::string& class_name,
    const std::vector<std::string>& methods,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::FunctionalClass, line, column);
    token.set_class_name(class_name);
    token.set_attribute("methods", join_strings(methods, ","));
    return token;
}

SemanticToken SemanticToken::identifier(
    const std::string& name,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::Identifier, line, column);
    token.set_name(name);
    token.raw_value = name;
    return token;
}

SemanticToken SemanticToken::literal(
    const std::string& value,
    const std::string& literal_type,
    size_t line, size_t column
) {
    SemanticTokenType type;
    if (literal_type == "number") {
        type = SemanticTokenType::NumberLiteral;
    } else if (literal_type == "string") {
        type = SemanticTokenType::StringLiteral;
    } else if (literal_type == "boolean") {
        type = SemanticTokenType::BooleanLiteral;
    } else {
        type = SemanticTokenType::Unknown;
    }
    
    SemanticToken token(type, line, column);
    token.set_attribute("value", value);
    token.set_attribute("literal_type", literal_type);
    token.raw_value = value;
    return token;
}

SemanticToken SemanticToken::placeholder(
    const std::string& feature_name,
    const std::string& description,
    size_t line, size_t column
) {
    SemanticToken token(SemanticTokenType::Placeholder, line, column);
    token.set_attribute("feature_name", feature_name);
    token.set_attribute("description", description);
    return token;
}

// SemanticTokenStream implementation
SemanticTokenStream::SemanticTokenStream(std::vector<SemanticToken> tokens)
    : tokens(std::move(tokens)), pos(0) {}

const SemanticToken& SemanticTokenStream::current() const {
    ensure_valid_position();
    return tokens[pos];
}

const SemanticToken& SemanticTokenStream::peek(size_t offset) const {
    size_t peek_pos = pos + offset;
    if (peek_pos >= tokens.size()) {
        static const SemanticToken unknown_token(SemanticTokenType::Unknown);
        return unknown_token;
    }
    return tokens[peek_pos];
}

const SemanticToken& SemanticTokenStream::previous() const {
    if (pos == 0) {
        throw std::runtime_error("Cannot access previous token at beginning of stream");
    }
    return tokens[pos - 1];
}

void SemanticTokenStream::advance() {
    if (pos < tokens.size()) {
        pos++;
    }
}

bool SemanticTokenStream::is_at_end() const {
    return pos >= tokens.size();
}

void SemanticTokenStream::set_position(size_t new_pos) {
    if (new_pos > tokens.size()) {
        throw std::runtime_error("Invalid semantic token stream position");
    }
    pos = new_pos;
}

std::vector<SemanticToken> SemanticTokenStream::filter_by_type(SemanticTokenType type) const {
    std::vector<SemanticToken> result;
    std::copy_if(tokens.begin(), tokens.end(), std::back_inserter(result),
                 [type](const SemanticToken& token) { return token.type == type; });
    return result;
}

std::vector<SemanticToken> SemanticTokenStream::filter_by_attribute(const std::string& key, const std::string& value) const {
    std::vector<SemanticToken> result;
    std::copy_if(tokens.begin(), tokens.end(), std::back_inserter(result),
                 [&key, &value](const SemanticToken& token) { 
                     return token.has_attribute(key) && token.get_attribute(key) == value; 
                 });
    return result;
}

size_t SemanticTokenStream::count_by_type(SemanticTokenType type) const {
    return std::count_if(tokens.begin(), tokens.end(),
                        [type](const SemanticToken& token) { return token.type == type; });
}

void SemanticTokenStream::dump_tokens() const {
    std::cout << "Semantic Token Stream (" << tokens.size() << " tokens):\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "  [" << i << "] " << tokens[i].to_string() << "\n";
    }
}

void SemanticTokenStream::ensure_valid_position() const {
    if (pos >= tokens.size()) {
        throw std::runtime_error("Semantic token stream position out of bounds");
    }
}

// SemanticFeatureRegistry implementation
SemanticFeatureRegistry::SemanticFeatureRegistry() {
    initialize_feature_registry();
}

ImplementationStatus SemanticFeatureRegistry::get_status(SemanticTokenType type) const {
    auto it = features.find(type);
    return it != features.end() ? it->second.status : ImplementationStatus::Research;
}

bool SemanticFeatureRegistry::is_implemented(SemanticTokenType type) const {
    return get_status(type) == ImplementationStatus::Implemented;
}

bool SemanticFeatureRegistry::is_experimental(SemanticTokenType type) const {
    return get_status(type) == ImplementationStatus::Experimental;
}

std::string SemanticFeatureRegistry::get_description(SemanticTokenType type) const {
    auto it = features.find(type);
    return it != features.end() ? it->second.description : "No description available";
}

std::string SemanticFeatureRegistry::get_planned_version(SemanticTokenType type) const {
    auto it = features.find(type);
    return it != features.end() ? it->second.planned_version : "Unknown";
}

std::string SemanticFeatureRegistry::get_alternative(SemanticTokenType type) const {
    auto it = features.find(type);
    return it != features.end() ? it->second.alternative : "";
}

std::string SemanticFeatureRegistry::get_github_issue(SemanticTokenType type) const {
    auto it = features.find(type);
    return it != features.end() ? it->second.github_issue : "";
}

void SemanticFeatureRegistry::set_status(SemanticTokenType type, ImplementationStatus status) {
    features[type].status = status;
}

void SemanticFeatureRegistry::set_description(SemanticTokenType type, const std::string& description) {
    features[type].description = description;
}

void SemanticFeatureRegistry::set_planned_version(SemanticTokenType type, const std::string& version) {
    features[type].planned_version = version;
}

void SemanticFeatureRegistry::generate_status_report() const {
    std::cout << "CPrime Compiler V2 - Semantic Token Implementation Status\n";
    std::cout << "========================================================\n\n";
    
    for (auto status : {ImplementationStatus::Implemented, 
                       ImplementationStatus::PartiallyImplemented,
                       ImplementationStatus::Experimental,
                       ImplementationStatus::Planned,
                       ImplementationStatus::Research,
                       ImplementationStatus::Deprecated}) {
        
        auto tokens = get_tokens_by_status(status);
        if (!tokens.empty()) {
            std::cout << status_to_string(status) << " (" << tokens.size() << " features):\n";
            for (auto token_type : tokens) {
                std::cout << "  - " << token_type_to_string(token_type);
                if (get_status(token_type) == ImplementationStatus::Planned) {
                    std::cout << " (planned for " << get_planned_version(token_type) << ")";
                }
                std::cout << "\n";
            }
            std::cout << "\n";
        }
    }
    
    size_t total_features = features.size();
    size_t implemented = count_by_status(ImplementationStatus::Implemented);
    double completion_percentage = total_features > 0 ? (double)implemented / total_features * 100.0 : 0.0;
    
    std::cout << "Overall Progress: " << implemented << "/" << total_features 
              << " (" << completion_percentage << "% complete)\n";
}

size_t SemanticFeatureRegistry::count_by_status(ImplementationStatus status) const {
    return std::count_if(features.begin(), features.end(),
                        [status](const auto& pair) { return pair.second.status == status; });
}

std::vector<SemanticTokenType> SemanticFeatureRegistry::get_tokens_by_status(ImplementationStatus status) const {
    std::vector<SemanticTokenType> result;
    for (const auto& [token_type, feature_info] : features) {
        if (feature_info.status == status) {
            result.push_back(token_type);
        }
    }
    return result;
}

void SemanticFeatureRegistry::initialize_feature_registry() {
    // Initialize with current implementation status
    
    // Implemented features (basic functionality)
    features[SemanticTokenType::Identifier] = {ImplementationStatus::Implemented, "Basic identifier tokens"};
    features[SemanticTokenType::NumberLiteral] = {ImplementationStatus::Implemented, "Number literal tokens"};
    features[SemanticTokenType::StringLiteral] = {ImplementationStatus::Implemented, "String literal tokens"};
    features[SemanticTokenType::BooleanLiteral] = {ImplementationStatus::Implemented, "Boolean literal tokens"};
    features[SemanticTokenType::Operator] = {ImplementationStatus::Implemented, "Operator tokens"};
    features[SemanticTokenType::Punctuation] = {ImplementationStatus::Implemented, "Punctuation tokens"};
    features[SemanticTokenType::Comment] = {ImplementationStatus::Implemented, "Comment tokens"};
    
    // Planned features (core language constructs)
    features[SemanticTokenType::RuntimeAccessRightDeclaration] = {ImplementationStatus::Planned, "Runtime access rights with vtables", "0.2.0"};
    features[SemanticTokenType::CompileTimeAccessRightDeclaration] = {ImplementationStatus::Planned, "Compile-time access rights", "0.1.0"};
    features[SemanticTokenType::RuntimeTypeParameter] = {ImplementationStatus::Planned, "Runtime type parameters", "0.2.0"};
    features[SemanticTokenType::CompileTimeTypeParameter] = {ImplementationStatus::Planned, "Compile-time type parameters", "0.1.0"};
    features[SemanticTokenType::RaiiDefer] = {ImplementationStatus::Planned, "RAII defer statements", "0.1.0"};
    features[SemanticTokenType::DataClass] = {ImplementationStatus::Planned, "Data class definitions", "0.1.0"};
    features[SemanticTokenType::FunctionalClass] = {ImplementationStatus::Planned, "Functional class definitions", "0.1.0"};
    
    // Research features (advanced constructs)
    features[SemanticTokenType::CoroutineDefer] = {ImplementationStatus::Research, "Coroutine-specific defer statements"};
    features[SemanticTokenType::RuntimeUnion] = {ImplementationStatus::Research, "Runtime unions with vtable dispatch"};
    features[SemanticTokenType::CompileTimeUnion] = {ImplementationStatus::Research, "Compile-time unions with pattern matching"};
    features[SemanticTokenType::CoroutineFunction] = {ImplementationStatus::Research, "Async coroutine functions"};
    features[SemanticTokenType::AwaitExpression] = {ImplementationStatus::Research, "Coroutine await expressions"};
    features[SemanticTokenType::YieldExpression] = {ImplementationStatus::Research, "Coroutine yield expressions"};
    
    // Experimental features
    features[SemanticTokenType::DangerClass] = {ImplementationStatus::Experimental, "Danger classes for unsafe operations"};
    features[SemanticTokenType::Interface] = {ImplementationStatus::Experimental, "Interface definitions"};
    
    // Complex features still under design
    features[SemanticTokenType::MatchStatement] = {ImplementationStatus::Research, "Pattern matching statements"};
    features[SemanticTokenType::GenericType] = {ImplementationStatus::Research, "Generic type system"};
    features[SemanticTokenType::MoveExpression] = {ImplementationStatus::Research, "Move semantics"};
    features[SemanticTokenType::CopyExpression] = {ImplementationStatus::Research, "Copy semantics"};
}

std::string SemanticFeatureRegistry::status_to_string(ImplementationStatus status) const {
    switch (status) {
        case ImplementationStatus::Implemented: return "Implemented";
        case ImplementationStatus::PartiallyImplemented: return "Partially Implemented";
        case ImplementationStatus::Planned: return "Planned";
        case ImplementationStatus::Research: return "Research";
        case ImplementationStatus::Experimental: return "Experimental";
        case ImplementationStatus::Deprecated: return "Deprecated";
    }
    return "Unknown";
}

std::string SemanticFeatureRegistry::token_type_to_string(SemanticTokenType type) const {
    // Convert enum to string - simplified version
    switch (type) {
        case SemanticTokenType::RuntimeAccessRightDeclaration: return "RuntimeAccessRightDeclaration";
        case SemanticTokenType::CompileTimeAccessRightDeclaration: return "CompileTimeAccessRightDeclaration";
        case SemanticTokenType::RuntimeTypeParameter: return "RuntimeTypeParameter";
        case SemanticTokenType::CompileTimeTypeParameter: return "CompileTimeTypeParameter";
        case SemanticTokenType::RaiiDefer: return "RaiiDefer";
        case SemanticTokenType::CoroutineDefer: return "CoroutineDefer";
        case SemanticTokenType::RuntimeUnion: return "RuntimeUnion";
        case SemanticTokenType::CompileTimeUnion: return "CompileTimeUnion";
        case SemanticTokenType::DataClass: return "DataClass";
        case SemanticTokenType::FunctionalClass: return "FunctionalClass";
        case SemanticTokenType::DangerClass: return "DangerClass";
        case SemanticTokenType::Interface: return "Interface";
        case SemanticTokenType::Function: return "Function";
        case SemanticTokenType::CoroutineFunction: return "CoroutineFunction";
        case SemanticTokenType::Identifier: return "Identifier";
        case SemanticTokenType::NumberLiteral: return "NumberLiteral";
        case SemanticTokenType::StringLiteral: return "StringLiteral";
        case SemanticTokenType::BooleanLiteral: return "BooleanLiteral";
        case SemanticTokenType::Operator: return "Operator";
        case SemanticTokenType::Punctuation: return "Punctuation";
        case SemanticTokenType::Comment: return "Comment";
        default: return "Unknown";
    }
}

} // namespace cprime