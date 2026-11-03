// ----------------------------------------
// -- Lexer for acts --


// Tokens
/*
*   -- each token contains each of these 3 things
•	type: the label (NUMBER, PLUS, LET ...).
•	text: the exact characters it came from ("12"). The parser needs this to know *which* number or *which* variable name.
•	line: the line number it was found on. This is only for error messages, but adding it later is painful, so add it now.
*/



#include <iostream>
#include <string>
#include <vector>

// The labels a chunk can have
enum class TokenType {
    Number,
    Plus, Minus, Star, Slash,
    LeftParen, RightParen,
    Error,
    EndOfFile
};

// Token
// - 1. label
// - 2. original text
// - 3. The line it was found on
struct Token {
    TokenType type;
    std::string text;
    int line;
};

// Print the label into a printable name (DEBUGGING)
std::string typeName(TokenType t) {
    switch (t) {
    case TokenType::Number:     return "NUMBER";
    case TokenType::Plus:       return "PLUS";
    case TokenType::Minus:      return "MINUS";
    case TokenType::Star:       return "STAR";
    case TokenType::Slash:      return "SLASH";
    case TokenType::LeftParen:  return "LEFT_PAREN";
    case TokenType::RightParen: return "RIGHT_PAREN";
    case TokenType::Error:      return "ERROR";
    case TokenType::EndOfFile:  return "EOF";
    }
    return "?";
}

bool isDigit(char c) { return c >= '0' && c <= '9'; }


class Lexer {
private:

    // Source - an expression of code condenced into a single string.
    // The entire file
    std::string source_;

    // The position (index) in the entire source_ string, never resets to zero
    size_t pos_ = 0;

    // Current line number of the entire file (y-axis)
    int line_ = 1;


    //---------------------
    // - Helper Functions -

    // Are we at the end of the current expression?
    bool isAtEnd() const { return pos_ >= source_.size(); }

    // If we are at the end of an expression, return nothing
    // else, show what character is next. '==' is 2 '=', they're different
    char peek() const { return isAtEnd() ? '\0' : source_[pos_]; } // look, don't move

    // Returns the current value of pos, then afterwards, increments pos.
    // source = "cat' : pos = 0 -> advance() returns 'c' then increments pos = 1
    char advance() { return source_[pos_++]; } // return it, then move on

    // Makes a token --- [type, text] so line "TOKEN_NUMBER, 42"
    // also returns the line it is at
    Token make(TokenType type, const std::string& text) const { return { type, text, line_ }; }

    // This scans the file and figure out what each line does
    Token scanToken() {

        // Get the current position
        char c = advance();

        // Figure out which operation it is
        switch (c) {
        case '+': return make(TokenType::Plus, "+");
        case '-': return make(TokenType::Minus, "-");
        case '*': return make(TokenType::Star, "*");
        case '/': return make(TokenType::Slash, "/");
        case '(': return make(TokenType::LeftParen, "(");
        case ')': return make(TokenType::RightParen, ")");
        }

        // Say it is a digit
        if (isDigit(c)) return make(TokenType::Number, std::string(1, c));

        // If neither a digit or an operation, it is something bad, throw error
        return make(TokenType::Error, std::string("Unexpected character '") + c + "'");
    }



public:

    // The Constructor (Lexer) -> This is the setup function that runs
    //      the moment you create a new lexer.
    // & (reference) and const mean ->
    //      "Hey C++, don't make a heavy copy of this string in memory; 
    //      just point me to the original text safely without letting 
    //      me accidentally change it from the outside."
    // explicit -> Only use this constructor if the programmer explicitly asks for it.
    // : source_(source) (The Initializer List): 
    //      This is how C++ initializes member variables before the constructor even starts.
    //      It takes the source string you handed in from the outside and saves it
    //      into the private source_ variable we talked about earlier.
    //      The curly braces {} at the end are empty because all the setup work was
    //      already done by that initializer list!
    explicit Lexer(const std::string& source) : source_(source) {}

    // Function that digests the entire file all at once
    std::vector<Token> scanAll() {

        // A dynamic list or array. 
        // This function returns a complete list (std::vector) 
        // containing all the Token objects it discovers.
        std::vector<Token> tokens;

        // Scan the file until we reach the end
        while (!isAtEnd()) {

            // Takes that newly created token 
            // and adds it to the end of your list.
            tokens.push_back(scanToken());
        }

        // tacks on one final token: an EndOfFile token.
        tokens.push_back(make(TokenType::EndOfFile, ""));
        return tokens;
    }
};

// Takes the text from the command line and prints every token.
int main(int argc, char** argv) {

    // The source, the text we will be tokenizing, or a fallback if empy
    std::string source = (argc > 1) ? argv[1] : "1+2*(3-4)";

    // Run the Lexer
    Lexer lexer(source);

    // Print the output of the lexer
    for (const Token& t : lexer.scanAll()) {
        std::cout << "line " << t.line << "  " << typeName(t.type)
            << "  '" << t.text << "'\n";
    }
}
