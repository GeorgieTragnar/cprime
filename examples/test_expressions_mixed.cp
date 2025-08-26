// Mixed Complexity Expression Tests
// Tests: All expression patterns combined with control flow

// Variable declarations with complex expressions
int base = 10;
int multiplier = 3;
int offset = 5;

// Complex assignments combining all patterns
int result1 = ((base + offset) * multiplier);
int result2 = (base * (multiplier + 2)) - ((offset * 2) + 1);
float ratio = (base + offset) / (multiplier + 1);

// Expressions with mixed literals and identifiers
int mixed1 = (base + 42) * (3 + offset);
int mixed2 = ((base * 2) + 100) / ((offset + 5) - 2);
bool condition = ((base > 5) && (offset < 10)) || (multiplier == 3);

// Control flow with complex expressions
// Note: These test if/while patterns use MANDATORY_EXPRESSION correctly