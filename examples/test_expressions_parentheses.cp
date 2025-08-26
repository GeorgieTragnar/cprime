// Parenthesized Expression Tests  
// Tests: OPTIONAL_PARENTHESIZED pattern with proper operator tokens

// Simple parentheses
int simple = (5);
int basic = (42);

// Parentheses with binary operations
int groupedAdd = (3 + 4);
int groupedSub = (10 - 6);
int groupedMul = (2 * 8);

// Precedence control with parentheses
int precedence1 = (5 + 3) * 2;
int precedence2 = 5 + (3 * 2);
int precedence3 = (10 - 4) / (3 + 1);

// Comparison with parentheses
bool comparison1 = (5 > 3);
bool comparison2 = (10 == 10);
bool comparison3 = (7 < 12);

// Logical operations with parentheses
bool logical1 = (true && false);
bool logical2 = (true || false);
bool logical3 = (5 > 3) && (7 < 10);
bool logical4 = (2 == 2) || (4 != 5);