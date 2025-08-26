// Complex pattern test with multiple optionals and repeatables

// Test pattern: [const] type [namespace::nested::] identifier [= expression] ;
const int std::vector::size_type my_var = 42;
int simple_var;
static float std::nested::complex_var = 3.14;
volatile double global_var;

// Test repeatable namespace patterns
my::deeply::nested::namespace::variable_name = 100;
std::experimental::filesystem::path file_path;

// Mixed complexity
const static int deeply::nested::final_var = 999;