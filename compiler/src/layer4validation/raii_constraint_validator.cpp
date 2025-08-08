#include "raii_constraint_validator.h"
#include <algorithm>

namespace cprime::layer4validation {

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
    
    return result;
}

validation::ValidationResult RAIIConstraintValidator::validate_constructor_destructor_pairing() {
    ConstructorDestructorChecker checker(symbol_table_);
    return checker.validate_pairing(ast_);
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