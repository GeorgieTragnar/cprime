// Clean CPrime - C++ compatible syntax
class Connection {
public:
    Connection() : ptr(nullptr) {}
    
private:
    int* ptr;
    
    danger {
        // Dangerous operations
        delete ptr;
    }
};

// Interface and runtime features (CPrime extensions)
interface UserOps {
    runtime exposes operations;
};

plex DataProcessor {
    defer cleanup();
    mut int counter = 0;
    
    // C++ style features
    constexpr auto getValue() noexcept -> decltype(counter) {
        return counter;
    }
    
    void processData() {
        try {
            int* data = new int[100];
            // Process data
            delete[] data;
        } catch (const std::exception& e) {
            // Handle exception
        }
    }
};