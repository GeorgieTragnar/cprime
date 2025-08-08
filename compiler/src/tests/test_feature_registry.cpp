#include "../layer2/semantic_token.h"
#include "test_framework.h"
#include <iostream>

using namespace cprime;
using namespace cprime::testing;

bool test_feature_registry_queries() {
    TestLogger logger("Feature Registry Queries");
    
    try {
        logger << "=== Testing Feature Registry Queries ===\n";
        
        SemanticFeatureRegistry registry;
        
        // Test implementation status queries for key features
        logger << "Testing implementation status queries:\n";
        
        bool runtime_access_implemented = registry.is_implemented(SemanticTokenType::RuntimeAccessRightDeclaration);
        logger << "RuntimeAccessRightDeclaration status: " 
               << (runtime_access_implemented ? "Implemented" : "Not implemented") << "\n";
        
        bool identifier_implemented = registry.is_implemented(SemanticTokenType::Identifier);
        logger << "Identifier status: " 
               << (identifier_implemented ? "Implemented" : "Not implemented") << "\n";
        
        bool data_class_implemented = registry.is_implemented(SemanticTokenType::DataClass);
        logger << "DataClass status: " 
               << (data_class_implemented ? "Implemented" : "Not implemented") << "\n";
        
        bool functional_class_implemented = registry.is_implemented(SemanticTokenType::FunctionalClass);
        logger << "FunctionalClass status: " 
               << (functional_class_implemented ? "Implemented" : "Not implemented") << "\n";
        
        // Test some defer-related features
        bool raii_defer_implemented = registry.is_implemented(SemanticTokenType::RaiiDefer);
        logger << "RaiiDefer status: " 
               << (raii_defer_implemented ? "Implemented" : "Not implemented") << "\n";
        
        bool coroutine_defer_implemented = registry.is_implemented(SemanticTokenType::CoroutineDefer);
        logger << "CoroutineDefer status: " 
               << (coroutine_defer_implemented ? "Implemented" : "Not implemented") << "\n";
        
        // Test union features
        bool runtime_union_implemented = registry.is_implemented(SemanticTokenType::RuntimeUnion);
        logger << "RuntimeUnion status: " 
               << (runtime_union_implemented ? "Implemented" : "Not implemented") << "\n";
        
        bool compile_time_union_implemented = registry.is_implemented(SemanticTokenType::CompileTimeUnion);
        logger << "CompileTimeUnion status: " 
               << (compile_time_union_implemented ? "Implemented" : "Not implemented") << "\n";
        
        logger << "Feature registry queries completed successfully\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_feature_registry_status_report() {
    TestLogger logger("Feature Registry Status Report");
    
    try {
        logger << "=== Testing Feature Registry Status Report ===\n";
        
        SemanticFeatureRegistry registry;
        
        logger << "Generating comprehensive status report:\n";
        logger << "======================================\n";
        
        // The generate_status_report method outputs to std::cout
        // We'll capture that it runs without error
        logger << "Executing registry.generate_status_report()...\n";
        
        // Note: This outputs directly to std::cout, not our logger
        // But we can verify it doesn't crash
        registry.generate_status_report();
        
        logger << "Status report generation completed successfully\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

bool test_feature_coverage_analysis() {
    TestLogger logger("Feature Coverage Analysis");
    
    try {
        logger << "=== Testing Feature Coverage Analysis ===\n";
        
        SemanticFeatureRegistry registry;
        
        // Test coverage for different categories of features
        logger << "Analyzing feature implementation coverage:\n";
        
        // Core language features
        std::vector<SemanticTokenType> core_features = {
            SemanticTokenType::Identifier,
            SemanticTokenType::DataClass,
            SemanticTokenType::FunctionalClass,
            SemanticTokenType::CustomType,
        };
        
        int core_implemented = 0;
        for (auto feature : core_features) {
            if (registry.is_implemented(feature)) {
                core_implemented++;
            }
        }
        logger << "Core features implemented: " << core_implemented << "/" << core_features.size() << "\n";
        
        // Access rights features
        std::vector<SemanticTokenType> access_features = {
            SemanticTokenType::RuntimeAccessRightDeclaration,
            SemanticTokenType::CompileTimeAccessRightDeclaration,
        };
        
        int access_implemented = 0;
        for (auto feature : access_features) {
            if (registry.is_implemented(feature)) {
                access_implemented++;
            }
        }
        logger << "Access rights features implemented: " << access_implemented << "/" << access_features.size() << "\n";
        
        // RAII/Defer features
        std::vector<SemanticTokenType> raii_features = {
            SemanticTokenType::RaiiDefer,
            SemanticTokenType::CoroutineDefer,
        };
        
        int raii_implemented = 0;
        for (auto feature : raii_features) {
            if (registry.is_implemented(feature)) {
                raii_implemented++;
            }
        }
        logger << "RAII/Defer features implemented: " << raii_implemented << "/" << raii_features.size() << "\n";
        
        // Union features
        std::vector<SemanticTokenType> union_features = {
            SemanticTokenType::RuntimeUnion,
            SemanticTokenType::CompileTimeUnion,
        };
        
        int union_implemented = 0;
        for (auto feature : union_features) {
            if (registry.is_implemented(feature)) {
                union_implemented++;
            }
        }
        logger << "Union features implemented: " << union_implemented << "/" << union_features.size() << "\n";
        
        // Calculate overall coverage
        int total_tested = core_features.size() + access_features.size() + raii_features.size() + union_features.size();
        int total_implemented = core_implemented + access_implemented + raii_implemented + union_implemented;
        
        logger << "\nOverall feature coverage: " << total_implemented << "/" << total_tested;
        if (total_tested > 0) {
            double percentage = (double)total_implemented / total_tested * 100.0;
            logger << " (" << (int)percentage << "%)";
        }
        logger << "\n";
        
        TEST_SUCCESS(logger);
        
    } catch (const std::exception& e) {
        logger.test_exception(e);
        return false;
    }
}

int main() {
    TestSuite suite("Feature Registry Tests");
    
    std::cout << "CPrime Feature Registry Tests\n";
    std::cout << "=============================\n\n";
    
    suite.run_test(test_feature_registry_queries);
    suite.run_test(test_feature_registry_status_report);
    suite.run_test(test_feature_coverage_analysis);
    
    suite.print_results();
    
    return suite.all_passed() ? 0 : 1;
}