#pragma once

#include "../commons/logger.h"
#include <string>
#include <sstream>
#include <map>
#include <vector>

namespace cprime::layer0validation {

/**
 * StreamInspector - Advanced stringstream analysis and debugging
 * 
 * Provides detailed analysis of processed stringstreams for debugging purposes.
 * This class helps understand the content and structure of input streams
 * before they are passed to subsequent compilation layers.
 */
class StreamInspector {
public:
    /**
     * Comprehensive analysis of a single stringstream
     * @param stream_id Identifier for the stream
     * @param stream The stringstream to analyze
     * @param logger Logger for debug output
     */
    static void analyze_stream(
        const std::string& stream_id,
        const std::stringstream& stream,
        cprime::Logger& logger
    );
    
    /**
     * Analyze multiple streams with comparison
     * @param streams Map of stream_id to stringstream
     * @param logger Logger for debug output
     */
    static void analyze_stream_collection(
        const std::map<std::string, std::stringstream>& streams,
        cprime::Logger& logger
    );
    
    /**
     * Character-level analysis of stream content
     * @param stream_id Identifier for the stream
     * @param content Stream content as string
     * @param logger Logger for debug output
     */
    static void analyze_character_distribution(
        const std::string& stream_id,
        const std::string& content,
        cprime::Logger& logger
    );
    
    /**
     * Line-by-line breakdown of stream content
     * @param stream_id Identifier for the stream
     * @param content Stream content as string
     * @param logger Logger for debug output
     * @param max_lines Maximum number of lines to show (0 = all)
     */
    static void analyze_line_structure(
        const std::string& stream_id,
        const std::string& content,
        cprime::Logger& logger,
        size_t max_lines = 10
    );
    
    /**
     * Show raw stream content with formatting for debugging
     * @param stream_id Identifier for the stream
     * @param content Stream content as string
     * @param logger Logger for debug output
     * @param max_chars Maximum characters to show (0 = all)
     */
    static void show_raw_content(
        const std::string& stream_id,
        const std::string& content,
        cprime::Logger& logger,
        size_t max_chars = 500
    );
    
    /**
     * Generate summary statistics for a stream
     * @param content Stream content as string
     * @return Statistics structure with counts and analysis
     */
    struct StreamStats {
        size_t total_chars = 0;
        size_t total_lines = 0;
        size_t whitespace_chars = 0;
        size_t printable_chars = 0;
        size_t alpha_chars = 0;
        size_t numeric_chars = 0;
        size_t punctuation_chars = 0;
        size_t newlines = 0;
        size_t tabs = 0;
        size_t spaces = 0;
        
        std::string to_string() const;
    };
    
    static StreamStats generate_stats(const std::string& content);
    
    /**
     * Compare two streams for differences
     * @param stream1_id First stream identifier
     * @param content1 First stream content
     * @param stream2_id Second stream identifier
     * @param content2 Second stream content
     * @param logger Logger for debug output
     */
    static void compare_streams(
        const std::string& stream1_id,
        const std::string& content1,
        const std::string& stream2_id,
        const std::string& content2,
        cprime::Logger& logger
    );

private:
    // Helper functions
    static std::vector<std::string> split_into_lines(const std::string& content);
    static std::string escape_for_display(const std::string& content, size_t max_length = 100);
    static char classify_character(char c);
};

} // namespace cprime::layer0validation