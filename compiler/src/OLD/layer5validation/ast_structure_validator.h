#pragma once

#include "../validation_common.h"
#include "../layer3/ast.h"
#include "../layer3/symbol_table.h"
#include <memory>

namespace cprime::layer3validation {

/**
 * AST structure validator for Layer 3.
 * Validates that the AST structure is well-formed and symbols are properly resolved.
 * 
 * Responsibilities:
 * - Validate symbol resolution throughout the AST
 * - Check type consistency in expressions and declarations
 * - Ensure AST node integrity and proper tree structure
 * - Verify declaration ordering and dependencies
 * - Detect circular dependencies between types/modules
 */
class ASTStructureValidator : public validation::BaseValidator {
public:
    explicit ASTStructureValidator(std::shared_ptr<ast::CompilationUnit> ast, SymbolTable& symbol_table);
    
    // BaseValidator interface
    validation::ValidationResult validate() override;
    std::string get_validator_name() const override { return "ASTStructureValidator"; }
    
    // Specific validation methods
    validation::ValidationResult validate_symbol_resolution();
    validation::ValidationResult validate_type_consistency();
    validation::ValidationResult validate_ast_node_integrity();
    validation::ValidationResult validate_declaration_ordering();
    validation::ValidationResult validate_circular_dependencies();
    
private:
    std::shared_ptr<ast::CompilationUnit> ast_;
    SymbolTable& symbol_table_;
};

} // namespace cprime::layer3validation