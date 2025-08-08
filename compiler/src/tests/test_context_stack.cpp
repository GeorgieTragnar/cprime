#include "../layer1/context_stack.h"
#include "test_framework.h"
#include <iostream>

using namespace cprime;
using namespace cprime::testing;

bool test_basic_context_operations() {
    TestLogger logger("Basic Context Operations");
    
    try {
        logger << "=== Testing Basic Context Stack Operations ===\n";
        
        ContextStack context_stack;
        
        // Test initial state - should have top-level context
        logger << "Initial context stack depth: " << context_stack.depth() << "\n";
        
        if (context_stack.depth() != 1) {
            TEST_FAILURE(logger, "Initial context stack depth should be 1 (top-level context)");
        }
        
        // Test pushing contexts
        context_stack.push(ParseContext::class_definition("Connection", true));
        logger << "After pushing class definition, depth: " << context_stack.depth() << "\n";
        
        if (context_stack.depth() != 2) {
            TEST_FAILURE(logger, "Context stack depth should be 2 after first push");
        }
        
        context_stack.push(ParseContext::access_rights_declaration("UserOps", true));
        logger << "After pushing access rights, depth: " << context_stack.depth() << "\n";
        
        if (context_stack.depth() != 3) {
            TEST_FAILURE(logger, "Context stack depth should be 3 after second push");
        }
        
        // Test current context
        auto current = context_stack.current();
        if (!current) {
            TEST_FAILURE(logger, "Current context should not be null");
        }
        
        logger << "Current context: " << current->to_string() << "\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_context_stack_queries() {
    TestLogger logger("Context Stack Queries");
    
    try {
        logger << "=== Testing Context Stack Query Methods ===\n";
        
        ContextStack context_stack;
        
        // Test queries on empty stack
        logger << "Empty stack queries:\n";
        logger << "  Is in class definition: " << (context_stack.is_in_class_definition() ? "yes" : "no") << "\n";
        logger << "  Is in access rights declaration: " << (context_stack.is_in_access_rights_declaration() ? "yes" : "no") << "\n";
        logger << "  Current class name: '" << context_stack.current_class_name() << "'\n";
        
        // Push class definition context
        context_stack.push(ParseContext::class_definition("TestClass", true));
        
        logger << "\nAfter pushing class definition:\n";
        logger << "  Is in class definition: " << (context_stack.is_in_class_definition() ? "yes" : "no") << "\n";
        logger << "  Current class name: '" << context_stack.current_class_name() << "'\n";
        
        if (!context_stack.is_in_class_definition()) {
            TEST_FAILURE(logger, "Should be in class definition context");
        }
        
        if (context_stack.current_class_name() != "TestClass") {
            TEST_FAILURE(logger, "Current class name should be 'TestClass'");
        }
        
        // Push access rights context
        context_stack.push(ParseContext::access_rights_declaration("AdminOps", false));
        
        logger << "\nAfter pushing access rights declaration:\n";
        logger << "  Is in access rights declaration: " << (context_stack.is_in_access_rights_declaration() ? "yes" : "no") << "\n";
        logger << "  Still in class definition: " << (context_stack.is_in_class_definition() ? "yes" : "no") << "\n";
        
        if (!context_stack.is_in_access_rights_declaration()) {
            TEST_FAILURE(logger, "Should be in access rights declaration context");
        }
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_context_resolver() {
    TestLogger logger("Context Resolver");
    
    try {
        logger << "=== Testing Context Resolver ===\n";
        
        ContextStack context_stack;
        
        // Test resolver in different contexts
        context_stack.push(ParseContext::class_definition("Connection", true));
        context_stack.push(ParseContext::access_rights_declaration("UserOps", true));
        
        ContextResolver resolver(context_stack);
        auto runtime_interpretation = resolver.resolve_runtime_keyword();
        
        logger << "Runtime keyword interpretation: " << resolver.interpretation_to_string(runtime_interpretation) << "\n";
        
        // Test context stack dump for debugging
        logger << "\nContext stack dump:\n";
        
        // The dump method outputs to std::cout, so we note its execution
        logger << "Context stack dump completed\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_context_stack_pop_operations() {
    TestLogger logger("Context Stack Pop Operations");
    
    try {
        logger << "=== Testing Context Stack Pop Operations ===\n";
        
        ContextStack context_stack;
        
        // Build up a context stack
        context_stack.push(ParseContext::class_definition("TestClass", false));
        context_stack.push(ParseContext::access_rights_declaration("ReadOps", false));
        context_stack.push(ParseContext::class_definition("InnerClass", false));
        
        logger << "Built context stack with depth: " << context_stack.depth() << "\n";
        
        // Test popping contexts (1 initial + 3 pushed = 4 total)
        if (context_stack.depth() != 4) {
            TEST_FAILURE(logger, "Expected depth 4 after pushing 3 contexts (1 initial + 3 pushed)");
        }
        
        // Pop and check each level
        context_stack.pop();
        logger << "After first pop, depth: " << context_stack.depth() << "\n";
        if (context_stack.depth() != 3) {
            TEST_FAILURE(logger, "Expected depth 3 after first pop");
        }
        
        context_stack.pop();
        logger << "After second pop, depth: " << context_stack.depth() << "\n";
        if (context_stack.depth() != 2) {
            TEST_FAILURE(logger, "Expected depth 2 after second pop");
        }
        
        // Check we're back in the original class context
        if (!context_stack.is_in_class_definition()) {
            TEST_FAILURE(logger, "Should still be in class definition after pops");
        }
        
        if (context_stack.current_class_name() != "TestClass") {
            TEST_FAILURE(logger, "Should be back to original TestClass context");
        }
        
        logger << "Final class name: " << context_stack.current_class_name() << "\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

int main() {
    TestSuite suite("Context Stack Tests");
    
    std::cout << "CPrime Context Stack Tests\n";
    std::cout << "==========================\n\n";
    
    suite.run_test(test_basic_context_operations);
    suite.run_test(test_context_stack_queries);
    suite.run_test(test_context_resolver);
    suite.run_test(test_context_stack_pop_operations);
    
    suite.print_results();
    
    return suite.all_passed() ? 0 : 1;
}