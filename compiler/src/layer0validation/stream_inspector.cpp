#include "stream_inspector.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace cprime::layer0validation {

void StreamInspector::analyze_stream(
    const std::string& stream_id,
    const std::stringstream& stream,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    
    std::string content = stream.str();
    
    LOG_INFO("=== Stream Analysis: {} ===", stream_id);
    
    // Generate and display statistics
    auto stats = generate_stats(content);
    LOG_INFO("Stream statistics: {}", stats.to_string());
    
    // Show character distribution
    analyze_character_distribution(stream_id, content, logger);
    
    // Show line structure
    analyze_line_structure(stream_id, content, logger);
    
    // Show raw content preview
    show_raw_content(stream_id, content, logger);
}

void StreamInspector::analyze_stream_collection(
    const std::map<std::string, std::stringstream>& streams,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    
    LOG_INFO("=== Stream Collection Analysis ===");
    LOG_INFO("Total streams: {}", streams.size());
    
    // Analyze each stream
    for (const auto& [stream_id, stream] : streams) {
        analyze_stream(stream_id, stream, logger);
    }
    
    // If we have multiple streams, compare them
    if (streams.size() > 1) {
        LOG_INFO("=== Stream Comparison ===");
        auto it1 = streams.begin();
        auto it2 = std::next(it1);
        
        while (it2 != streams.end()) {
            compare_streams(
                it1->first, it1->second.str(),
                it2->first, it2->second.str(),
                logger
            );
            ++it1;
            ++it2;
        }
    }
}

void StreamInspector::analyze_character_distribution(
    const std::string& stream_id,
    const std::string& content,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    
    LOG_DEBUG("Character distribution for stream '{}':", stream_id);
    
    auto stats = generate_stats(content);
    
    if (stats.total_chars > 0) {
        double whitespace_pct = (double(stats.whitespace_chars) / stats.total_chars) * 100.0;
        double printable_pct = (double(stats.printable_chars) / stats.total_chars) * 100.0;
        double alpha_pct = (double(stats.alpha_chars) / stats.total_chars) * 100.0;
        double numeric_pct = (double(stats.numeric_chars) / stats.total_chars) * 100.0;
        
        LOG_DEBUG("  Whitespace: {} chars ({:.1f}%)", stats.whitespace_chars, whitespace_pct);
        LOG_DEBUG("  Printable: {} chars ({:.1f}%)", stats.printable_chars, printable_pct);
        LOG_DEBUG("  Alphabetic: {} chars ({:.1f}%)", stats.alpha_chars, alpha_pct);
        LOG_DEBUG("  Numeric: {} chars ({:.1f}%)", stats.numeric_chars, numeric_pct);
        LOG_DEBUG("  Newlines: {}, Tabs: {}, Spaces: {}", stats.newlines, stats.tabs, stats.spaces);
    }
}

void StreamInspector::analyze_line_structure(
    const std::string& stream_id,
    const std::string& content,
    cprime::Logger& logger,
    size_t max_lines
) {
    auto& log = logger; // Alias for LOG macros
    
    auto lines = split_into_lines(content);
    
    LOG_DEBUG("Line structure for stream '{}' ({} lines):", stream_id, lines.size());
    
    size_t lines_to_show = (max_lines == 0) ? lines.size() : std::min(max_lines, lines.size());
    
    for (size_t i = 0; i < lines_to_show; ++i) {
        std::string escaped = escape_for_display(lines[i]);
        LOG_DEBUG("  Line {}: [{}] '{}'", i + 1, lines[i].length(), escaped);
    }
    
    if (lines.size() > lines_to_show) {
        LOG_DEBUG("  ... ({} more lines)", lines.size() - lines_to_show);
    }
}

void StreamInspector::show_raw_content(
    const std::string& stream_id,
    const std::string& content,
    cprime::Logger& logger,
    size_t max_chars
) {
    auto& log = logger; // Alias for LOG macros
    
    size_t chars_to_show = (max_chars == 0) ? content.length() : std::min(max_chars, content.length());
    
    if (chars_to_show > 0) {
        std::string preview = content.substr(0, chars_to_show);
        std::string escaped = escape_for_display(preview, chars_to_show);
        
        LOG_DEBUG("Raw content preview for '{}' (showing {} of {} chars):", 
                 stream_id, chars_to_show, content.length());
        LOG_DEBUG("'{}'", escaped);
        
        if (content.length() > chars_to_show) {
            LOG_DEBUG("... ({} more characters)", content.length() - chars_to_show);
        }
    } else {
        LOG_DEBUG("Stream '{}' is empty", stream_id);
    }
}

StreamInspector::StreamStats StreamInspector::generate_stats(const std::string& content) {
    StreamStats stats;
    
    stats.total_chars = content.length();
    
    for (char c : content) {
        if (std::isspace(c)) {
            stats.whitespace_chars++;
            if (c == '\n') stats.newlines++;
            else if (c == '\t') stats.tabs++;
            else if (c == ' ') stats.spaces++;
        }
        
        if (std::isprint(c)) {
            stats.printable_chars++;
        }
        
        if (std::isalpha(c)) {
            stats.alpha_chars++;
        }
        
        if (std::isdigit(c)) {
            stats.numeric_chars++;
        }
        
        if (std::ispunct(c)) {
            stats.punctuation_chars++;
        }
    }
    
    // Count lines (number of newlines + 1, or 0 if empty)
    stats.total_lines = stats.total_chars > 0 ? stats.newlines + 1 : 0;
    
    return stats;
}

std::string StreamInspector::StreamStats::to_string() const {
    std::stringstream ss;
    ss << total_chars << " chars, " << total_lines << " lines";
    if (total_chars > 0) {
        ss << " (ws:" << whitespace_chars 
           << ", print:" << printable_chars 
           << ", alpha:" << alpha_chars 
           << ", num:" << numeric_chars << ")";
    }
    return ss.str();
}

void StreamInspector::compare_streams(
    const std::string& stream1_id,
    const std::string& content1,
    const std::string& stream2_id,
    const std::string& content2,
    cprime::Logger& logger
) {
    auto& log = logger; // Alias for LOG macros
    
    LOG_DEBUG("Comparing streams '{}' vs '{}':", stream1_id, stream2_id);
    
    auto stats1 = generate_stats(content1);
    auto stats2 = generate_stats(content2);
    
    LOG_DEBUG("  Size: {} vs {} characters", stats1.total_chars, stats2.total_chars);
    LOG_DEBUG("  Lines: {} vs {}", stats1.total_lines, stats2.total_lines);
    
    if (content1 == content2) {
        LOG_DEBUG("  Content: IDENTICAL");
    } else {
        LOG_DEBUG("  Content: DIFFERENT");
        
        // Show some basic difference info
        size_t min_len = std::min(content1.length(), content2.length());
        size_t first_diff = 0;
        
        for (size_t i = 0; i < min_len; ++i) {
            if (content1[i] != content2[i]) {
                first_diff = i;
                break;
            }
        }
        
        if (first_diff < min_len || content1.length() != content2.length()) {
            LOG_DEBUG("  First difference at position: {}", first_diff);
        }
    }
}

std::vector<std::string> StreamInspector::split_into_lines(const std::string& content) {
    std::vector<std::string> lines;
    std::stringstream ss(content);
    std::string line;
    
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    // If the content doesn't end with a newline, getline misses the last line
    if (!content.empty() && content.back() != '\n' && lines.empty()) {
        lines.push_back(content);
    }
    
    return lines;
}

std::string StreamInspector::escape_for_display(const std::string& content, size_t max_length) {
    std::string result;
    result.reserve(std::min(content.length() * 2, max_length)); // Estimate for escaping
    
    for (char c : content) {
        if (result.length() >= max_length - 10) { // Leave room for "..."
            result += "...";
            break;
        }
        
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            default:
                if (c >= 32 && c <= 126) { // Printable ASCII
                    result += c;
                } else {
                    // Show non-printable as hex
                    result += "\\x";
                    result += "0123456789ABCDEF"[(c >> 4) & 0xF];
                    result += "0123456789ABCDEF"[c & 0xF];
                }
                break;
        }
    }
    
    return result;
}

char StreamInspector::classify_character(char c) {
    if (std::isspace(c)) return 'W'; // Whitespace
    if (std::isalpha(c)) return 'A'; // Alpha
    if (std::isdigit(c)) return 'N'; // Numeric
    if (std::ispunct(c)) return 'P'; // Punctuation
    return '?'; // Other
}

} // namespace cprime::layer0validation