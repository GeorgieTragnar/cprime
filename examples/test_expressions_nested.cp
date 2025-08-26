// Nested Recursive Expression Tests
// Tests: Deep recursion with multiple MANDATORY_EXPRESSION levels

// Double nested parentheses
int doubleNested = ((5 + 3));
int tripleNested = (((10 - 2)));

// Complex arithmetic nesting
int complex1 = ((2 + 3) * (4 + 1));
int complex2 = ((10 - 2) / (3 + 1));
int complex3 = ((5 * 2) + (8 / 2));

// Mixed operations with deep nesting
int deep1 = (((2 + 3) * 4) - (5 + 1));
int deep2 = (((10 / 2) + 3) * ((6 - 2) + 1));

// Comparison with nested expressions
bool nestedComp1 = ((5 + 2) > (3 + 3));
bool nestedComp2 = ((10 - 4) == (2 * 3));
bool nestedComp3 = ((8 / 2) < (3 + 2));

// Logical operations with nested expressions
bool nestedLogic1 = ((5 > 3) && (7 < 10));
bool nestedLogic2 = ((2 == 2) || (4 != 4));
bool nestedLogic3 = (((5 + 1) > 4) && ((10 - 2) == 8));

// Very deep nesting
int veryDeep = ((((2 + 1) * 2) + ((3 * 2) - 1)) / (((4 + 1) - 2) + 1));