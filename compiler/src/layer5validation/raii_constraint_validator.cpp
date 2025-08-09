#include "raii_constraint_validator.h"
#include <algorithm>

namespace cprime::layer4validation {

// ============================================================================
// DeferStatementAnalyzer Implementation - DEFER FUNCTIONALITY CORE  
// ============================================================================

DeferStatementAnalyzer::DeferStatementAnalyzer(const SymbolTable& symbol_table)
    : symbol_table_(symbol_table) {
}

validation::ValidationResult DeferStatementAnalyzer::analyze_defer_statements(std::shared_ptr<ast::CompilationUnit> ast) {
    validation::ValidationResult result;
    
    if (!ast) {
        result.add_error(
            "Cannot analyze defer statements: AST is null",
            validation::SourceLocation(),
            "Ensure AST is properly constructed before defer analysis"
        );
        return result;
    }
    
    // Analyze defer statements in all functions
    for (const auto& decl : ast->get_declarations()) {
        if (auto func_decl = std::dynamic_pointer_cast<ast::FunctionDecl>(decl)) {
            auto scope_analysis = analyze_function_scope(*func_decl);
            result.merge(validate_defer_patterns(scope_analysis));
        }
    }
    
    return result;
}

DeferStatementAnalyzer::ScopeAnalysis DeferStatementAnalyzer::analyze_function_scope(const ast::FunctionDecl& /* func_decl */) {
    ScopeAnalysis analysis;
    
    // TODO: Traverse function body AST to find:
    // 1. All defer statements
    // 2. All variable declarations  
    // 3. Control flow patterns (conditionals, loops)
    
    // For now, create a placeholder analysis
    // This will be expanded when AST visitor pattern is implemented
    
    return analysis;
}

validation::ValidationResult DeferStatementAnalyzer::validate_defer_patterns(const ScopeAnalysis& analysis) {
    validation::ValidationResult result;
    
    // Check each defer statement for unsupported patterns
    for (const auto& defer_info : analysis.defer_statements) {
        // Check for heap allocation defer (not supported yet)
        result.merge(check_heap_allocation_defer(defer_info));
    }
    
    // Check for complex conditional defer patterns  
    result.merge(check_complex_conditional_defer(analysis));
    
    return result;
}

validation::ValidationResult DeferStatementAnalyzer::check_heap_allocation_defer(const DeferInfo& defer_info) {
    validation::ValidationResult result;
    
    // Check if the deferred variable is heap-allocated
    if (is_heap_allocated_variable(defer_info.variable_name)) {
        result.add_error(
            "TODO: Heap object defer not implemented - heap allocation system needed first",
            ast_to_validation_location(defer_info.defer_location),
            "Use stack-allocated objects with defer for now, or implement heap allocation system"
        );
    }
    
    return result;
}

validation::ValidationResult DeferStatementAnalyzer::check_complex_conditional_defer(const ScopeAnalysis& analysis) {
    validation::ValidationResult result;
    
    // Check for complex conditional defer patterns
    if (has_complex_conditional_defer(analysis)) {
        result.add_error(
            "TODO: Conditional defer without assured return not implemented - will work under warning in future",
            validation::SourceLocation(),
            "Use defer in simple conditional blocks with assured returns, or at function scope"
        );
    }
    
    return result;
}

bool DeferStatementAnalyzer::is_heap_allocated_variable(const std::string& var_name) {
    // TODO: Check symbol table or AST to determine if variable is heap-allocated
    // For now, assume variables containing "Box", "Rc", "Arc" are heap-allocated
    return var_name.find("Box") != std::string::npos ||
           var_name.find("Rc") != std::string::npos ||
           var_name.find("Arc") != std::string::npos;
}

bool DeferStatementAnalyzer::has_complex_conditional_defer(const ScopeAnalysis& analysis) {
    // Check for defer statements in conditional scopes without assured returns
    for (const auto& defer_info : analysis.defer_statements) {
        if (defer_info.is_in_conditional_scope && !defer_info.conditional_has_assured_return) {
            return true;
        }
    }
    
    return false;
}

validation::SourceLocation DeferStatementAnalyzer::ast_to_validation_location(const ast::SourceLocation& loc) const {
    return validation::SourceLocation(loc.line, loc.column, loc.start_pos, loc.end_pos);
}

// ============================================================================
// RAIIConstraintValidator Implementation
// ============================================================================

RAIIConstraintValidator::RAIIConstraintValidator(std::shared_ptr<ast::CompilationUnit> ast, SymbolTable& symbol_table)
    : ast_(ast), symbol_table_(symbol_table) {
}

validation::ValidationResult RAIIConstraintValidator::validate() {
    validation::ValidationResult result;
    
    if (!ast_) {
        result.add_error(
            "Cannot validate RAII constraints: AST is null",
            validation::SourceLocation(),
            "Ensure AST is properly constructed before validation"
        );
        return result;
    }
    
    // Run core RAII constraint validation
    result.merge(validate_constructor_destructor_pairing());
    
    // Run defer statement validation
    result.merge(validate_defer_statements());
    
    return result;
}

validation::ValidationResult RAIIConstraintValidator::validate_constructor_destructor_pairing() {
    ConstructorDestructorChecker checker(symbol_table_);
    return checker.validate_pairing(ast_);
}

validation::ValidationResult RAIIConstraintValidator::validate_defer_statements() {
    // Create analyzer instance and run defer statement analysis
    DeferStatementAnalyzer analyzer(symbol_table_);
    return analyzer.analyze_defer_statements(ast_);
}

validation::ValidationResult RAIIConstraintValidator::validate_stack_object_defer_reordering() {
    validation::ValidationResult result;
    
    // This will be called by DeferStatementAnalyzer for specific stack object defer validation
    // For now, return success as detailed logic is in DeferStatementAnalyzer
    
    return result;
}

validation::ValidationResult RAIIConstraintValidator::detect_unsupported_defer_patterns() {
    validation::ValidationResult result;
    
    // This will be called by DeferStatementAnalyzer to detect unsupported patterns
    // For now, return success as detailed logic is in DeferStatementAnalyzer
    
    return result;
}

// ============================================================================
// ConstructorDestructorChecker Implementation - THE CORE RAII RULE ENFORCER
// ============================================================================

ConstructorDestructorChecker::ConstructorDestructorChecker(const SymbolTable& symbol_table)
    : symbol_table_(symbol_table) {
}

validation::ValidationResult ConstructorDestructorChecker::validate_pairing(std::shared_ptr<ast::CompilationUnit> ast) {
    validation::ValidationResult result;
    
    if (!ast) {
        result.add_error(
            "Cannot validate constructor/destructor pairing: AST is null",
            validation::SourceLocation(),
            "Ensure AST is properly constructed before validation"
        );
        return result;
    }
    
    // Analyze all classes for constructor/destructor patterns
    auto class_analyses = analyze_classes(ast);
    
    // Apply RAII pairing rules
    result.merge(apply_pairing_rules(class_analyses));
    
    return result;
}

std::vector<ConstructorDestructorChecker::ClassAnalysis> ConstructorDestructorChecker::analyze_classes(std::shared_ptr<ast::CompilationUnit> ast) {
    std::vector<ClassAnalysis> analyses;
    
    for (const auto& decl : ast->get_declarations()) {
        if (auto class_decl = std::dynamic_pointer_cast<ast::ClassDecl>(decl)) {
            analyses.push_back(analyze_single_class(*class_decl));
        }
    }
    
    return analyses;
}

ConstructorDestructorChecker::ClassAnalysis ConstructorDestructorChecker::analyze_single_class(const ast::ClassDecl& class_decl) {
    ClassAnalysis analysis(class_decl.get_name(), class_decl.get_location());
    
    for (const auto& member : class_decl.get_members()) {
        if (auto func_decl = std::dynamic_pointer_cast<ast::FunctionDecl>(member)) {
            std::string func_name = func_decl->get_name();
            
            // Check if this is a constructor
            if (func_name == class_decl.get_name()) {
                analysis.constructors.push_back(func_decl->get_location());
            }
            // Check if this is a destructor
            else if (func_name == "~" + class_decl.get_name()) {
                analysis.destructors.push_back(func_decl->get_location());
            }
        }
    }
    
    return analysis;
}

validation::ValidationResult ConstructorDestructorChecker::apply_pairing_rules(const std::vector<ClassAnalysis>& analyses) {
    validation::ValidationResult result;
    
    for (const auto& analysis : analyses) {
        result.merge(validate_constructor_destructor_rule(analysis));
    }
    
    return result;
}

validation::ValidationResult ConstructorDestructorChecker::validate_constructor_destructor_rule(const ClassAnalysis& analysis) {
    validation::ValidationResult result;
    
    bool has_constructors = analysis.has_any_constructor();
    bool has_destructors = analysis.has_any_destructor();
    
    // CORE RAII RULES:
    
    // Rule 1: If ANY constructor exists → destructor MUST exist
    if (has_constructors && !has_destructors) {
        std::string constructor_list;
        for (size_t i = 0; i < analysis.constructors.size(); ++i) {
            if (i > 0) constructor_list += ", ";
            constructor_list += "line " + std::to_string(analysis.constructors[i].line);
        }
        
        result.add_error(
            "Class '" + analysis.class_name + "' has constructor(s) but no destructor",
            ast_to_validation_location(analysis.class_location),
            "Add destructor: ~" + analysis.class_name + "() = default; (or custom implementation)"
        );
    }
    
    // Rule 2: If destructor exists → at least one constructor MUST exist
    if (has_destructors && !has_constructors) {
        result.add_error(
            "Class '" + analysis.class_name + "' has destructor but no constructors",
            ast_to_validation_location(analysis.destructors[0]),
            "Add constructor: " + analysis.class_name + "() = default; (or custom implementation)"
        );
    }
    
    // Rule 3: Classes with NO constructors AND NO destructors are allowed (plain data)
    if (analysis.is_plain_data_class()) {
        // This is explicitly allowed - no construction/destruction possible
        // Add informational note if desired
        result.add_info(
            "Class '" + analysis.class_name + "' is a plain data class (no constructors or destructors)",
            ast_to_validation_location(analysis.class_location),
            "This is allowed - objects of this type cannot be constructed or destructed"
        );
    }
    
    return result;
}

validation::SourceLocation ConstructorDestructorChecker::ast_to_validation_location(const ast::SourceLocation& loc) const {
    return validation::SourceLocation(loc.line, loc.column, loc.start_pos, loc.end_pos);
}

} // namespace cprime::layer4validation