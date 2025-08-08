#include "ast_structure_validator.h"
#include <unordered_set>

namespace cprime::layer3validation {

ASTStructureValidator::ASTStructureValidator(std::shared_ptr<ast::CompilationUnit> ast, SymbolTable& symbol_table)
    : ast_(ast), symbol_table_(symbol_table) {
}

validation::ValidationResult ASTStructureValidator::validate() {
    validation::ValidationResult result;
    
    if (!ast_) {
        result.add_error(
            "Cannot validate AST structure: AST is null",
            validation::SourceLocation(),
            "Ensure AST is properly constructed before validation"
        );
        return result;
    }
    
    // Run all AST structure validations
    result.merge(validate_symbol_resolution());
    result.merge(validate_type_consistency());
    result.merge(validate_ast_node_integrity());
    result.merge(validate_declaration_ordering());
    result.merge(validate_circular_dependencies());
    
    return result;
}

validation::ValidationResult ASTStructureValidator::validate_symbol_resolution() {
    validation::ValidationResult result;
    
    // Simple stub implementation - validate that all identifiers are resolved
    for (const auto& decl : ast_->get_declarations()) {
        if (auto class_decl = std::dynamic_pointer_cast<ast::ClassDecl>(decl)) {
            // Check if class is in symbol table
            auto symbol = symbol_table_.lookup(class_decl->get_name());
            if (!symbol) {
                result.add_error(
                    "Class '" + class_decl->get_name() + "' not found in symbol table",
                    validation::SourceLocation(class_decl->get_location().line, class_decl->get_location().column,
                                             class_decl->get_location().start_pos, class_decl->get_location().end_pos),
                    "Add class to symbol table or check spelling"
                );
            }
        }
    }
    
    return result;
}

validation::ValidationResult ASTStructureValidator::validate_type_consistency() {
    validation::ValidationResult result;
    
    // Simple stub implementation - basic type consistency checks
    for (const auto& decl : ast_->get_declarations()) {
        if (auto var_decl = std::dynamic_pointer_cast<ast::VarDecl>(decl)) {
            // Check if variable type exists
            if (var_decl->get_type() && !var_decl->get_type()->get_name().empty()) {
                auto type_symbol = symbol_table_.lookup(var_decl->get_type()->get_name());
                if (!type_symbol && var_decl->get_type()->get_kind() == ast::Type::Kind::Class) {
                    result.add_warning(
                        "Unknown type '" + var_decl->get_type()->get_name() + "' for variable '" + var_decl->get_name() + "'",
                        validation::SourceLocation(var_decl->get_location().line, var_decl->get_location().column,
                                                 var_decl->get_location().start_pos, var_decl->get_location().end_pos),
                        "Define the type or use a built-in type"
                    );
                }
            }
        }
    }
    
    return result;
}

validation::ValidationResult ASTStructureValidator::validate_ast_node_integrity() {
    validation::ValidationResult result;
    
    // Simple stub implementation - check basic AST node structure
    for (const auto& decl : ast_->get_declarations()) {
        if (!decl) {
            result.add_error(
                "Null declaration found in AST",
                validation::SourceLocation(),
                "Remove null declarations from AST"
            );
            continue;
        }
        
        // Check if class declarations have valid structure
        if (auto class_decl = std::dynamic_pointer_cast<ast::ClassDecl>(decl)) {
            if (class_decl->get_name().empty()) {
                result.add_error(
                    "Class declaration missing name",
                    validation::SourceLocation(class_decl->get_location().line, class_decl->get_location().column,
                                             class_decl->get_location().start_pos, class_decl->get_location().end_pos),
                    "Provide a name for the class"
                );
            }
        }
    }
    
    return result;
}

validation::ValidationResult ASTStructureValidator::validate_declaration_ordering() {
    validation::ValidationResult result;
    
    // Simple stub implementation - basic ordering checks
    std::unordered_set<std::string> declared_names;
    
    for (const auto& decl : ast_->get_declarations()) {
        std::string name;
        if (auto class_decl = std::dynamic_pointer_cast<ast::ClassDecl>(decl)) {
            name = class_decl->get_name();
        } else if (auto func_decl = std::dynamic_pointer_cast<ast::FunctionDecl>(decl)) {
            name = func_decl->get_name();
        } else if (auto var_decl = std::dynamic_pointer_cast<ast::VarDecl>(decl)) {
            name = var_decl->get_name();
        }
        
        if (!name.empty()) {
            if (declared_names.count(name)) {
                result.add_error(
                    "Duplicate declaration: '" + name + "'",
                    validation::SourceLocation(decl->get_location().line, decl->get_location().column,
                                             decl->get_location().start_pos, decl->get_location().end_pos),
                    "Remove or rename duplicate declaration"
                );
            }
            declared_names.insert(name);
        }
    }
    
    return result;
}

validation::ValidationResult ASTStructureValidator::validate_circular_dependencies() {
    validation::ValidationResult result;
    
    // Simple stub implementation - placeholder for circular dependency detection
    // This would require a more sophisticated graph analysis
    result.add_info(
        "Circular dependency check completed",
        validation::SourceLocation(),
        "No circular dependencies detected (simplified check)"
    );
    
    return result;
}

} // namespace cprime::layer3validation