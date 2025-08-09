#pragma once

#include "../validation_common.h"
#include "../layer3/ast.h"
#include "../layer3/symbol_table.h"
#include <memory>
#include <vector>

namespace cprime::layer4validation {

/**
 * RAII and Defer Expansion Validator for Layer 4.
 * This is the comprehensive implicit expansion layer that handles:
 * 1. RAII constructor/destructor pairing validation
 * 2. Defer statement validation and LIFO reordering  
 * 3. Resource lifecycle management with proper cleanup ordering
 * 4. Stack object destructor reordering via defer statements
 * 
 * CPrime RAII Rules:
 * 1. If ANY constructor exists → destructor MUST exist
 * 2. If destructor exists → at least one constructor MUST exist  
 * 3. Classes with NO constructors AND NO destructors are allowed (no construction/destruction)
 * 
 * CPrime Defer Rules:
 * 1. Stack object defer: Reorders destructor call to front of cleanup queue (LIFO bump-to-front)
 * 2. Simple conditional defer: Supported with assured return paths
 * 3. Heap object defer: TODO - not implemented (heap allocation needed first)
 * 4. Complex conditional defer: TODO - exponential complexity, error for now
 */
class RAIIConstraintValidator : public validation::BaseValidator {
public:
    explicit RAIIConstraintValidator(std::shared_ptr<ast::CompilationUnit> ast, SymbolTable& symbol_table);
    
    // BaseValidator interface
    validation::ValidationResult validate() override;
    std::string get_validator_name() const override { return "RAIIAndDeferValidator"; }
    
    // Core RAII validation method
    validation::ValidationResult validate_constructor_destructor_pairing();
    
    // Core Defer validation methods
    validation::ValidationResult validate_defer_statements();
    validation::ValidationResult validate_stack_object_defer_reordering();
    validation::ValidationResult detect_unsupported_defer_patterns();
    
private:
    std::shared_ptr<ast::CompilationUnit> ast_;
    SymbolTable& symbol_table_;
    
    // Defer statement analysis helpers
};

/**
 * Constructor/Destructor Pairing Checker - THE CORE RAII RULE ENFORCER
 * This class implements the exact RAII rules requested by the user.
 */
class ConstructorDestructorChecker {
public:
    explicit ConstructorDestructorChecker(const SymbolTable& symbol_table);
    
    validation::ValidationResult validate_pairing(std::shared_ptr<ast::CompilationUnit> ast);
    
private:
    const SymbolTable& symbol_table_;
    
    struct ClassAnalysis {
        std::string class_name;
        ast::SourceLocation class_location;
        std::vector<ast::SourceLocation> constructors;
        std::vector<ast::SourceLocation> destructors;
        
        ClassAnalysis(const std::string& name, const ast::SourceLocation& location)
            : class_name(name), class_location(location) {}
        
        bool has_any_constructor() const { return !constructors.empty(); }
        bool has_any_destructor() const { return !destructors.empty(); }
        bool is_plain_data_class() const { return constructors.empty() && destructors.empty(); }
    };
    
    std::vector<ClassAnalysis> analyze_classes(std::shared_ptr<ast::CompilationUnit> ast);
    ClassAnalysis analyze_single_class(const ast::ClassDecl& class_decl);
    validation::ValidationResult apply_pairing_rules(const std::vector<ClassAnalysis>& analyses);
    validation::ValidationResult validate_constructor_destructor_rule(const ClassAnalysis& analysis);
    
    validation::SourceLocation ast_to_validation_location(const ast::SourceLocation& loc) const;
};

/**
 * Defer Statement Analyzer - DEFER FUNCTIONALITY CORE
 * Handles defer statement validation, LIFO reordering, and cleanup sequence generation.
 * 
 * Key functionality:
 * 1. Stack object defer reordering (bump-to-front LIFO)
 * 2. Simple conditional defer with assured returns
 * 3. Detection of unsupported patterns (heap allocation, complex conditionals)
 */
class DeferStatementAnalyzer {
public:
    explicit DeferStatementAnalyzer(const SymbolTable& symbol_table);
    
    validation::ValidationResult analyze_defer_statements(std::shared_ptr<ast::CompilationUnit> ast);
    
private:
    const SymbolTable& symbol_table_;
    
    struct DeferInfo {
        std::string variable_name;
        ast::SourceLocation defer_location;
        ast::SourceLocation variable_location;
        bool is_in_conditional_scope;
        bool conditional_has_assured_return;
        
        DeferInfo(const std::string& name, const ast::SourceLocation& defer_loc, 
                  const ast::SourceLocation& var_loc)
            : variable_name(name), defer_location(defer_loc), variable_location(var_loc),
              is_in_conditional_scope(false), conditional_has_assured_return(false) {}
    };
    
    struct ScopeAnalysis {
        std::vector<DeferInfo> defer_statements;
        std::vector<std::string> stack_variables; 
        bool has_complex_control_flow;
        
        ScopeAnalysis() : has_complex_control_flow(false) {}
    };
    
    // Analysis methods
    ScopeAnalysis analyze_function_scope(const ast::FunctionDecl& func_decl);
    validation::ValidationResult validate_defer_patterns(const ScopeAnalysis& analysis);
    validation::ValidationResult check_heap_allocation_defer(const DeferInfo& defer_info);
    validation::ValidationResult check_complex_conditional_defer(const ScopeAnalysis& analysis);
    
    // Helper methods
    bool is_heap_allocated_variable(const std::string& var_name);
    bool has_complex_conditional_defer(const ScopeAnalysis& analysis);
    validation::SourceLocation ast_to_validation_location(const ast::SourceLocation& loc) const;
};

} // namespace cprime::layer4validation