#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>

namespace cprime {

/**
 * Semantic token types - unambiguous tokens with resolved meaning.
 * These are the output of Layer 2 (Semantic Token Translator).
 */
enum class SemanticTokenType {
    // Access Rights Variants (context-resolved)
    RuntimeAccessRightDeclaration,      // "runtime exposes UserOps { ... }"
    CompileTimeAccessRightDeclaration,  // "exposes UserOps { ... }"
    RuntimeTypeParameter,               // "Connection<runtime UserOps>"
    CompileTimeTypeParameter,           // "Connection<UserOps>"
    
    // Defer Variants (context-resolved)
    RaiiDefer,                         // "defer FileOps::destruct(&mut file)"
    CoroutineDefer,                    // "co_defer cleanup_resources()"
    
    // Union Variants (context-resolved)
    RuntimeUnion,                      // "union runtime ConnectionSpace { ... }"
    CompileTimeUnion,                  // "union Message { ... }"
    
    // Class Variants (context-resolved)
    DataClass,                         // "class Connection { ... }"
    FunctionalClass,                   // "functional class FileOps { ... }"
    DangerClass,                       // "danger class UnsafeWrapper { ... }"
    
    // Interface Definitions
    Interface,                         // "interface Drawable { ... }"
    
    // Function Definitions
    Function,                          // "fn process_data(...) -> Result"
    CoroutineFunction,                 // "async fn handle_request(...) -> Response"
    
    // Variable Declarations
    VariableDeclaration,               // "let x: i32 = 42;"
    RuntimeVariableDeclaration,        // "let conn: runtime Connection = ..."
    
    // Type System
    CustomType,                        // User-defined type reference
    BuiltinType,                       // int, bool, string, etc.
    PointerType,                       // *T
    ReferenceType,                     // &T
    ArrayType,                         // [T; N]
    GenericType,                       // Connection<T>
    
    // Expressions
    BinaryExpression,                  // a + b, a == b, etc.
    UnaryExpression,                   // !a, -a, etc.
    FunctionCall,                      // function_name(args)
    FieldAccess,                       // object.field
    MethodCall,                        // object.method(args)
    
    // Literals
    NumberLiteral,                     // 42, 3.14
    StringLiteral,                     // "hello world"
    BooleanLiteral,                    // true, false
    
    // Control Flow
    IfStatement,                       // if condition { ... }
    WhileLoop,                         // while condition { ... }
    ForLoop,                           // for item in collection { ... }
    MatchStatement,                    // match value { ... }
    
    // Memory Management
    MoveExpression,                    // move(value)
    CopyExpression,                    // copy(value)
    DropStatement,                     // drop(value)
    
    // Coroutine Specific
    AwaitExpression,                   // co_await async_call()
    YieldExpression,                   // co_yield value
    
    // Pass-through tokens (no semantic transformation needed)
    Identifier,                        // Regular identifiers
    Operator,                          // Operators that don't need context resolution
    Punctuation,                       // Punctuation marks
    Comment,                           // Comments (preserved for formatting)
    
    // Special
    Unknown,                           // Unknown or error token
    Placeholder,                       // Placeholder for unimplemented features
};

/**
 * Semantic token data - carries the resolved semantic meaning and associated data.
 */
struct SemanticToken {
    SemanticTokenType type;
    std::unordered_map<std::string, std::string> attributes;
    
    // Source location (preserved from raw token)
    size_t source_line;
    size_t source_column;
    size_t source_position;
    
    // Original raw token value (for debugging and error reporting)
    std::string raw_value;
    
    SemanticToken(SemanticTokenType type, size_t line = 0, size_t column = 0, size_t position = 0)
        : type(type), source_line(line), source_column(column), source_position(position) {}
    
    // Attribute helpers
    void set_attribute(const std::string& key, const std::string& value) {
        attributes[key] = value;
    }
    
    std::string get_attribute(const std::string& key, const std::string& default_value = "") const {
        auto it = attributes.find(key);
        return it != attributes.end() ? it->second : default_value;
    }
    
    bool has_attribute(const std::string& key) const {
        return attributes.find(key) != attributes.end();
    }
    
    bool get_bool_attribute(const std::string& key, bool default_value = false) const {
        auto value = get_attribute(key);
        if (value.empty()) return default_value;
        return value == "true";
    }
    
    // Convenience methods for common attributes
    std::string get_name() const { return get_attribute("name"); }
    std::string get_class_name() const { return get_attribute("class_name"); }
    std::string get_access_right() const { return get_attribute("access_right"); }
    std::string get_type_name() const { return get_attribute("type_name"); }
    std::vector<std::string> get_granted_fields() const;
    
    void set_name(const std::string& name) { set_attribute("name", name); }
    void set_class_name(const std::string& class_name) { set_attribute("class_name", class_name); }
    void set_access_right(const std::string& access_right) { set_attribute("access_right", access_right); }
    void set_type_name(const std::string& type_name) { set_attribute("type_name", type_name); }
    void set_granted_fields(const std::vector<std::string>& fields);
    
    // Debug representation
    std::string to_string() const;
    
    // Factory methods for common semantic tokens
    static SemanticToken runtime_access_right_declaration(
        const std::string& access_right,
        const std::vector<std::string>& granted_fields,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken compile_time_access_right_declaration(
        const std::string& access_right,
        const std::vector<std::string>& granted_fields,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken runtime_type_parameter(
        const std::string& type_name,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken compile_time_type_parameter(
        const std::string& type_name,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken raii_defer(
        const std::string& function_call,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken coroutine_defer(
        const std::string& cleanup_expression,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken runtime_union(
        const std::string& union_name,
        const std::vector<std::string>& variants,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken compile_time_union(
        const std::string& union_name,
        const std::vector<std::string>& variants,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken data_class(
        const std::string& class_name,
        const std::vector<std::string>& fields,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken functional_class(
        const std::string& class_name,
        const std::vector<std::string>& methods,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken identifier(
        const std::string& name,
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken literal(
        const std::string& value,
        const std::string& literal_type, // "number", "string", "boolean"
        size_t line = 0, size_t column = 0
    );
    
    static SemanticToken placeholder(
        const std::string& feature_name,
        const std::string& description,
        size_t line = 0, size_t column = 0
    );
};

/**
 * Semantic token stream for convenient iteration and processing.
 */
class SemanticTokenStream {
public:
    explicit SemanticTokenStream(std::vector<SemanticToken> tokens);
    
    // Navigation
    const SemanticToken& current() const;
    const SemanticToken& peek(size_t offset = 1) const;
    const SemanticToken& previous() const;
    void advance();
    bool is_at_end() const;
    
    // Position management
    size_t position() const { return pos; }
    void set_position(size_t new_pos);
    size_t size() const { return tokens.size(); }
    
    // Token access
    const std::vector<SemanticToken>& get_tokens() const { return tokens; }
    std::vector<SemanticToken>& get_tokens() { return tokens; }
    
    // Filtering and queries
    std::vector<SemanticToken> filter_by_type(SemanticTokenType type) const;
    std::vector<SemanticToken> filter_by_attribute(const std::string& key, const std::string& value) const;
    size_t count_by_type(SemanticTokenType type) const;
    
    // Debug output
    void dump_tokens() const;
    
private:
    std::vector<SemanticToken> tokens;
    size_t pos;
    
    void ensure_valid_position() const;
};

/**
 * Feature implementation status tracking for semantic tokens.
 * This enables incremental development with clear error messages.
 */
enum class ImplementationStatus {
    Implemented,           // Ready for code generation
    PartiallyImplemented, // Some functionality available
    Planned,              // Designed but not implemented
    Research,             // Under research/design
    Experimental,         // Experimental implementation
    Deprecated,           // Deprecated, use alternative
};

/**
 * Feature registry for tracking implementation status of semantic tokens.
 */
class SemanticFeatureRegistry {
public:
    SemanticFeatureRegistry();
    
    // Status queries
    ImplementationStatus get_status(SemanticTokenType type) const;
    bool is_implemented(SemanticTokenType type) const;
    bool is_experimental(SemanticTokenType type) const;
    
    // Feature information
    std::string get_description(SemanticTokenType type) const;
    std::string get_planned_version(SemanticTokenType type) const;
    std::string get_alternative(SemanticTokenType type) const;
    std::string get_github_issue(SemanticTokenType type) const;
    
    // Status updates (for development)
    void set_status(SemanticTokenType type, ImplementationStatus status);
    void set_description(SemanticTokenType type, const std::string& description);
    void set_planned_version(SemanticTokenType type, const std::string& version);
    
    // Development dashboard
    void generate_status_report() const;
    size_t count_by_status(ImplementationStatus status) const;
    std::vector<SemanticTokenType> get_tokens_by_status(ImplementationStatus status) const;
    
private:
    struct FeatureInfo {
        ImplementationStatus status;
        std::string description;
        std::string planned_version;
        std::string alternative;
        std::string github_issue;
        
        FeatureInfo(ImplementationStatus status = ImplementationStatus::Research)
            : status(status) {}
            
        FeatureInfo(ImplementationStatus status, const std::string& description)
            : status(status), description(description) {}
            
        FeatureInfo(ImplementationStatus status, const std::string& description, const std::string& planned_version)
            : status(status), description(description), planned_version(planned_version) {}
    };
    
    std::unordered_map<SemanticTokenType, FeatureInfo> features;
    
    void initialize_feature_registry();
    std::string status_to_string(ImplementationStatus status) const;
    std::string token_type_to_string(SemanticTokenType type) const;
};

} // namespace cprime