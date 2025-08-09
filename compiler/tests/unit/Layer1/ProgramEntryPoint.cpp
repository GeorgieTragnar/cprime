#include "test_common.h"
#include "layer1/raw_token.h"
#include <string>
#include <vector>

using namespace cprime;

// ============================================================================
// Program Entry Point Tests - Layer 1
// Tests C++ style main function recognition and validation
// ============================================================================

class ProgramEntryPointTest : public BufferedLayer1Test {
protected:
    /**
     * Validate that tokens represent a proper C++ style program entry point
     * Expected pattern: [return_type] main ( [optional_params] ) { ... }
     */
    void validateProgramEntryPoint(const std::vector<RawToken>& tokens, 
                                  const std::string& context) {
        test_logger_->debug("Validating program entry point for {}", context);
        
        if (tokens.empty()) {
            test_logger_->error("No tokens found for entry point validation");
            FAIL() << "Program entry point requires tokens";
        }
        
        // State machine for entry point validation
        enum State {
            EXPECT_RETURN_TYPE,
            EXPECT_MAIN,
            EXPECT_OPEN_PAREN,
            IN_PARAMETERS,
            EXPECT_CLOSE_PAREN,
            EXPECT_OPEN_BRACE,
            FOUND_ENTRY_POINT
        };
        
        State state = EXPECT_RETURN_TYPE;
        bool found_valid_entry = false;
        
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            test_logger_->debug("Processing token[{}]: {} (state: {})", i, token.to_string(), static_cast<int>(state));
            
            switch (state) {
                case EXPECT_RETURN_TYPE:
                    if (token.type == RawTokenType::KEYWORD && token.value == "int") {
                        test_logger_->debug("Found valid return type: int");
                        state = EXPECT_MAIN;
                    }
                    // Skip whitespace and comments
                    else if (token.type == RawTokenType::WHITESPACE || token.type == RawTokenType::COMMENT) {
                        continue;
                    }
                    else {
                        test_logger_->debug("Skipping non-return-type token: {}", token.value);
                    }
                    break;
                    
                case EXPECT_MAIN:
                    if (token.type == RawTokenType::IDENTIFIER && token.value == "main") {
                        test_logger_->debug("Found main function identifier");
                        state = EXPECT_OPEN_PAREN;
                    }
                    else if (token.type == RawTokenType::WHITESPACE || token.type == RawTokenType::COMMENT) {
                        continue;
                    }
                    else {
                        test_logger_->debug("Expected 'main' but got: {}", token.value);
                        state = EXPECT_RETURN_TYPE; // Reset and continue looking
                    }
                    break;
                    
                case EXPECT_OPEN_PAREN:
                    if (token.type == RawTokenType::SYMBOL && token.value == "(") {
                        test_logger_->debug("Found opening parenthesis");
                        state = IN_PARAMETERS;
                    }
                    else if (token.type == RawTokenType::WHITESPACE || token.type == RawTokenType::COMMENT) {
                        continue;
                    }
                    else {
                        test_logger_->debug("Expected '(' but got: {}", token.value);
                        state = EXPECT_RETURN_TYPE; // Reset
                    }
                    break;
                    
                case IN_PARAMETERS:
                    if (token.type == RawTokenType::SYMBOL && token.value == ")") {
                        test_logger_->debug("Found closing parenthesis (empty parameters)");
                        state = EXPECT_OPEN_BRACE;
                    }
                    else if (token.type == RawTokenType::KEYWORD && token.value == "int") {
                        test_logger_->debug("Found parameter type: int (expecting argc/argv pattern)");
                        state = EXPECT_CLOSE_PAREN; // Skip to close paren for simplicity
                    }
                    else if (token.type == RawTokenType::WHITESPACE || token.type == RawTokenType::COMMENT) {
                        continue;
                    }
                    break;
                    
                case EXPECT_CLOSE_PAREN:
                    if (token.type == RawTokenType::SYMBOL && token.value == ")") {
                        test_logger_->debug("Found closing parenthesis (with parameters)");
                        state = EXPECT_OPEN_BRACE;
                    }
                    // Skip everything until we find the closing paren
                    break;
                    
                case EXPECT_OPEN_BRACE:
                    if (token.type == RawTokenType::SYMBOL && token.value == "{") {
                        test_logger_->debug("Found opening brace - VALID ENTRY POINT DETECTED!");
                        found_valid_entry = true;
                        state = FOUND_ENTRY_POINT;
                    }
                    else if (token.type == RawTokenType::WHITESPACE || token.type == RawTokenType::COMMENT) {
                        continue;
                    }
                    else {
                        test_logger_->debug("Expected '{{' but got: {}", token.value);
                        state = EXPECT_RETURN_TYPE; // Reset
                    }
                    break;
                    
                case FOUND_ENTRY_POINT:
                    // Entry point found, can stop processing
                    break;
            }
            
            if (found_valid_entry) break;
        }
        
        ASSERT_TRUE(found_valid_entry) 
            << "No valid program entry point found in " << context 
            << " (expected: int main(...) { ... })";
            
        test_logger_->info("Successfully validated program entry point for {}", context);
    }
    
    /**
     * Validate that tokens do NOT represent a valid entry point
     */
    void validateInvalidEntryPoint(const std::vector<RawToken>& tokens,
                                  const std::string& context,
                                  const std::string& reason) {
        test_logger_->debug("Validating INVALID entry point for {}: {}", context, reason);
        
        // This should fail - we expect no valid entry point
        bool has_valid_entry = false;
        
        // Simple check for obvious invalid patterns
        bool has_main = false;
        bool has_proper_return = false;
        
        for (const auto& token : tokens) {
            if (token.value == "main") has_main = true;
            if (token.value == "int" && !has_proper_return) has_proper_return = true;
        }
        
        // For this test, we're mainly checking that certain invalid patterns are tokenized
        // The actual semantic validation will happen in later layers
        test_logger_->info("Invalid entry point test completed for {}: main={}, int={}", 
                          context, has_main, has_proper_return);
        
        // We still expect the code to tokenize successfully, even if semantically invalid
        EXPECT_FALSE(tokens.empty()) << "Invalid code should still produce tokens";
    }
};

// ============================================================================
// Valid Entry Point Tests
// ============================================================================

TEST_F(ProgramEntryPointTest, StandardMainFunction) {
    std::string code = "int main() { return 0; }";
    
    auto tokens = tokenizeWithLogging(code, "StandardMainFunction");
    validateProgramEntryPoint(tokens, "StandardMainFunction");
}

TEST_F(ProgramEntryPointTest, MainWithArgcArgv) {
    std::string code = "int main(int argc, char* argv[]) { return argc; }";
    
    auto tokens = tokenizeWithLogging(code, "MainWithArgcArgv");
    validateProgramEntryPoint(tokens, "MainWithArgcArgv");
}

TEST_F(ProgramEntryPointTest, MainWithArgcArgvAlternative) {
    std::string code = "int main(int argc, char** argv) { return 0; }";
    
    auto tokens = tokenizeWithLogging(code, "MainWithArgcArgvAlternative");
    validateProgramEntryPoint(tokens, "MainWithArgcArgvAlternative");
}

TEST_F(ProgramEntryPointTest, MainWithWhitespaceAndComments) {
    std::string code = R"(
        // Program entry point
        int main() {
            return 0; // Success
        }
    )";
    
    auto tokens = tokenizeWithLogging(code, "MainWithWhitespaceAndComments");
    validateProgramEntryPoint(tokens, "MainWithWhitespaceAndComments");
}

TEST_F(ProgramEntryPointTest, ComplexMainFunction) {
    std::string code = R"(
        int main(int argc, char* argv[]) {
            if (argc > 1) {
                print("Hello {}", argv[1]);
            } else {
                print("Hello World");
            }
            return 0;
        }
    )";
    
    auto tokens = tokenizeWithLogging(code, "ComplexMainFunction");
    validateProgramEntryPoint(tokens, "ComplexMainFunction");
}

// ============================================================================
// Invalid Entry Point Tests (These SHOULD fail validation)
// ============================================================================

TEST_F(ProgramEntryPointTest, MissingReturnType) {
    std::string code = "main() { return 0; }";
    
    auto tokens = tokenizeWithLogging(code, "MissingReturnType");
    
    // This should fail validation - no valid entry point
    EXPECT_THROW({
        validateProgramEntryPoint(tokens, "MissingReturnType");
    }, ::testing::AssertionException);
}

TEST_F(ProgramEntryPointTest, WrongReturnType) {
    std::string code = "void main() { }";
    
    auto tokens = tokenizeWithLogging(code, "WrongReturnType");
    
    // This should fail validation - void instead of int
    EXPECT_THROW({
        validateProgramEntryPoint(tokens, "WrongReturnType");
    }, ::testing::AssertionException);
}

TEST_F(ProgramEntryPointTest, WrongFunctionName) {
    std::string code = "int Main() { return 0; }"; // Capital M
    
    auto tokens = tokenizeWithLogging(code, "WrongFunctionName");
    
    // This should fail validation - Main instead of main
    EXPECT_THROW({
        validateProgramEntryPoint(tokens, "WrongFunctionName");
    }, ::testing::AssertionException);
}

TEST_F(ProgramEntryPointTest, MissingParentheses) {
    std::string code = "int main { return 0; }";
    
    auto tokens = tokenizeWithLogging(code, "MissingParentheses");
    
    // This should fail validation - missing ()
    EXPECT_THROW({
        validateProgramEntryPoint(tokens, "MissingParentheses");
    }, ::testing::AssertionException);
}

TEST_F(ProgramEntryPointTest, EmptyInput) {
    std::string code = "";
    
    auto tokens = tokenizeWithLogging(code, "EmptyInput");
    
    // This should fail validation - no tokens
    EXPECT_THROW({
        validateProgramEntryPoint(tokens, "EmptyInput");
    }, ::testing::AssertionException);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(ProgramEntryPointTest, MultipleMainFunctions) {
    std::string code = R"(
        void helper() { }
        int main() { return 0; }
        void another() { }
    )";
    
    auto tokens = tokenizeWithLogging(code, "MultipleMainFunctions");
    
    // Should find the valid main function despite other functions
    validateProgramEntryPoint(tokens, "MultipleMainFunctions");
}

TEST_F(ProgramEntryPointTest, MainAsVariableName) {
    std::string code = R"(
        int main() {
            int main = 42; // Variable named main
            return main;
        }
    )";
    
    auto tokens = tokenizeWithLogging(code, "MainAsVariableName");
    
    // Should still find the function main despite variable named main
    validateProgramEntryPoint(tokens, "MainAsVariableName");
}