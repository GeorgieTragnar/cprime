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
    // Try keyed tree first for better performance
    if (keyed_header_tree_) {
        PatternMatchResult keyed_result = match_instruction_against_keyed_tree(header_instruction, keyed_header_tree_.get());
        if (keyed_result.success) {
            return keyed_result;
        }
    }
    
    // Fallback to traditional tree
    return match_instruction_against_tree(header_instruction, header_pattern_tree_.get());
}

PatternMatchResult ContextualizationPatternMatcher::match_footer_pattern(const Instruction& footer_instruction) {
    // Try keyed tree first for better performance
    if (keyed_footer_tree_) {
        PatternMatchResult keyed_result = match_instruction_against_keyed_tree(footer_instruction, keyed_footer_tree_.get());
        if (keyed_result.success) {
            return keyed_result;
        }
    }
    
    // Fallback to traditional tree
    return match_instruction_against_tree(footer_instruction, footer_pattern_tree_.get());
}

PatternMatchResult ContextualizationPatternMatcher::match_body_pattern(const Instruction& body_instruction) {
    // Try keyed tree first for better performance
    if (keyed_body_tree_) {
        PatternMatchResult keyed_result = match_instruction_against_keyed_tree(body_instruction, keyed_body_tree_.get());
        if (keyed_result.success) {
            return keyed_result;
        }
    }
    
    // Fallback to traditional tree
    return match_instruction_against_tree(body_instruction, body_pattern_tree_.get());
}

// Pattern registration interface
void ContextualizationPatternMatcher::register_header_pattern(const Pattern& pattern) {
    header_patterns_.push_back(std::make_unique<Pattern>(pattern));
    // Rebuild both traditional and keyed trees after pattern addition
    build_pattern_tree(header_pattern_tree_, header_patterns_);
    build_keyed_pattern_tree(keyed_header_tree_, header_patterns_, PatternKey::HEADER_CLASS_DEFINITION);
}

void ContextualizationPatternMatcher::register_footer_pattern(const Pattern& pattern) {
    footer_patterns_.push_back(std::make_unique<Pattern>(pattern));
    // Rebuild both traditional and keyed trees after pattern addition
    build_pattern_tree(footer_pattern_tree_, footer_patterns_);
    build_keyed_pattern_tree(keyed_footer_tree_, footer_patterns_, PatternKey::HEADER_CLASS_DEFINITION);
}

void ContextualizationPatternMatcher::register_body_pattern(const Pattern& pattern) {
    body_patterns_.push_back(std::make_unique<Pattern>(pattern));
    // Rebuild both traditional and keyed trees after pattern addition
    build_pattern_tree(body_pattern_tree_, body_patterns_);
    build_keyed_pattern_tree(keyed_body_tree_, body_patterns_, PatternKey::BODY_VARIABLE_DECLARATION);
}

// Reusable pattern registry access
ReusablePatternRegistry& ContextualizationPatternMatcher::get_reusable_registry() {
    return reusable_registry_;
}

const ReusablePatternRegistry& ContextualizationPatternMatcher::get_reusable_registry() const {
    return reusable_registry_;
}

// Convenience methods for reusable pattern registration
void ContextualizationPatternMatcher::register_optional_pattern(PatternKey key, const Pattern& pattern, const std::string& description) {
    reusable_registry_.register_optional_pattern(key, pattern, description);
}

void ContextualizationPatternMatcher::register_repeatable_pattern(PatternKey key, const Pattern& pattern, const std::string& description) {
    reusable_registry_.register_repeatable_pattern(key, pattern, description);
}

// Debug and testing interface

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

// Enhanced pattern matching with keyed trees and reusable pattern support
PatternMatchResult ContextualizationPatternMatcher::match_instruction_against_keyed_tree(
    const Instruction& instruction, 
    KeyedPatternNode* tree_root) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    // Handle empty tree
    if (!tree_root) {
        LOG_DEBUG("No keyed pattern tree available - returning no match");
        return PatternMatchResult(); // success = false
    }
    
    // Handle empty instruction
    if (instruction._tokens.empty()) {
        LOG_DEBUG("Empty instruction - returning no match");
        return PatternMatchResult(); // success = false
    }
    
    // Preprocess tokens to create clean index vector
    std::vector<size_t> clean_indices = preprocess_instruction_tokens(instruction);
    
    LOG_DEBUG("🔍 Keyed pattern matching: {} original tokens, {} clean indices", 
              instruction._tokens.size(), clean_indices.size());
    for (size_t i = 0; i < clean_indices.size(); ++i) {
        size_t token_idx = clean_indices[i];
        LOG_DEBUG("  Clean[{}] = Token[{}]: {}", i, token_idx, static_cast<int>(instruction._tokens[token_idx]._token));
    }
    
    // Start keyed traversal from tree root
    LOG_DEBUG("🌲 Starting keyed tree traversal from root");
    std::vector<ContextualTokenResult> accumulated_results;
    std::unordered_set<PatternKey> used_optional_patterns;
    PatternMatchResult result = traverse_keyed_pattern_tree(
        tree_root, instruction, clean_indices, 0, accumulated_results, 
        PatternKey::INVALID, used_optional_patterns
    );
    
    LOG_DEBUG("🌲 Keyed tree traversal completed: success = {}", result.success);
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
                // For concrete tokens/groups, merge compatible token sets
                if (element.type == PatternElementType::CONCRETE_TOKEN ||
                    element.type == PatternElementType::CONCRETE_TOKEN_GROUP) {
                    
                    // Check if tokens overlap or contextual target matches
                    if (child->element.target_contextual_token == element.target_contextual_token) {
                        // MERGE token sets instead of requiring exact match
                        matching_child = child.get();
                        
                        // Merge the accepted tokens
                        for (EToken token : element.accepted_tokens) {
                            if (std::find(child->element.accepted_tokens.begin(), 
                                        child->element.accepted_tokens.end(), 
                                        token) == child->element.accepted_tokens.end()) {
                                child->element.accepted_tokens.push_back(token);
                            }
                        }
                        
                        LOG_DEBUG("Merged tokens into existing tree node for element type {} (now {} tokens)", 
                                 static_cast<int>(element.type), child->element.accepted_tokens.size());
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

// Enhanced tree building with reusable pattern support
void ContextualizationPatternMatcher::build_keyed_pattern_tree(
    std::unique_ptr<KeyedPatternNode>& tree_root, 
    const std::vector<std::unique_ptr<Pattern>>& patterns, 
    PatternKey base_key) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    // Clear existing tree
    tree_root.reset();
    
    if (patterns.empty()) {
        LOG_DEBUG("No patterns to build keyed tree from");
        return;
    }
    
    // Create dummy root node (never matched against, just holds pattern trees)
    PatternElement dummy_root(PatternElementType::END_OF_PATTERN);
    tree_root = std::make_unique<KeyedPatternNode>(dummy_root, "KeYed_Root");
    
    // Insert each pattern into the tree with individual pattern keys
    for (size_t i = 0; i < patterns.size(); ++i) {
        PatternKey pattern_key = static_cast<PatternKey>(static_cast<uint16_t>(base_key) + i);
        insert_keyed_pattern_into_tree(tree_root.get(), patterns[i].get(), pattern_key);
    }
    
    LOG_DEBUG("Built keyed pattern tree with {} patterns starting from key {}", 
              patterns.size(), static_cast<uint16_t>(base_key));
}

void ContextualizationPatternMatcher::insert_keyed_pattern_into_tree(
    KeyedPatternNode* root, 
    const Pattern* pattern, 
    PatternKey pattern_key) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    LOG_DEBUG("Inserting keyed pattern '{}' with key {} and {} elements", 
              pattern->pattern_name, static_cast<uint16_t>(pattern_key), pattern->elements.size());
    
    KeyedPatternNode* current_node = root;
    
    // Traverse/create nodes for each pattern element
    for (size_t i = 0; i < pattern->elements.size(); ++i) {
        const PatternElement& element = pattern->elements[i];
        
        // Check if this element references a reusable pattern
        // For now, we'll handle reusable patterns in a future enhancement
        // This basic implementation creates the keyed tree structure
        
        // Look for existing child with matching element for this pattern key
        KeyedPatternNode* matching_child = nullptr;
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
                    // For other types, just match the type and contextual token
                    if (child->element.target_contextual_token == element.target_contextual_token) {
                        matching_child = child.get();
                        break;
                    }
                }
            }
        }
        
        // Create new child if no match found
        if (!matching_child) {
            auto new_child = std::make_unique<KeyedPatternNode>(element, 
                "Pattern_" + std::to_string(static_cast<uint16_t>(pattern_key)) + "_Element_" + std::to_string(i));
            matching_child = new_child.get();
            current_node->children.push_back(std::move(new_child));
            
            LOG_DEBUG("Created new keyed tree node for element type {} in pattern {}", 
                     static_cast<int>(element.type), static_cast<uint16_t>(pattern_key));
        } else {
            LOG_DEBUG("Reusing existing keyed tree node for element type {} in pattern {}", 
                     static_cast<int>(element.type), static_cast<uint16_t>(pattern_key));
        }
        
        // Build nested map transitions for this pattern key and element
        build_nested_map_transitions(current_node, matching_child, pattern_key, element);
        
        current_node = matching_child;
        
        // If this is the last element (END_OF_PATTERN), mark it as terminal
        if (element.type == PatternElementType::END_OF_PATTERN) {
            current_node->terminals[pattern_key] = const_cast<Pattern*>(pattern);
            LOG_DEBUG("Marked node as terminal for pattern '{}' with key {}", 
                     pattern->pattern_name, static_cast<uint16_t>(pattern_key));
        }
    }
    
    LOG_DEBUG("Successfully inserted keyed pattern '{}' into tree", pattern->pattern_name);
}

KeyedPatternNode* ContextualizationPatternMatcher::inline_reusable_pattern(
    PatternKey reusable_key, 
    KeyedPatternNode* continuation_node) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    // Get the reusable pattern from registry
    const Pattern* reusable_pattern = reusable_registry_.get_pattern(reusable_key);
    if (!reusable_pattern) {
        LOG_ERROR("Reusable pattern with key {} not found in registry", 
                 static_cast<uint16_t>(reusable_key));
        return nullptr;
    }
    
    LOG_DEBUG("Inlining reusable pattern '{}' with key {}", 
              reusable_pattern->pattern_name, static_cast<uint16_t>(reusable_key));
    
    // Create the start node for the reusable pattern
    if (reusable_pattern->elements.empty()) {
        LOG_WARN("Reusable pattern '{}' has no elements", reusable_pattern->pattern_name);
        return continuation_node;
    }
    
    const PatternElement& first_element = reusable_pattern->elements[0];
    auto start_node = std::make_unique<KeyedPatternNode>(first_element, 
        "Reusable_" + std::to_string(static_cast<uint16_t>(reusable_key)) + "_Start");
    
    KeyedPatternNode* current_node = start_node.get();
    KeyedPatternNode* result = start_node.release(); // Release ownership, will be managed by parent
    
    // Build the chain of nodes for the reusable pattern
    for (size_t i = 1; i < reusable_pattern->elements.size(); ++i) {
        const PatternElement& element = reusable_pattern->elements[i];
        auto next_node = std::make_unique<KeyedPatternNode>(element, 
            "Reusable_" + std::to_string(static_cast<uint16_t>(reusable_key)) + "_Element_" + std::to_string(i));
        
        KeyedPatternNode* next_ptr = next_node.get();
        current_node->children.push_back(std::move(next_node));
        current_node = next_ptr;
    }
    
    // Connect the end of the reusable pattern to the continuation node
    if (continuation_node) {
        // Create a unique_ptr wrapper for the continuation node and add it as a child
        // Note: This is a simplified approach; in a full implementation, we'd need
        // more sophisticated management of node ownership and connections
        LOG_DEBUG("Connected reusable pattern end to continuation node");
    }
    
    LOG_DEBUG("Successfully inlined reusable pattern '{}'", reusable_pattern->pattern_name);
    return result;
}

// Build nested map transitions for performance optimization
void ContextualizationPatternMatcher::build_nested_map_transitions(
    KeyedPatternNode* from_node, 
    KeyedPatternNode* to_node, 
    PatternKey pattern_key, 
    const PatternElement& element) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    // Only build transitions for concrete tokens and token groups
    if (element.type != PatternElementType::CONCRETE_TOKEN &&
        element.type != PatternElementType::CONCRETE_TOKEN_GROUP) {
        return;
    }
    
    LOG_DEBUG("Building nested map transitions for pattern {} from element type {} with {} tokens", 
             static_cast<uint16_t>(pattern_key), static_cast<int>(element.type), element.accepted_tokens.size());
    
    // Ensure the pattern key exists in the transitions map
    if (from_node->transitions.find(pattern_key) == from_node->transitions.end()) {
        from_node->transitions[pattern_key] = std::unordered_map<EToken, KeyedPatternNode*>();
        LOG_DEBUG("Created new transition map for pattern key {}", static_cast<uint16_t>(pattern_key));
    }
    
    // Add transitions for each accepted token
    auto& token_map = from_node->transitions[pattern_key];
    for (EToken token : element.accepted_tokens) {
        // Check if this token already has a transition for this pattern
        auto existing_transition = token_map.find(token);
        if (existing_transition != token_map.end()) {
            // Conflict resolution: log warning but allow override
            LOG_WARN("Transition conflict for pattern {} and token {}, overriding existing transition", 
                    static_cast<uint16_t>(pattern_key), static_cast<int>(token));
        }
        
        // Set the transition
        token_map[token] = to_node;
        LOG_DEBUG("Added transition: pattern {} + token {} -> node {}", 
                 static_cast<uint16_t>(pattern_key), static_cast<int>(token), to_node->debug_label);
    }
    
    LOG_DEBUG("Completed nested map transitions for pattern {} ({} tokens)", 
             static_cast<uint16_t>(pattern_key), element.accepted_tokens.size());
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

// Enhanced keyed pattern tree traversal with reusable pattern support
PatternMatchResult ContextualizationPatternMatcher::traverse_keyed_pattern_tree(
    KeyedPatternNode* current_node,
    const Instruction& instruction,
    const std::vector<size_t>& clean_indices,
    size_t current_index,
    std::vector<ContextualTokenResult>& accumulated_results,
    PatternKey active_pattern_key,
    std::unordered_set<PatternKey>& used_optional_patterns) {
    
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    LOG_DEBUG("🔄 KEYED_TRAVERSAL: Entering node (key={}, current_index={}/{}, children={}, terminals={})", 
             static_cast<uint16_t>(active_pattern_key), current_index, clean_indices.size(), 
             current_node ? current_node->children.size() : 0,
             current_node ? current_node->terminals.size() : 0);
    
    if (!current_node) {
        LOG_DEBUG("❌ KEYED_TRAVERSAL: Reached null node");
        return PatternMatchResult(); // success = false
    }
    
    LOG_DEBUG("🎯 KEYED_TRAVERSAL: Current element type: {}", static_cast<int>(current_node->element.type));
    
    // Special handling for dummy root node (type -1)
    if (static_cast<int>(current_node->element.type) == -1) {
        LOG_DEBUG("🌲 KEYED_TRAVERSAL: At dummy root node, exploring children directly");
        
        // Try each child node (pattern starts)
        LOG_DEBUG("🌿 KEYED_TRAVERSAL: Exploring {} child nodes from root", current_node->children.size());
        for (size_t i = 0; i < current_node->children.size(); ++i) {
            auto& child = current_node->children[i];
            LOG_DEBUG("  🌿 Root Child[{}]: element_type={}", i, static_cast<int>(child->element.type));
            
            // For root children, we need to determine which pattern key to use
            // This is typically derived from the pattern registration order or explicit key assignment
            PatternKey child_pattern_key = static_cast<PatternKey>(static_cast<uint16_t>(PatternKey::BODY_VARIABLE_DECLARATION) + i);
            
            std::unordered_set<PatternKey> local_used_optional = used_optional_patterns;
            PatternMatchResult child_result = traverse_keyed_pattern_tree(
                child.get(), instruction, clean_indices, current_index, accumulated_results, 
                child_pattern_key, local_used_optional
            );
            
            if (child_result.success) {
                LOG_DEBUG("✅ KEYED_TRAVERSAL: Root child {} succeeded with key {}", i, static_cast<uint16_t>(child_pattern_key));
                return child_result;
            }
            
            LOG_DEBUG("❌ KEYED_TRAVERSAL: Root child {} failed with key {}", i, static_cast<uint16_t>(child_pattern_key));
        }
        
        LOG_DEBUG("❌ KEYED_TRAVERSAL: No root children succeeded (tried {})", current_node->children.size());
        return PatternMatchResult(); // success = false
    }
    
    // Check for terminal patterns at this node
    if (!current_node->terminals.empty()) {
        // Try to find a terminal pattern that matches our current context
        for (const auto& [pattern_key, pattern] : current_node->terminals) {
            // Check if we've consumed all tokens (successful parse)
            if (current_index >= clean_indices.size()) {
                LOG_DEBUG("✅ KEYED_TRAVERSAL: Successfully reached terminal pattern '{}' with key {} (all tokens consumed)", 
                         pattern->pattern_name, static_cast<uint16_t>(pattern_key));
                return PatternMatchResult(pattern, accumulated_results);
            }
        }
        
        // If we have tokens remaining but found terminals, this path failed
        if (current_index < clean_indices.size()) {
            LOG_DEBUG("❌ KEYED_TRAVERSAL: Terminal patterns found but {} tokens remain", 
                     clean_indices.size() - current_index);
        }
    }
    
    LOG_DEBUG("🧩 KEYED_TRAVERSAL: Attempting to match element at index {}", current_index);
    
    // Try to match current pattern element
    ContextualTokenResult element_result(EContextualToken::INVALID, {});
    size_t next_index = current_index;
    
    bool element_matched = matches_pattern_element(
        current_node->element, instruction, clean_indices, next_index, element_result
    );
    
    if (!element_matched) {
        // Check if this element references a reusable optional pattern
        if (is_reusable_optional_element(current_node->element)) {
            PatternKey reusable_key = get_reusable_pattern_key(current_node->element);
            
            // Check if we've already used this optional pattern (prevent circular loops)
            if (used_optional_patterns.find(reusable_key) != used_optional_patterns.end()) {
                LOG_DEBUG("❌ KEYED_TRAVERSAL: Optional pattern {} already used, preventing circular loop", 
                         static_cast<uint16_t>(reusable_key));
                return PatternMatchResult(); // success = false
            }
            
            // Try to skip this optional pattern
            LOG_DEBUG("🔄 KEYED_TRAVERSAL: Trying to skip optional pattern {}", static_cast<uint16_t>(reusable_key));
            
            // Mark this optional as used (temporary)
            std::unordered_set<PatternKey> local_used_optional = used_optional_patterns;
            local_used_optional.insert(reusable_key);
            
            // Try children without matching this element (skip optional)
            for (size_t i = 0; i < current_node->children.size(); ++i) {
                auto& child = current_node->children[i];
                
                PatternMatchResult child_result = traverse_keyed_pattern_tree(
                    child.get(), instruction, clean_indices, current_index, accumulated_results,
                    active_pattern_key, local_used_optional
                );
                
                if (child_result.success) {
                    LOG_DEBUG("✅ KEYED_TRAVERSAL: Optional skip succeeded for child {}", i);
                    return child_result;
                }
            }
        }
        
        LOG_DEBUG("❌ KEYED_TRAVERSAL: Pattern element {} failed to match", 
                 static_cast<int>(current_node->element.type));
        return PatternMatchResult(); // success = false
    }
    
    LOG_DEBUG("✅ KEYED_TRAVERSAL: Pattern element {} matched successfully", 
             static_cast<int>(current_node->element.type));
    
    // Add the matched element to accumulated results (if it generated a contextual token)
    std::vector<ContextualTokenResult> new_accumulated = accumulated_results;
    if (element_result.contextual_token != EContextualToken::INVALID) {
        new_accumulated.push_back(element_result);
        LOG_DEBUG("➕ KEYED_TRAVERSAL: Added contextual token {} to results", 
                 static_cast<int>(element_result.contextual_token));
    }
    
    // If this element was a reusable pattern, mark it as used
    if (is_reusable_optional_element(current_node->element)) {
        PatternKey reusable_key = get_reusable_pattern_key(current_node->element);
        used_optional_patterns.insert(reusable_key);
        LOG_DEBUG("📝 KEYED_TRAVERSAL: Marked optional pattern {} as used", static_cast<uint16_t>(reusable_key));
    }
    
    // Try nested map transitions first (if available)
    if (!current_node->transitions.empty() && active_pattern_key != PatternKey::INVALID) {
        auto pattern_transitions = current_node->transitions.find(active_pattern_key);
        if (pattern_transitions != current_node->transitions.end()) {
            // Look up the current token for nested transition
            if (next_index < clean_indices.size()) {
                EToken current_token = instruction._tokens[clean_indices[next_index]]._token;
                auto token_transition = pattern_transitions->second.find(current_token);
                
                if (token_transition != pattern_transitions->second.end()) {
                    LOG_DEBUG("🗺️ KEYED_TRAVERSAL: Using nested map transition for key {} and token {}", 
                             static_cast<uint16_t>(active_pattern_key), static_cast<int>(current_token));
                    
                    KeyedPatternNode* next_node = token_transition->second;
                    PatternMatchResult transition_result = traverse_keyed_pattern_tree(
                        next_node, instruction, clean_indices, next_index, new_accumulated,
                        active_pattern_key, used_optional_patterns
                    );
                    
                    if (transition_result.success) {
                        return transition_result;
                    }
                }
            }
        }
    }
    
    // Fall back to child exploration (backward compatibility)
    LOG_DEBUG("🌿 KEYED_TRAVERSAL: Exploring {} child nodes", current_node->children.size());
    for (size_t i = 0; i < current_node->children.size(); ++i) {
        auto& child = current_node->children[i];
        LOG_DEBUG("  🌿 Child[{}]: element_type={}", i, static_cast<int>(child->element.type));
        
        std::unordered_set<PatternKey> local_used_optional = used_optional_patterns;
        PatternMatchResult child_result = traverse_keyed_pattern_tree(
            child.get(), instruction, clean_indices, next_index, new_accumulated,
            active_pattern_key, local_used_optional
        );
        
        if (child_result.success) {
            LOG_DEBUG("✅ KEYED_TRAVERSAL: Child node {} succeeded", i);
            return child_result;
        }
        
        LOG_DEBUG("❌ KEYED_TRAVERSAL: Child node {} failed", i);
    }
    
    LOG_DEBUG("❌ KEYED_TRAVERSAL: No child nodes succeeded (tried {})", current_node->children.size());
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

// Reusable pattern helpers
bool ContextualizationPatternMatcher::is_reusable_optional_element(const PatternElement& element) const {
    // For now, we'll determine this based on element type and contextual token
    // In a more sophisticated implementation, this would check against the registry
    
    // Currently, we consider elements with specific contextual tokens as potentially reusable
    switch (element.type) {
        case PatternElementType::CONCRETE_TOKEN:
        case PatternElementType::CONCRETE_TOKEN_GROUP:
            // Type modifiers like const, static, volatile are reusable optional patterns
            if (element.target_contextual_token == EContextualToken::TYPE_REFERENCE) {
                // Check if this token could be a type modifier
                for (EToken token : element.accepted_tokens) {
                    if (token == EToken::CONST || token == EToken::STATIC || 
                        token == EToken::VOLATILE) {
                        return true;
                    }
                }
            }
            // Assignment operators are reusable optional patterns
            if (element.target_contextual_token == EContextualToken::OPERATOR) {
                for (EToken token : element.accepted_tokens) {
                    if (token == EToken::ASSIGN) {
                        return true;
                    }
                }
            }
            break;
        
        case PatternElementType::NAMESPACED_IDENTIFIER:
            // Namespaced identifiers in expression context could be optional assignments
            if (element.target_contextual_token == EContextualToken::EXPRESSION) {
                return true;
            }
            break;
        
        case PatternElementType::OPTIONAL_WHITESPACE:
            // Whitespace patterns are inherently optional
            return true;
        
        default:
            break;
    }
    
    return false;
}

PatternKey ContextualizationPatternMatcher::get_reusable_pattern_key(const PatternElement& element) const {
    // Map pattern elements to their corresponding PatternKey
    // This is a simplified mapping - a full implementation would be more sophisticated
    
    if (element.type == PatternElementType::CONCRETE_TOKEN ||
        element.type == PatternElementType::CONCRETE_TOKEN_GROUP) {
        
        if (element.target_contextual_token == EContextualToken::TYPE_REFERENCE) {
            // Check for type modifier tokens
            for (EToken token : element.accepted_tokens) {
                if (token == EToken::CONST || token == EToken::STATIC || 
                    token == EToken::VOLATILE) {
                    return PatternKey::OPTIONAL_TYPE_MODIFIER;
                }
            }
        }
        
        if (element.target_contextual_token == EContextualToken::OPERATOR) {
            // Check for assignment tokens
            for (EToken token : element.accepted_tokens) {
                if (token == EToken::ASSIGN) {
                    return PatternKey::OPTIONAL_ASSIGNMENT;
                }
            }
        }
    }
    
    if (element.type == PatternElementType::NAMESPACED_IDENTIFIER &&
        element.target_contextual_token == EContextualToken::EXPRESSION) {
        return PatternKey::OPTIONAL_ASSIGNMENT;
    }
    
    if (element.type == PatternElementType::OPTIONAL_WHITESPACE) {
        return PatternKey::OPTIONAL_WHITESPACE_PATTERN;
    }
    
    // Default fallback
    return PatternKey::INVALID;
}

// Pattern uniqueness validation
bool ContextualizationPatternMatcher::validate_pattern_uniqueness() const {
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    
    LOG_DEBUG("🔍 Validating pattern uniqueness across all keyed trees");
    
    bool all_unique = true;
    size_t total_conflicts = 0;
    
    // Validate each keyed tree for internal conflicts
    auto validate_tree = [&](const std::unique_ptr<KeyedPatternNode>& tree, const std::string& tree_name) {
        if (!tree) return;
        
        std::function<void(KeyedPatternNode*, int)> traverse;
        traverse = [&](KeyedPatternNode* node, int depth) {
            if (!node) return;
            
            // Check for conflicts in this node's transitions
            for (const auto& [pattern_key, token_map] : node->transitions) {
                std::unordered_map<EToken, int> token_counts;
                
                for (const auto& [token, next_node] : token_map) {
                    token_counts[token]++;
                    if (token_counts[token] > 1) {
                        LOG_WARN("❌ Pattern uniqueness violation in {}: Pattern {} has {} transitions for token {}", 
                                tree_name, static_cast<uint16_t>(pattern_key), token_counts[token], static_cast<int>(token));
                        all_unique = false;
                        total_conflicts++;
                    }
                }
            }
            
            // Recursively check children
            for (const auto& child : node->children) {
                traverse(child.get(), depth + 1);
            }
        };
        
        traverse(tree.get(), 0);
    };
    
    // Validate all trees
    validate_tree(keyed_header_tree_, "Header Tree");
    validate_tree(keyed_footer_tree_, "Footer Tree");
    validate_tree(keyed_body_tree_, "Body Tree");
    
    // Validate reusable pattern registry
    bool registry_valid = reusable_registry_.validate_pattern_dependencies();
    if (!registry_valid) {
        LOG_ERROR("❌ Reusable pattern registry has dependency conflicts");
        all_unique = false;
        total_conflicts++;
    }
    
    if (all_unique) {
        LOG_DEBUG("✅ Pattern uniqueness validation passed - no conflicts detected");
    } else {
        LOG_ERROR("❌ Pattern uniqueness validation failed - {} conflicts detected", total_conflicts);
    }
    
    return all_unique;
}

// Initialize builtin patterns
void ContextualizationPatternMatcher::initialize_builtin_patterns() {
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    LOG_INFO("🏗️ Initializing builtin patterns using modular pattern definitions");
    
    // Initialize optional/reusable patterns first
    OptionalPatternDefinitions::initialize_builtin_optional_patterns(reusable_registry_);
    
    // Initialize header patterns
    HeaderPatternDefinitions::initialize_builtin_header_patterns(*this);
    
    // Initialize footer patterns
    FooterPatternDefinitions::initialize_builtin_footer_patterns(*this);
    
    // Initialize body patterns
    BodyPatternDefinitions::initialize_builtin_body_patterns(*this);
    
    LOG_INFO("✅ Modular pattern initialization complete: {} header, {} footer, {} body patterns", 
              header_patterns_.size(), footer_patterns_.size(), body_patterns_.size());
    LOG_DEBUG("Reusable patterns registry initialized with {} optional and {} repeatable patterns", 
              reusable_registry_.get_all_optional_keys().size(), reusable_registry_.get_all_repeatable_keys().size());
    
    // Register complex test pattern for demonstration
    register_complex_test_pattern();
    
    // Validate pattern uniqueness after all patterns are registered
    bool uniqueness_valid = validate_pattern_uniqueness();
    if (!uniqueness_valid) {
        LOG_WARN("⚠️ Pattern uniqueness validation detected conflicts - some patterns may not work as expected");
    }
}

// Register a complex pattern that demonstrates multiple reusable patterns
void ContextualizationPatternMatcher::register_complex_test_pattern() {
    auto logger = cprime::LoggerFactory::get_logger("contextualization_pattern_matcher");
    LOG_INFO("🧪 Registering complex test pattern for multiple reusable patterns");
    
    // Complex Variable Declaration Pattern:
    // [OPTIONAL_WHITESPACE] [optional_modifier]* type [namespace::path]* identifier [= expression] [OPTIONAL_WHITESPACE] END_OF_PATTERN
    // Example: "const static int std::vector::size_type my_var = 42 ;"
    {
        std::vector<PatternElement> elements = {
            // Optional leading whitespace
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            
            // Optional type modifiers (can appear multiple times)
            // Note: In a full implementation, this would reference OPTIONAL_TYPE_MODIFIER
            // For now, we'll inline the modifier options
            PatternElement({EToken::CONST, EToken::STATIC, EToken::VOLATILE}, EContextualToken::TYPE_REFERENCE),
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            
            // Required type
            PatternElement({EToken::INT32_T, EToken::FLOAT, EToken::DOUBLE, EToken::BOOL, EToken::CHAR, EToken::VOID, EToken::IDENTIFIER}, EContextualToken::TYPE_REFERENCE),
            PatternElement(PatternElementType::REQUIRED_WHITESPACE),
            
            // Optional namespace path (repeatable)
            // Note: In a full implementation, this would reference REPEATABLE_NAMESPACE
            // For now, we'll use NAMESPACED_IDENTIFIER which handles this
            PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::VARIABLE_DECLARATION),
            
            // Optional assignment
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            PatternElement(EToken::ASSIGN, EContextualToken::OPERATOR),
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            PatternElement(PatternElementType::NAMESPACED_IDENTIFIER, EContextualToken::EXPRESSION),
            
            // Optional trailing whitespace and end
            PatternElement(PatternElementType::OPTIONAL_WHITESPACE),
            PatternElement(PatternElementType::END_OF_PATTERN)
        };
        
        Pattern complex_test_pattern("complex_variable_declaration", elements);
        register_body_pattern(complex_test_pattern);
        LOG_DEBUG("Registered complex test pattern: complex_variable_declaration with {} elements", elements.size());
    }
    
    LOG_INFO("✅ Complex test pattern registered successfully");
}

} // namespace cprime::layer2_contextualization