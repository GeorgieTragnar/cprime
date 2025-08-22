#include "contextualization_pattern_matcher.h"
#include "../../commons/logger.h"

namespace cprime::layer2_contextualization {

ContextualizationPatternMatcher& ContextualizationPatternMatcher::getInstance() {
    static ContextualizationPatternMatcher instance;
    
    // Initialize builtin patterns on first access
    if (!instance.patterns_initialized_) {
        instance.initialize_builtin_patterns();
        instance.patterns_initialized_ = true;
    }
    
    return instance;
}

ContextualizationPatternMatcher::ContextualizationPatternMatcher() {
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    LOG_DEBUG("ContextualizationPatternMatcher singleton created");
}

// Main pattern matching interface
PatternMatchResult ContextualizationPatternMatcher::match_header_pattern(const Instruction& header_instruction) {
    return match_instruction_against_tree(header_instruction, header_pattern_tree_.get());
}

PatternMatchResult ContextualizationPatternMatcher::match_footer_pattern(const Instruction& footer_instruction) {
    return match_instruction_against_tree(footer_instruction, footer_pattern_tree_.get());
}

PatternMatchResult ContextualizationPatternMatcher::match_body_pattern(const Instruction& body_instruction) {
    return match_instruction_against_tree(body_instruction, body_pattern_tree_.get());
}

// Pattern registration interface
void ContextualizationPatternMatcher::register_header_pattern(const Pattern& pattern) {
    header_patterns_.push_back(std::make_unique<Pattern>(pattern));
    // Rebuild tree after pattern addition
    build_pattern_tree(header_pattern_tree_, header_patterns_);
}

void ContextualizationPatternMatcher::register_footer_pattern(const Pattern& pattern) {
    footer_patterns_.push_back(std::make_unique<Pattern>(pattern));
    // Rebuild tree after pattern addition
    build_pattern_tree(footer_pattern_tree_, footer_patterns_);
}

void ContextualizationPatternMatcher::register_body_pattern(const Pattern& pattern) {
    body_patterns_.push_back(std::make_unique<Pattern>(pattern));
    // Rebuild tree after pattern addition
    build_pattern_tree(body_pattern_tree_, body_patterns_);
}

// Debug and testing interface
size_t ContextualizationPatternMatcher::get_header_pattern_count() const {
    return header_patterns_.size();
}

size_t ContextualizationPatternMatcher::get_footer_pattern_count() const {
    return footer_patterns_.size();
}

size_t ContextualizationPatternMatcher::get_body_pattern_count() const {
    return body_patterns_.size();
}

void ContextualizationPatternMatcher::clear_all_patterns() {
    header_patterns_.clear();
    footer_patterns_.clear();
    body_patterns_.clear();
    header_pattern_tree_.reset();
    footer_pattern_tree_.reset();
    body_pattern_tree_.reset();
    patterns_initialized_ = false;
}

// Core pattern matching algorithm
PatternMatchResult ContextualizationPatternMatcher::match_instruction_against_tree(
    const Instruction& instruction, 
    PatternNode* tree_root) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    // Handle empty tree
    if (!tree_root) {
        LOG_DEBUG("No pattern tree available - returning no match");
        return PatternMatchResult(); // success = false
    }
    
    // Handle empty instruction
    if (instruction._tokens.empty()) {
        LOG_DEBUG("Empty instruction - returning no match");
        return PatternMatchResult(); // success = false
    }
    
    // Preprocess tokens to create clean index vector
    std::vector<size_t> clean_indices = preprocess_instruction_tokens(instruction);
    
    LOG_INFO("🔍 Pattern matching: {} original tokens, {} clean indices", 
              instruction._tokens.size(), clean_indices.size());
    for (size_t i = 0; i < clean_indices.size(); ++i) {
        size_t token_idx = clean_indices[i];
        LOG_INFO("  Clean[{}] = Token[{}]: {}", i, token_idx, static_cast<int>(instruction._tokens[token_idx]._token));
    }
    
    // Start traversal from tree root  
    LOG_INFO("🌲 Starting tree traversal from root (tree_root = {})", tree_root ? "valid" : "null");
    std::vector<ContextualTokenResult> accumulated_results;
    PatternMatchResult result = traverse_pattern_tree(tree_root, instruction, clean_indices, 0, accumulated_results);
    LOG_INFO("🌲 Tree traversal completed: success = {}", result.success);
    return result;
}

// Token preprocessing - creates clean index vector skipping whitespace/comments
std::vector<size_t> ContextualizationPatternMatcher::preprocess_instruction_tokens(const Instruction& instruction) {
    std::vector<size_t> clean_indices;
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    bool last_was_whitespace = false;
    
    for (size_t i = 0; i < instruction._tokens.size(); ++i) {
        const Token& token = instruction._tokens[i];
        
        // Skip comments completely
        if (token._token == EToken::COMMENT) {
            LOG_DEBUG("Skipping COMMENT at index {}", i);
            continue;
        }
        
        // Handle whitespace consolidation
        if (token._token == EToken::SPACE || 
            token._token == EToken::TAB ||
            token._token == EToken::NEWLINE ||
            token._token == EToken::CARRIAGE_RETURN) {
            
            if (!last_was_whitespace) {
                // First whitespace in sequence - track it
                clean_indices.push_back(i);
                LOG_DEBUG("Tracking first whitespace at index {}", i);
                last_was_whitespace = true;
            } else {
                LOG_DEBUG("Skipping consecutive whitespace at index {}", i);
            }
            continue;
        }
        
        // Regular token - always track
        clean_indices.push_back(i);
        LOG_DEBUG("Tracking regular token {} at index {}", static_cast<int>(token._token), i);
        last_was_whitespace = false;
    }
    
    return clean_indices;
}

// Tree building helpers
void ContextualizationPatternMatcher::build_pattern_tree(
    std::unique_ptr<PatternNode>& tree_root, 
    const std::vector<std::unique_ptr<Pattern>>& patterns) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    // Clear existing tree
    tree_root.reset();
    
    if (patterns.empty()) {
        LOG_DEBUG("No patterns to build tree from");
        return;
    }
    
    // Create dummy root node (never matched against, just holds pattern trees)
    PatternElement dummy_root(PatternElementType::END_OF_PATTERN);
    dummy_root.type = static_cast<PatternElementType>(-1); // Special marker for root
    tree_root = std::make_unique<PatternNode>(dummy_root);
    
    // Insert each pattern into the tree
    for (const auto& pattern : patterns) {
        insert_pattern_into_tree(tree_root.get(), pattern.get());
    }
    
    LOG_DEBUG("Built pattern tree with {} patterns", patterns.size());
}

void ContextualizationPatternMatcher::insert_pattern_into_tree(PatternNode* root, const Pattern* pattern) {
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    LOG_DEBUG("Inserting pattern '{}' with {} elements", pattern->pattern_name, pattern->elements.size());
    
    PatternNode* current_node = root;
    
    // Traverse/create nodes for each pattern element
    for (size_t i = 0; i < pattern->elements.size(); ++i) {
        const PatternElement& element = pattern->elements[i];
        
        // Look for existing child with matching element
        PatternNode* matching_child = nullptr;
        for (auto& child : current_node->children) {
            if (child->element.type == element.type) {
                // For concrete tokens/groups, also check the accepted tokens
                if (element.type == PatternElementType::CONCRETE_TOKEN ||
                    element.type == PatternElementType::CONCRETE_TOKEN_GROUP) {
                    
                    if (child->element.accepted_tokens == element.accepted_tokens &&
                        child->element.target_contextual_token == element.target_contextual_token) {
                        matching_child = child.get();
                        break;
                    }
                } else {
                    // For other element types, type match is sufficient
                    if (child->element.target_contextual_token == element.target_contextual_token) {
                        matching_child = child.get();
                        break;
                    }
                }
            }
        }
        
        // Create new child if no match found
        if (!matching_child) {
            auto new_child = std::make_unique<PatternNode>(element);
            matching_child = new_child.get();
            current_node->children.push_back(std::move(new_child));
            
            LOG_DEBUG("Created new tree node for element type {}", static_cast<int>(element.type));
        } else {
            LOG_DEBUG("Reusing existing tree node for element type {}", static_cast<int>(element.type));
        }
        
        current_node = matching_child;
        
        // If this is the last element (END_OF_PATTERN), mark it as complete
        if (element.type == PatternElementType::END_OF_PATTERN) {
            current_node->is_end_of_pattern = true;
            current_node->complete_pattern = const_cast<Pattern*>(pattern);
            LOG_DEBUG("Marked node as END_OF_PATTERN for pattern '{}'", pattern->pattern_name);
        }
    }
    
    LOG_DEBUG("Successfully inserted pattern '{}' into tree", pattern->pattern_name);
}

// Pattern matching traversal
PatternMatchResult ContextualizationPatternMatcher::traverse_pattern_tree(
    PatternNode* current_node,
    const Instruction& instruction,
    const std::vector<size_t>& clean_indices,
    size_t current_index,
    std::vector<ContextualTokenResult>& accumulated_results) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    LOG_INFO("🔄 TRAVERSAL: Entering node (current_index={}/{}, children={}, is_end={})", 
             current_index, clean_indices.size(), 
             current_node ? current_node->children.size() : 0,
             current_node ? current_node->is_end_of_pattern : false);
    
    if (!current_node) {
        LOG_INFO("❌ TRAVERSAL: Reached null node");
        return PatternMatchResult(); // success = false
    }
    
    LOG_INFO("🎯 TRAVERSAL: Current element type: {}", static_cast<int>(current_node->element.type));
    
    // Special handling for dummy root node (type -1)
    if (static_cast<int>(current_node->element.type) == -1) {
        LOG_INFO("🌲 TRAVERSAL: At dummy root node, exploring children directly");
        // Skip matching, go directly to children exploration
        std::vector<ContextualTokenResult> new_accumulated = accumulated_results;
        
        // Try each child node (pattern starts)
        LOG_INFO("🌿 TRAVERSAL: Exploring {} child nodes from root", current_node->children.size());
        for (size_t i = 0; i < current_node->children.size(); ++i) {
            auto& child = current_node->children[i];
            LOG_INFO("  🌿 Root Child[{}]: element_type={}", i, static_cast<int>(child->element.type));
            
            PatternMatchResult child_result = traverse_pattern_tree(
                child.get(), instruction, clean_indices, current_index, new_accumulated
            );
            
            if (child_result.success) {
                LOG_INFO("✅ TRAVERSAL: Root child {} succeeded, returning result", i);
                return child_result;
            }
            
            LOG_INFO("❌ TRAVERSAL: Root child {} failed", i);
        }
        
        LOG_INFO("❌ TRAVERSAL: No root children succeeded (tried {})", current_node->children.size());
        return PatternMatchResult(); // success = false
    }
    
    // If this is an END_OF_PATTERN node, check if we've consumed all tokens
    if (current_node->is_end_of_pattern) {
        if (current_index >= clean_indices.size()) {
            LOG_INFO("✅ TRAVERSAL: Successfully reached END_OF_PATTERN with all tokens consumed");
            return PatternMatchResult(current_node->complete_pattern, accumulated_results);
        } else {
            LOG_INFO("❌ TRAVERSAL: END_OF_PATTERN but {} tokens remain", 
                     clean_indices.size() - current_index);
            return PatternMatchResult(); // success = false
        }
    }
    
    LOG_INFO("🧩 TRAVERSAL: Attempting to match element at index {}", current_index);
    
    // Try to match current pattern element
    ContextualTokenResult element_result(EContextualToken::INVALID, {});
    size_t next_index = current_index;
    
    bool element_matched = matches_pattern_element(
        current_node->element, instruction, clean_indices, next_index, element_result
    );
    
    if (!element_matched) {
        LOG_INFO("❌ TRAVERSAL: Pattern element {} failed to match", 
                 static_cast<int>(current_node->element.type));
        return PatternMatchResult(); // success = false
    }
    
    LOG_INFO("✅ TRAVERSAL: Pattern element {} matched successfully", 
             static_cast<int>(current_node->element.type));
    
    // Add the matched element to accumulated results (if it generated a contextual token)
    std::vector<ContextualTokenResult> new_accumulated = accumulated_results;
    if (element_result.contextual_token != EContextualToken::INVALID) {
        new_accumulated.push_back(element_result);
        LOG_INFO("➕ TRAVERSAL: Added contextual token {} to results", 
                 static_cast<int>(element_result.contextual_token));
    }
    
    // Try each child node (branch exploration)
    LOG_INFO("🌿 TRAVERSAL: Exploring {} child nodes", current_node->children.size());
    for (size_t i = 0; i < current_node->children.size(); ++i) {
        auto& child = current_node->children[i];
        LOG_INFO("  🌿 Child[{}]: element_type={}", i, static_cast<int>(child->element.type));
        
        PatternMatchResult child_result = traverse_pattern_tree(
            child.get(), instruction, clean_indices, next_index, new_accumulated
        );
        
        if (child_result.success) {
            LOG_INFO("✅ TRAVERSAL: Child node {} succeeded, returning result", i);
            return child_result;
        }
        
        LOG_INFO("❌ TRAVERSAL: Child node {} failed", i);
    }
    
    LOG_INFO("❌ TRAVERSAL: No child nodes succeeded (tried {})", current_node->children.size());
    return PatternMatchResult(); // success = false
}

// Pattern element matching helpers
bool ContextualizationPatternMatcher::matches_pattern_element(
    const PatternElement& element,
    const Instruction& instruction,
    const std::vector<size_t>& clean_indices,
    size_t& current_index,
    ContextualTokenResult& result) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    switch (element.type) {
        case PatternElementType::CONCRETE_TOKEN: {
            if (current_index >= clean_indices.size()) {
                LOG_DEBUG("CONCRETE_TOKEN: No more tokens available");
                return false;
            }
            
            size_t token_idx = clean_indices[current_index];
            EToken token = instruction._tokens[token_idx]._token;
            
            if (matches_concrete_token(element, token)) {
                result = ContextualTokenResult(element.target_contextual_token, {token_idx});
                current_index++;
                LOG_DEBUG("CONCRETE_TOKEN: Matched {} at index {}", 
                         static_cast<int>(token), token_idx);
                return true;
            }
            
            LOG_DEBUG("CONCRETE_TOKEN: Token {} doesn't match expected", static_cast<int>(token));
            return false;
        }
        
        case PatternElementType::CONCRETE_TOKEN_GROUP: {
            if (current_index >= clean_indices.size()) {
                LOG_DEBUG("CONCRETE_TOKEN_GROUP: No more tokens available");
                return false;
            }
            
            size_t token_idx = clean_indices[current_index];
            EToken token = instruction._tokens[token_idx]._token;
            
            if (matches_concrete_token_group(element, token)) {
                result = ContextualTokenResult(element.target_contextual_token, {token_idx});
                current_index++;
                LOG_DEBUG("CONCRETE_TOKEN_GROUP: Matched {} at index {}", 
                         static_cast<int>(token), token_idx);
                return true;
            }
            
            LOG_DEBUG("CONCRETE_TOKEN_GROUP: Token {} doesn't match any in group", static_cast<int>(token));
            return false;
        }
        
        case PatternElementType::REQUIRED_WHITESPACE:
        case PatternElementType::OPTIONAL_WHITESPACE:
            return matches_whitespace_pattern(element, instruction, clean_indices, current_index, result);
            
        case PatternElementType::NAMESPACED_IDENTIFIER:
            return matches_namespaced_identifier(element, instruction, clean_indices, current_index, result);
            
        case PatternElementType::END_OF_PATTERN:
            // END_OF_PATTERN should only match if we've consumed all tokens
            if (current_index >= clean_indices.size()) {
                LOG_DEBUG("END_OF_PATTERN: Successfully reached end of tokens");
                return true;
            }
            LOG_DEBUG("END_OF_PATTERN: Still have {} tokens remaining", 
                     clean_indices.size() - current_index);
            return false;
    }
    
    LOG_DEBUG("Unknown pattern element type: {}", static_cast<int>(element.type));
    return false;
}

bool ContextualizationPatternMatcher::matches_concrete_token(const PatternElement& element, EToken token) {
    return element.accepted_tokens.size() == 1 && element.accepted_tokens[0] == token;
}

bool ContextualizationPatternMatcher::matches_concrete_token_group(const PatternElement& element, EToken token) {
    for (EToken accepted : element.accepted_tokens) {
        if (accepted == token) {
            return true;
        }
    }
    return false;
}

bool ContextualizationPatternMatcher::matches_whitespace_pattern(
    const PatternElement& element,
    const Instruction& instruction,
    const std::vector<size_t>& clean_indices,
    size_t& current_index,
    ContextualTokenResult& result) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    // Check if current token is whitespace
    bool found_whitespace = false;
    std::vector<size_t> whitespace_indices;
    
    if (current_index < clean_indices.size()) {
        size_t token_idx = clean_indices[current_index];
        EToken token = instruction._tokens[token_idx]._token;
        
        // Check if it's a whitespace token
        if (token == EToken::SPACE || token == EToken::TAB || 
            token == EToken::NEWLINE || token == EToken::CARRIAGE_RETURN) {
            
            found_whitespace = true;
            whitespace_indices.push_back(token_idx);
            current_index++;
            
            LOG_DEBUG("WHITESPACE: Found whitespace {} at index {}", 
                     static_cast<int>(token), token_idx);
        }
    }
    
    if (element.type == PatternElementType::REQUIRED_WHITESPACE) {
        if (!found_whitespace) {
            LOG_DEBUG("REQUIRED_WHITESPACE: No whitespace found at current position");
            return false;
        }
        
        // Create contextual token for whitespace (if needed)
        if (element.target_contextual_token != EContextualToken::INVALID) {
            result = ContextualTokenResult(element.target_contextual_token, whitespace_indices);
        }
        
        LOG_DEBUG("REQUIRED_WHITESPACE: Successfully matched");
        return true;
    } else { // OPTIONAL_WHITESPACE
        // Optional whitespace always succeeds, even if no whitespace found
        if (found_whitespace && element.target_contextual_token != EContextualToken::INVALID) {
            result = ContextualTokenResult(element.target_contextual_token, whitespace_indices);
        }
        
        LOG_DEBUG("OPTIONAL_WHITESPACE: Successfully matched (found: {})", found_whitespace);
        return true;
    }
}

bool ContextualizationPatternMatcher::matches_namespaced_identifier(
    const PatternElement& element,
    const Instruction& instruction,
    const std::vector<size_t>& clean_indices,
    size_t& current_index,
    ContextualTokenResult& result) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    if (current_index >= clean_indices.size()) {
        LOG_DEBUG("NAMESPACED_IDENTIFIER: No more tokens available");
        return false;
    }
    
    std::vector<size_t> identifier_indices;
    size_t start_index = current_index;
    
    // Must start with an IDENTIFIER
    size_t token_idx = clean_indices[current_index];
    if (instruction._tokens[token_idx]._token != EToken::IDENTIFIER) {
        LOG_DEBUG("NAMESPACED_IDENTIFIER: Expected IDENTIFIER at start, got {}", 
                 static_cast<int>(instruction._tokens[token_idx]._token));
        return false;
    }
    
    identifier_indices.push_back(token_idx);
    current_index++;
    LOG_DEBUG("NAMESPACED_IDENTIFIER: Found base identifier at index {}", token_idx);
    
    // Look for optional namespace patterns: ::identifier
    while (current_index + 1 < clean_indices.size()) {
        size_t colon1_idx = clean_indices[current_index];
        size_t colon2_idx = clean_indices[current_index + 1];
        
        // Check for :: pattern
        if (instruction._tokens[colon1_idx]._token == EToken::COLON &&
            instruction._tokens[colon2_idx]._token == EToken::COLON) {
            
            // Look for identifier after ::
            if (current_index + 2 < clean_indices.size()) {
                size_t next_id_idx = clean_indices[current_index + 2];
                if (instruction._tokens[next_id_idx]._token == EToken::IDENTIFIER) {
                    // Found namespace::identifier pattern
                    identifier_indices.push_back(colon1_idx);  // First :
                    identifier_indices.push_back(colon2_idx);  // Second :
                    identifier_indices.push_back(next_id_idx); // Identifier
                    current_index += 3;
                    
                    LOG_DEBUG("NAMESPACED_IDENTIFIER: Found namespace resolution at indices {}, {}, {}", 
                             colon1_idx, colon2_idx, next_id_idx);
                    continue;
                }
            }
        }
        
        // No more namespace patterns found
        break;
    }
    
    if (identifier_indices.empty()) {
        LOG_DEBUG("NAMESPACED_IDENTIFIER: No identifiers found");
        current_index = start_index; // Reset on failure
        return false;
    }
    
    // Create contextual token result
    result = ContextualTokenResult(element.target_contextual_token, identifier_indices);
    
    LOG_DEBUG("NAMESPACED_IDENTIFIER: Successfully matched {} tokens", identifier_indices.size());
    return true;
}

// Initialize builtin patterns
void ContextualizationPatternMatcher::initialize_builtin_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    LOG_INFO("🏗️ Initializing builtin patterns");
    
    // Header Pattern 1: Class/Struct/Plex Definition
    // Pattern: OPTIONAL_WHITESPACE + CLASS|STRUCT|PLEX + REQUIRED_WHITESPACE + NAMESPACED_IDENTIFIER + OPTIONAL_WHITESPACE
    {
        std::vector<PatternElement> elements = {
            // Optional leading whitespace
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            PatternElement({EToken::CLASS, EToken::STRUCT, EToken::PLEX}, EContextualToken::TYPE_REFERENCE),
            PatternElement(PatternElementType::REQUIRED_WHITESPACE),
            PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::TYPE_REFERENCE),
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            PatternElement(PatternElementType::END_OF_PATTERN)
        };
        
        Pattern class_definition_pattern("class_definition", elements);
        register_header_pattern(class_definition_pattern);
        LOG_DEBUG("Registered header pattern: class_definition");
    }
    
    // Header Pattern 2: Simple Function Declaration  
    // Pattern: OPTIONAL_WHITESPACE + FUNC + REQUIRED_WHITESPACE + NAMESPACED_IDENTIFIER + OPTIONAL_WHITESPACE
    {
        std::vector<PatternElement> elements = {
            // Optional leading whitespace
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            PatternElement(EToken::FUNC, EContextualToken::FUNCTION_CALL),
            PatternElement(PatternElementType::REQUIRED_WHITESPACE),
            PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::FUNCTION_CALL),
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            PatternElement(PatternElementType::END_OF_PATTERN)
        };
        
        Pattern function_declaration_pattern("function_declaration", elements);
        register_header_pattern(function_declaration_pattern);
        LOG_DEBUG("Registered header pattern: function_declaration");
    }
    
    // Body Pattern 1: Variable Declaration with Assignment
    // Pattern: OPTIONAL_WHITESPACE + PRIMITIVE/IDENTIFIER + REQUIRED_WHITESPACE + IDENTIFIER + OPTIONAL_WHITESPACE + ASSIGN + OPTIONAL_WHITESPACE + IDENTIFIER/LITERAL + OPTIONAL_WHITESPACE
    {
        std::vector<PatternElement> elements = {
            // Optional leading whitespace
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            // Type (primitive keywords or identifiers that resolve to types)
            PatternElement({EToken::INT32_T, EToken::FLOAT, EToken::DOUBLE, EToken::BOOL, EToken::CHAR, EToken::VOID, EToken::IDENTIFIER}, EContextualToken::TYPE_REFERENCE),
            PatternElement(PatternElementType::REQUIRED_WHITESPACE),
            // Variable name
            PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::VARIABLE_DECLARATION),
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            // Assignment operator
            PatternElement(EToken::ASSIGN, EContextualToken::INVALID), // No contextual token for operators
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            // Value (identifier or literal)
            PatternElement({EToken::IDENTIFIER, EToken::INT_LITERAL, EToken::FLOAT_LITERAL, EToken::TRUE_LITERAL, EToken::FALSE_LITERAL, EToken::STRING_LITERAL, EToken::CHAR_LITERAL}, EContextualToken::VARIABLE_REFERENCE),
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            PatternElement(PatternElementType::END_OF_PATTERN)
        };
        
        Pattern variable_assignment_pattern("variable_assignment", elements);
        register_body_pattern(variable_assignment_pattern);
        LOG_DEBUG("Registered body pattern: variable_assignment");
    }
    
    // Body Pattern 2: Variable Declaration without Assignment  
    // Pattern: OPTIONAL_WHITESPACE + PRIMITIVE/IDENTIFIER + REQUIRED_WHITESPACE + IDENTIFIER + OPTIONAL_WHITESPACE
    {
        std::vector<PatternElement> elements = {
            // Optional leading whitespace
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            // Type (primitive keywords or identifiers that resolve to types)
            PatternElement({EToken::INT32_T, EToken::FLOAT, EToken::DOUBLE, EToken::BOOL, EToken::CHAR, EToken::VOID, EToken::IDENTIFIER}, EContextualToken::TYPE_REFERENCE),
            PatternElement(PatternElementType::REQUIRED_WHITESPACE),
            // Variable name
            PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::VARIABLE_DECLARATION),
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            PatternElement(PatternElementType::END_OF_PATTERN)
        };
        
        Pattern variable_declaration_pattern("variable_declaration", elements);
        register_body_pattern(variable_declaration_pattern);
        LOG_DEBUG("Registered body pattern: variable_declaration");
    }
    
    LOG_DEBUG("Builtin patterns initialization complete - {} header patterns, {} body patterns registered", 
              header_patterns_.size(), body_patterns_.size());
}

} // namespace cprime::layer2_contextualization