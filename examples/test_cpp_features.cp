// Modern C++ features test
class MyClass {
public:
    constexpr auto getValue() noexcept -> decltype(value) {
        return value;
    }
    
    void testFeatures() {
        // Memory management
        int* ptr = new int[10];
        delete[] ptr;
        
        // Exception handling
        try {
            throw std::runtime_error("test");
        } catch (const std::exception& e) {
            // Handle exception
        }
        
        // Conditional operator
        int result = condition ? 42 : 0;
        
        // Comma operator
        int a = 1, b = 2, c = 3;
        
        // Type queries
        static_assert(sizeof(int) == alignof(int));
        auto type = decltype(value);
    }
    
private:
    int value = 0;
};