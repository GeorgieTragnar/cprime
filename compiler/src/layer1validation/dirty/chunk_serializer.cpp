#include "chunk_serializer.h"
#include "enum_stringifier.h"
#include <sstream>
#include <stdexcept>
#include <regex>

namespace cprime::layer1validation {

// ============================================================================
// Single Chunk Serialization
// ============================================================================

std::string ChunkSerializer::serialize(const ProcessingChunk& chunk, const StringTable& string_table) {
    std::ostringstream oss;
    
    if (chunk.is_unprocessed()) {
        // Format: CHUNK[UNPROCESSED]: content="some code", start=10, end=19, line=2, col=5
        const auto& str = chunk.get_string();
        oss << "CHUNK[UNPROCESSED]: content=\"" << escape_string(str) << "\""
            << ", start=" << chunk.start_pos
            << ", end=" << chunk.end_pos  
            << ", line=" << chunk.line
            << ", col=" << chunk.column;
    } else {
        // Format: CHUNK[PROCESSED]: raw=IDENTIFIER, token=IDENTIFIER, pos=15, line=2, col=10, value=StringIndex[3]:"variable"
        const auto& token = chunk.get_token();
        oss << "CHUNK[PROCESSED]: raw=" << EnumStringifier::erawtoken_to_string(token._raw_token)
            << ", token=" << EnumStringifier::etoken_to_string(token._token)
            << ", pos=" << token._position
            << ", line=" << token._line
            << ", col=" << token._column
            << ", value=";
        
        // Serialize the variant value based on its type
        std::visit([&oss, &string_table](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                oss << "none";
            } else if constexpr (std::is_same_v<T, StringIndex>) {
                if (string_table.is_valid_index(value)) {
                    oss << "StringIndex[" << value.value << "]:\"" 
                        << escape_string(string_table.get_string(value)) << "\"";
                } else {
                    oss << "StringIndex[" << value.value << "]:INVALID";
                }
            } else if constexpr (std::is_same_v<T, int32_t>) {
                oss << "int32:" << value;
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                oss << "uint32:" << value;
            } else if constexpr (std::is_same_v<T, int64_t>) {
                oss << "int64:" << value;
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                oss << "uint64:" << value;
            } else if constexpr (std::is_same_v<T, long long>) {
                oss << "longlong:" << value;
            } else if constexpr (std::is_same_v<T, unsigned long long>) {
                oss << "ulonglong:" << value;
            } else if constexpr (std::is_same_v<T, float>) {
                oss << "float:" << value;
            } else if constexpr (std::is_same_v<T, double>) {
                oss << "double:" << value;
            } else if constexpr (std::is_same_v<T, long double>) {
                oss << "longdouble:" << value;
            } else if constexpr (std::is_same_v<T, char>) {
                oss << "char:'" << value << "'";
            } else if constexpr (std::is_same_v<T, wchar_t>) {
                oss << "wchar:" << static_cast<int>(value);
            } else if constexpr (std::is_same_v<T, char16_t>) {
                oss << "char16:" << static_cast<int>(value);
            } else if constexpr (std::is_same_v<T, char32_t>) {
                oss << "char32:" << static_cast<int>(value);
            } else if constexpr (std::is_same_v<T, bool>) {
                oss << "bool:" << (value ? "true" : "false");
            } else {
                oss << "unknown";
            }
        }, token._literal_value);
    }
    
    return oss.str();
}

ProcessingChunk ChunkSerializer::deserialize(const std::string& serialized, StringTable& string_table) {
    if (serialized.find("CHUNK[UNPROCESSED]:") == 0) {
        return parse_unprocessed_chunk(serialized);
    } else if (serialized.find("CHUNK[PROCESSED]:") == 0) {
        return parse_processed_chunk(serialized, string_table);
    } else {
        throw std::invalid_argument("Invalid chunk format: " + serialized);
    }
}

// ============================================================================
// Batch Chunk Serialization
// ============================================================================

std::string ChunkSerializer::serialize_chunks(const std::vector<ProcessingChunk>& chunks, const StringTable& string_table) {
    std::ostringstream oss;
    for (size_t i = 0; i < chunks.size(); ++i) {
        if (i > 0) oss << "\n";
        oss << serialize(chunks[i], string_table);
    }
    return oss.str();
}

std::vector<ProcessingChunk> ChunkSerializer::parse_chunks(const std::string& serialized, StringTable& string_table) {
    std::vector<ProcessingChunk> result;
    std::istringstream iss(serialized);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            result.push_back(deserialize(line, string_table));
        }
    }
    
    return result;
}

// ============================================================================
// Validation Utilities
// ============================================================================

bool ChunkSerializer::is_valid_chunk_format(const std::string& serialized) {
    try {
        StringTable temp_table;
        deserialize(serialized, temp_table);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string ChunkSerializer::compare_chunks(const std::vector<ProcessingChunk>& expected, 
                                          const std::vector<ProcessingChunk>& actual,
                                          const StringTable& string_table) {
    if (expected.size() != actual.size()) {
        return "Chunk count mismatch: expected " + std::to_string(expected.size()) + 
               ", got " + std::to_string(actual.size());
    }
    
    for (size_t i = 0; i < expected.size(); ++i) {
        std::string expected_str = serialize(expected[i], string_table);
        std::string actual_str = serialize(actual[i], string_table);
        
        if (expected_str != actual_str) {
            return "Chunk " + std::to_string(i) + " mismatch:\n" +
                   "Expected: " + expected_str + "\n" +
                   "Actual:   " + actual_str;
        }
    }
    
    return ""; // All chunks match
}

// ============================================================================
// Helper Functions
// ============================================================================

std::string ChunkSerializer::escape_string(const std::string& str) {
    std::string result;
    result.reserve(str.size() * 2); // Reserve space for potential escaping
    
    for (char c : str) {
        switch (c) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    
    return result;
}

std::string ChunkSerializer::unescape_string(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\\' && i + 1 < str.size()) {
            switch (str[i + 1]) {
                case '\\': result += '\\'; ++i; break;
                case '"': result += '"'; ++i; break;
                case 'n': result += '\n'; ++i; break;
                case 'r': result += '\r'; ++i; break;
                case 't': result += '\t'; ++i; break;
                default: result += str[i]; break; // Keep the backslash if not a known escape
            }
        } else {
            result += str[i];
        }
    }
    
    return result;
}

ProcessingChunk ChunkSerializer::parse_unprocessed_chunk(const std::string& line) {
    // Parse: CHUNK[UNPROCESSED]: content="some code", start=10, end=19, line=2, col=5
    std::regex pattern(R"(CHUNK\[UNPROCESSED\]: content=\"([^\"]*)\", start=(\d+), end=(\d+), line=(\d+), col=(\d+))");
    std::smatch matches;
    
    if (std::regex_match(line, matches, pattern)) {
        std::string content = unescape_string(matches[1].str());
        uint32_t start = std::stoul(matches[2].str());
        uint32_t end = std::stoul(matches[3].str());
        uint32_t line_num = std::stoul(matches[4].str());
        uint32_t column = std::stoul(matches[5].str());
        
        return ProcessingChunk(std::move(content), start, end, line_num, column);
    } else {
        throw std::invalid_argument("Invalid unprocessed chunk format: " + line);
    }
}

ProcessingChunk ChunkSerializer::parse_processed_chunk(const std::string& line, StringTable& string_table) {
    // This is a simplified parser - in a real implementation, you'd need more robust parsing
    // For now, we'll implement basic functionality
    throw std::runtime_error("ProcessingChunk deserialization not yet implemented - use for serialization only");
}

} // namespace cprime::layer1validation