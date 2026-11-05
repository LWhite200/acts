#pragma once
#pragma once
// ----------------------------------------
// -- Lexer for acts --


// Tokens
/*
*   -- each token contains each of these 3 things
•	type: the label (NUMBER, PLUS, LET ...).
•	text: the exact characters it came from ("12"). The parser needs this to know *which* number or *which* variable name.
•	line: the line number it was found on. This is only for error messages, but adding it later is painful, so add it now.
*/

// Basic c++ includes
#include <iostream>
#include <string>
#include <vector>

// File reading includes
#include <cstdlib>
#include <fstream>
#include <sstream>

// map
#include <unordered_map> 


// The labels a chunk can have
enum class TokenType {
    Number,
    Plus, Minus, Star, Slash,
    LeftParen, RightParen,
    Error,
    EndOfFile,
    Percent,
    Identifier,
    Let, Print, If, Else, While, True, False,
    Fn, Return, Nil, And, Or, Not,  // Capitalized to avoid C++ keyword clashes
    Equal, EqualEqual, Bang, BangEqual,
    Less, LessEqual, Greater, GreaterEqual,
    LeftBrace, RightBrace, Comma,

    LeftBracket, RightBracket, Semicolon,

    String

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
    case TokenType::Percent:    return "PRECENT";
    case TokenType::Identifier: return "IDENTIFIER";

    case TokenType::Let:        return "LET";
    case TokenType::Print:      return "PRINT";
    case TokenType::If:         return "IF";
    case TokenType::Else:       return "ELSE";
    case TokenType::While:      return "WHILE";
    case TokenType::True:       return "TRUE";
    case TokenType::False:      return "FALSE";

    case TokenType::Fn:     return "FN";
    case TokenType::Return: return "RETURN";
    case TokenType::Nil:    return "NIL";
    case TokenType::And:    return "AND";
    case TokenType::Or:     return "OR";
    case TokenType::Not:    return "NOT";

    case TokenType::Equal:        return "EQUAL";
    case TokenType::EqualEqual:   return "EQUAL_EQUAL";
    case TokenType::Bang:         return "BANG";
    case TokenType::BangEqual:    return "BANG_EQUAL";
    case TokenType::Less:         return "LESS";
    case TokenType::LessEqual:    return "LESS_EQUAL";
    case TokenType::Greater:      return "GREATER";
    case TokenType::GreaterEqual: return "GREATER_EQUAL";
    case TokenType::LeftBrace:    return "LEFT_BRACE";
    case TokenType::RightBrace:   return "RIGHT_BRACE";
    case TokenType::Comma:        return "COMMA";

    case TokenType::LeftBracket:  return "LEFT_BRACKET";
    case TokenType::RightBracket: return "RIGHT_BRACKET";
    case TokenType::Semicolon:    return "SEMICOLON";

    case TokenType::String:     return "STRING";
        // fn, return, print, nil, and, or, not
    }
    return "?";
}

bool isDigit(char c) { return c >= '0' && c <= '9'; }

// [6]
bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool isAlphaNumeric(char c) { return isAlpha(c) || isDigit(c); }

// [6] This is for reserved keywords
const std::unordered_map<std::string, TokenType> keywords = {
    {"let",    TokenType::Let},
    {"print",  TokenType::Print},
    {"if",     TokenType::If},
    {"else",   TokenType::Else},
    {"while",  TokenType::While},
    {"true",   TokenType::True},
    {"false",  TokenType::False},
    {"fn",     TokenType::Fn},
    {"return", TokenType::Return},
    {"nil",    TokenType::Nil},
    {"and",    TokenType::And},
    {"or",     TokenType::Or},
    {"not",    TokenType::Not},
};



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

    // [5] Looks two ahead. We need it to avoid a trap with the dot '3.5'.
    char peekNext() const { return pos_ + 1 >= source_.size() ? '\0' : source_[pos_ + 1]; }

    // Returns the current value of pos, then afterwards, increments pos.
    // source = "cat' : pos = 0 -> advance() returns 'c' then increments pos = 1
    char advance() { return source_[pos_++]; } // return it, then move on


    // checks whether the NEXT character is the one we hope for, and if so eats it. Add it next to advance().
    bool match(char expected) {
        if (isAtEnd() || source_[pos_] != expected) return false;
        pos_++;
        return true;
    }



    // Makes a token --- [type, text] so line "TOKEN_NUMBER, 42"
    // also returns the line it is at
    Token make(TokenType type, const std::string& text) const { return { type, text, line_ }; }

    Token scanString() {
        int startLine = line_;
        size_t start = pos_;  // first character after the opening quote
        while (!isAtEnd() && peek() != '"') {
            if (peek() == '\n') line_++;
            advance();
        }
        if (isAtEnd()) return { TokenType::Error, "Unterminated string", startLine };
        std::string text = source_.substr(start, pos_ - start);
        advance();  // eat the closing quote
        return { TokenType::String, text, startLine };
    }



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
        case '%': return make(TokenType::Percent, "%");
        case '{': return make(TokenType::LeftBrace, "{");
        case '}': return make(TokenType::RightBrace, "}");
        case '[': return make(TokenType::LeftBracket, "[");
        case ']': return make(TokenType::RightBracket, "]");
        case ',': return make(TokenType::Comma, ",");
        case ';': return make(TokenType::Semicolon, ";");
        case '=':
            return match('=') ? make(TokenType::EqualEqual, "==")
                : make(TokenType::Equal, "=");
        case '!':
            return match('=') ? make(TokenType::BangEqual, "!=")
                : make(TokenType::Error, "Unexpected character '!'"); // Fixed: Lone ! is an error!
        case '<':
            return match('=') ? make(TokenType::LessEqual, "<=")
                : make(TokenType::Less, "<");
        case '>':
            return match('=') ? make(TokenType::GreaterEqual, ">=")
                : make(TokenType::Greater, ">");

        case '"': return scanString();
        }

        // --- INSERT IT RIGHT HERE ---
        if (isAlpha(c)) {
            size_t start = pos_ - 1;
            while (isAlphaNumeric(peek())) advance();
            std::string word = source_.substr(start, pos_ - start);
            auto it = keywords.find(word);
            TokenType type = (it != keywords.end()) ? it->second : TokenType::Identifier;
            return make(type, word);
        }

        // [5] Say it is a digit
        if (isDigit(c)) {

            // We've encountered the start of a number literal
            size_t start = pos_ - 1;

            // Consume all consecutive digits for the integer portion
            while (isDigit(peek())) {
                advance();
            }

            // Check for a fractional component (e.g., 3.14)
            // A dot only qualifies as a decimal point if it is immediately followed by a digit
            if (peek() == '.' && isDigit(peekNext())) {
                advance(); // Consume the decimal point ('.')

                // Consume all consecutive digits for the fractional portion
                while (isDigit(peek())) {
                    advance();
                }
            }

            // Extract the full number substring from the source and create the token
            return make(TokenType::Number, source_.substr(start, pos_ - start));
        }


        // If neither a digit or an operation, it is something bad, throw error
        return make(TokenType::Error, std::string("Unexpected character '") + c + "'");
    }


    // Need to avoid spaces, tabs, blanks
    void skipWhitespaceAndComments() {
        while (!isAtEnd()) {

            // Look at the next character
            char c = peek();

            // If it is blank, move on
            if (c == ' ' || c == '\t' || c == '\r') {
                advance();
            }

            // if we hit an empty line, move on and increment line
            else if (c == '\n') {
                line_++;
                advance();
            }

            // If we ran into a comment, go to the end of the line
            else if (c == '#') {
                while (!isAtEnd() && peek() != '\n') advance();
            }

            else {
                break;
            }

        }
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

            // Skip spaces
            skipWhitespaceAndComments();

            // make sure we are not at the end
            if (isAtEnd()) break;

            // Takes that newly created token 
            // and adds it to the end of your list.
            tokens.push_back(scanToken());
        }

        // tacks on one final token: an EndOfFile token.
        tokens.push_back(make(TokenType::EndOfFile, ""));
        return tokens;
    }
};

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------

