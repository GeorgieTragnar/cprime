#pragma once

#include "enum/token.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace cprime {

/**
 * TokenizerState - Central source position tracking owned by orchestrator.
 * 
 * Responsibilities:
 * - Track tokenization position across all input streams
 * - Provide source correlation for error reporting
 * - Map token indices back to exact source positions
 * - Support multiple input files with unified position tracking
 * 
 * Design: Owned by CompilationContext, used by all layers for error correlation.
 */
class TokenizerState {
public:
    /**
     * Source file information for position tracking.
     */
    struct SourceFile {
        std::string file_path;
        std::string content;
        std::vector<size_t> line_starts;  // Byte offsets where each line starts
        
        SourceFile(const std::string& path, const std::string& source)
            : file_path(path), content(source) {
            calculate_line_starts();
        }
        
    private:
        void calculate_line_starts() {
            line_starts.clear();
            line_starts.push_back(0); // Line 1 starts at position 0
            
            for (size_t i = 0; i < content.length(); ++i) {
                if (content[i] == '\n') {
                    line_starts.push_back(i + 1);
                }
            }
        }
    };
    
    /**
     * Source position information.
     */
    struct SourcePosition {
        std::string file_path;
        uint32_t line;
        uint32_t column;
        uint32_t byte_offset;
        
        SourcePosition() : line(0), column(0), byte_offset(0) {}
        
        SourcePosition(const std::string& file, uint32_t l, uint32_t c, uint32_t offset)
            : file_path(file), line(l), column(c), byte_offset(offset) {}
        
        std::string to_string() const {
            return file_path + ":" + std::to_string(line) + ":" + std::to_string(column);
        }
    };
    
    /**
     * Token reference for error correlation.
     */
    struct TokenReference {
        size_t token_index;           // Index in flattened token stream
        std::string source_file;      // Source file this token came from
        SourcePosition position;      // Exact source position
        
        TokenReference(size_t idx, const std::string& file, const SourcePosition& pos)
            : token_index(idx), source_file(file), position(pos) {}
    };
    
private:
    // Source file tracking
    std::map<std::string, SourceFile> source_files_;
    
    // Token position mapping
    std::vector<TokenReference> token_references_;
    
    // Current processing state
    std::string current_file_;
    uint32_t current_line_;
    uint32_t current_column_;
    uint32_t current_byte_offset_;
    
public:
    TokenizerState() : current_line_(1), current_column_(1), current_byte_offset_(0) {}
    
    /**
     * Register a source file for tokenization.
     */
    void add_source_file(const std::string& file_path, const std::string& content) {
        source_files_.emplace(file_path, SourceFile(file_path, content));
    }
    
    /**
     * Begin tokenizing a specific source file.
     */
    void begin_file(const std::string& file_path) {
        current_file_ = file_path;
        current_line_ = 1;
        current_column_ = 1;
        current_byte_offset_ = 0;
    }
    
    /**
     * Advance position tracking when consuming a character.
     */
    void advance_position(char consumed_char) {
        current_byte_offset_++;
        
        if (consumed_char == '\n') {
            current_line_++;
            current_column_ = 1;
        } else {
            current_column_++;
        }
    }
    
    /**
     * Record a token with its source position.
     */
    void record_token(const EToken& token) {
        SourcePosition pos(current_file_, current_line_, current_column_, current_byte_offset_);
        TokenReference ref(token_references_.size(), current_file_, pos);
        token_references_.push_back(ref);
    }
    
    /**
     * Get source position for a token by its index.
     */
    SourcePosition get_token_position(size_t token_index) const {
        if (token_index >= token_references_.size()) {
            return SourcePosition(); // Invalid position
        }
        return token_references_[token_index].position;
    }
    
    /**
     * Get source context around a position (for error reporting).
     */
    std::string get_source_context(const SourcePosition& pos, size_t context_lines = 2) const {
        auto it = source_files_.find(pos.file_path);
        if (it == source_files_.end()) {
            return "Source file not found";
        }
        
        const SourceFile& file = it->second;
        
        if (pos.line == 0 || pos.line > file.line_starts.size()) {
            return "Invalid line number";
        }
        
        std::string context;
        
        // Calculate line range to show
        size_t start_line = (pos.line > context_lines) ? pos.line - context_lines : 1;
        size_t end_line = std::min(pos.line + context_lines, static_cast<uint32_t>(file.line_starts.size()));
        
        for (size_t line_num = start_line; line_num <= end_line; ++line_num) {
            size_t line_start = file.line_starts[line_num - 1];
            size_t line_end = (line_num < file.line_starts.size()) 
                ? file.line_starts[line_num] - 1  // Exclude the newline
                : file.content.length();
            
            std::string line_content = file.content.substr(line_start, line_end - line_start);
            
            context += std::to_string(line_num) + " | " + line_content + "\n";
            
            // Add error pointer for the target line
            if (line_num == pos.line) {
                context += std::string(std::to_string(line_num).length() + 3 + pos.column - 1, ' ') + "^\n";
            }
        }
        
        return context;
    }
    
    /**
     * Get total number of recorded tokens.
     */
    size_t get_token_count() const {
        return token_references_.size();
    }
    
    /**
     * Get source file information.
     */
    const SourceFile* get_source_file(const std::string& file_path) const {
        auto it = source_files_.find(file_path);
        return (it != source_files_.end()) ? &it->second : nullptr;
    }
    
    /**
     * Get all source files.
     */
    const std::map<std::string, SourceFile>& get_all_source_files() const {
        return source_files_;
    }
    
    /**
     * Clear all state (for reuse).
     */
    void clear() {
        source_files_.clear();
        token_references_.clear();
        current_file_.clear();
        current_line_ = 1;
        current_column_ = 1;
        current_byte_offset_ = 0;
    }
    
    /**
     * Get current position info (for debugging).
     */
    SourcePosition get_current_position() const {
        return SourcePosition(current_file_, current_line_, current_column_, current_byte_offset_);
    }
};

} // namespace cprime