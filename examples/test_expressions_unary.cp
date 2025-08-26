// Unary Operator Expression Tests
// Tests: OPTIONAL_UNARY_OPERATOR pattern (!, +, -)

// Simple unary operators
bool negated = !true;
bool negatedFalse = !false;
int positive = +42;
int negative = -10;

// Unary with identifiers
int value = 15;
bool notValue = !true;
int negativeValue = -value;
int positiveValue = +value;

// Unary with complex expressions
bool complexNot = !(5 > 3);
int negativeSum = -(5 + 3);
int positiveSum = +(10 - 2);

// Nested unary operations
bool doubleNot = !!true;
int doubleNegative = --10;  // Note: This might be parsed as -- operator if available
int negativeExpression = -(5 * (3 + 2));

// Unary in complex contexts
bool unaryLogic = !((5 > 3) && (10 < 20));
int unaryArithmetic = -(((2 + 3) * 4) + 5);
bool mixedUnary = !(5 == 5) || (!(3 < 2));