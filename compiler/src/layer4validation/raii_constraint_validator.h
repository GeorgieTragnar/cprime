#pragma once

#include "../validation_common.h"
#include "../layer3/ast.h"
#include "../layer3/symbol_table.h"
#include <memory>

namespace cprime::layer4validation {

/**
 * RAII constraint validator for Layer 4.
 * Validates that classes follow CPrime's RAII rules for constructor/destructor pairing.
 * 
 * CPrime RAII Rules:
 * 1. If ANY constructor exists → destructor MUST exist
 * 2. If destructor exists → at least one constructor MUST exist  
 * 3. Classes with NO constructors AND NO destructors are allowed (no construction/destruction)
 * 
 * This is the CORE implementation that enforces CPrime's RAII guarantees.
 */
class RAIIConstraintValidator : public validation::BaseValidator {
public:
    explicit RAIIConstraintValidator(std::shared_ptr<ast::CompilationUnit> ast, SymbolTable& symbol_table);
    
    // BaseValidator interface
    validation::ValidationResult validate() override;
    std::string get_validator_name() const override { return "RAIIConstraintValidator"; }
    
    // Core RAII validation method
    validation::ValidationResult validate_constructor_destructor_pairing();
    
private:
    std::shared_ptr<ast::CompilationUnit> ast_;
    SymbolTable& symbol_table_;
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

} // namespace cprime::layer4validation