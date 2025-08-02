# Future Compiler Warnings for CPrime

This document outlines warnings that should be implemented in the CPrime compiler once we have a proper warning system in place.

## Constructor/Destructor Consistency Warnings

### Warning: Constructible but Non-Destructible Objects

**Trigger Condition:** If any constructor is defined (either `= default` or custom implementation) but the destructor is deleted (either explicitly with `= delete` or implicitly by not declaring it).

**Rationale:** This creates objects that can be constructed but never properly destroyed, which is almost certainly a logic error. Users may not realize that CPrime implicitly deletes all special members unless explicitly declared.

**Examples:**

```cprime
// WARNING: Should emit warning
class ProblematicClass {
    x: int,
    
    ProblematicClass() = default;  // Constructor defined
    // Destructor implicitly deleted - WARNING!
};

// WARNING: Should emit warning  
class ExplicitlyProblematic {
    value: int,
    
    ExplicitlyProblematic() = default;  // Constructor defined
    ~ExplicitlyProblematic() = delete; // Explicitly deleted destructor - WARNING!
};

// OK: No constructors defined
class DataOnly {
    data: int,
    // All special members implicitly deleted - no warning
};

// OK: Complete definition
class Proper {
    value: int,
    
    Proper() = default;
    ~Proper() = default;  // Destructor explicitly defined - no warning
};
```

**Suggested Warning Message:**
```
Warning: Class 'ClassName' defines constructors but has no accessible destructor. 
Objects can be created but not destroyed. Did you forget to declare a destructor?
```

## Implementation Notes

- This warning helps users understand CPrime's implicit deletion behavior
- Particularly useful for developers transitioning from C++ where destructors are implicitly generated
- The warning should be suppressible for advanced use cases where this behavior is intentional
- Consider making this warning enabled by default but allow `-Wno-constructor-destructor-mismatch` to disable it

## Future Extensions

As the compiler evolves, additional warnings might include:
- Unused special member functions
- Potentially dangerous cast operations
- Resource management patterns that might benefit from RAII