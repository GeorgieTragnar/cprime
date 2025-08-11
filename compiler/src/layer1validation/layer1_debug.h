#pragma once

#include "../layer1/tokenizer.h"
#include "../commons/dirty/string_table.h"
#include "../commons/logger.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>

namespace cprime::layer1validation {

/**
 * Layer 1 debugging utilities for CLI integration.
 * 
 * Provides debugging and analysis capabilities for the tokenization layer,
 * similar to layer0validation for input processing. Used by cprime_cli
 * and analyze.sh script for Layer 1 debugging modes.
 * 
 * Features:
 * - Step-by-step tokenization analysis
 * - Individual sublayer processing inspection
 * - Token export for test case creation
 * - Detailed ProcessingChunk state visualization
 */
class Layer1Debug {
public:
    // ========================================================================
    // Tokenization Analysis
    // ========================================================================
    
    /**
     * Show complete tokenization process step by step.
     * Displays input, all sublayer ProcessingChunk states, and final RawToken output.
     * 
     * @param stream Input stringstream to tokenize
     * @param string_table StringTable for string interning
     * @param verbose If true, shows detailed ProcessingChunk serialization
     */
    static void show_tokenization_steps(std::stringstream& stream, StringTable& string_table, bool verbose = false);
    
    /**
     * Show only the final tokenization result.
     * Displays the final RawToken vector in human-readable format.
     * 
     * @param stream Input stringstream to tokenize
     * @param string_table StringTable for string interning
     * @param show_positions If true, includes line/column position information
     */
    static void show_final_tokens(std::stringstream& stream, StringTable& string_table, bool show_positions = true);
    
    /**
     * Analyze specific sublayer processing in detail.
     * Shows ProcessingChunk state before and after the specified sublayer.
     * 
     * @param stream Input stringstream to tokenize
     * @param string_table StringTable for string interning
     * @param sublayer_name Name of sublayer ("1a", "1b", "1c", "1d", "1e")
     */
    static void show_sublayer_processing(std::stringstream& stream, StringTable& string_table, const std::string& sublayer_name);
    
    // ========================================================================
    // Token Export and Test Creation
    // ========================================================================
    
    /**
     * Export tokenization result to file for test case creation.
     * Generates layer2 expected output file in the correct format.
     * 
     * @param stream Input stringstream to tokenize
     * @param string_table StringTable for string interning
     * @param output_file Path to output file
     */
    static void export_tokens_to_file(std::stringstream& stream, StringTable& string_table, const std::string& output_file);
    
    /**
     * Generate test case files from input.
     * Creates both layer1 (input) and layer2 (expected output) files.
     * 
     * @param input_content Source code content
     * @param test_case_name Name for the test case
     * @param test_cases_dir Directory to create test case in
     */
    static void generate_test_case(const std::string& input_content, const std::string& test_case_name, const std::string& test_cases_dir);
    
    // ========================================================================
    // Statistics and Analysis
    // ========================================================================
    
    /**
     * Show tokenization statistics.
     * Displays token counts by type, string table usage, etc.
     * 
     * @param stream Input stringstream to tokenize
     * @param string_table StringTable for string interning
     */
    static void show_tokenization_statistics(std::stringstream& stream, StringTable& string_table);
    
    /**
     * Validate tokenization against expected output.
     * Used for debugging tokenizer issues by comparing against known good output.
     * 
     * @param stream Input stringstream to tokenize
     * @param string_table StringTable for string interning
     * @param expected_output_file Path to expected output file
     * @return true if tokenization matches expected output
     */
    static bool validate_tokenization(std::stringstream& stream, StringTable& string_table, const std::string& expected_output_file);

private:
    // ========================================================================
    // Internal Helper Functions
    // ========================================================================
    
    /**
     * Run tokenization and capture all intermediate states.
     * Returns vectors of ProcessingChunks for each sublayer.
     */
    struct TokenizationStates {
        std::vector<ProcessingChunk> after_1a;
        std::vector<ProcessingChunk> after_1b;
        std::vector<ProcessingChunk> after_1c;
        std::vector<ProcessingChunk> after_1d;
        std::vector<RawToken> final_tokens;
    };
    
    static TokenizationStates capture_tokenization_states(std::stringstream& stream, StringTable& string_table);
    
    /**
     * Display ProcessingChunk vector with optional details.
     */
    static void display_processing_chunks(const std::vector<ProcessingChunk>& chunks, 
                                        const StringTable& string_table,
                                        const std::string& stage_name,
                                        bool verbose = false);
    
    /**
     * Display RawToken vector with optional position info.
     */
    static void display_raw_tokens(const std::vector<RawToken>& tokens,
                                 const StringTable& string_table,
                                 const std::string& stage_name,
                                 bool show_positions = true);
    
    /**
     * Count tokens by ERawToken type for statistics.
     */
    static std::map<ERawToken, size_t> count_tokens_by_type(const std::vector<RawToken>& tokens);
    
    /**
     * Get sublayer name for display purposes.
     */
    static std::string get_sublayer_display_name(const std::string& sublayer_code);
    
    /**
     * Validate sublayer name parameter.
     */
    static bool is_valid_sublayer_name(const std::string& sublayer_name);
};

} // namespace cprime::layer1validation