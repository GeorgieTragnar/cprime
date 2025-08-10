#pragma once

namespace cprime {

/**
 * Pure abstract Context class for extensible cache system.
 * 
 * Allows any layer to attach custom context data to Instructions and Scopes
 * without modifying core structures. Layers use dynamic_pointer_cast to
 * identify and access their specific context implementations.
 */
class Context {
public:
    virtual ~Context() = default;
};

} // namespace cprime