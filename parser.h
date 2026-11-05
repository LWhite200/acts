#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "ast.h"
#include "lexer.h"

class Parser {
public:

    // explicit -> do not change the object type to fit
    // Parser(sd::vector<Token> tokens) -> Take a list of tokens
    // moving the incoming parameter 'tokens' and putting it inside tokens_ 
    // // and wiping the old one clean
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    // IDK if this is where it should be
    ExprPtr parseExpression() { return parsePrimary(); }

private:

    // dynamic array (a growable list) that will hold elements of type Token.
    std::vector<Token> tokens_;
    size_t current_ = 0;

    // Look at current token
    const Token& peek() const { return tokens_[current_]; }

    // Look at previous token
    const Token& previous() const { return tokens_[current_ - 1]; }

    // If we are at end
    // mathches the token type to the end of file token
    bool isAtEnd() const { return peek().type == TokenType::EndOfFile; }

    // Move the token we are looking at forward
    const Token& advance() {
        if (!isAtEnd()) current_++;
        return previous(); // since we made sure we are not currently at the end if the file
    }

    // Does the current token match the type we are looking for
    bool check(TokenType type) const { return peek().type == type; }

    // Do we have matching tokens?
    // If so, we look at the next one
    bool match(TokenType type) {
        if (!check(type)) return false;
        advance();
        return true;
    }

    // debug message
    std::string found() const {
        return isAtEnd() ? "end of file" : "'" + peek().text + "'";
    }

    // Each mathch eats the token and builds one leaf node.
    // If nothing matches, we do not know what this token is doing here. so throw error
    // turn tokens into nodes
    ExprPtr parsePrimary() {

        // If we found a number token
        if (match(TokenType::Number)) {
            // Set the token's string into a double
            double value = std::stod(previous().text);

            // Return a unive NumberExpr object
            return std::make_unique<NumberExpr>(value, previous().line);
        }
        if (match(TokenType::String)) {
            return std::make_unique<StringExpr>(previous().text, previous().line);
        }

        // both are boolExpr, but easier to tell what specifically it is
        if (match(TokenType::True)) {
            return std::make_unique<BoolExpr>(true, previous().line);
        }
        if (match(TokenType::False)) {
            return std::make_unique<BoolExpr>(false, previous().line);
        }
        throw ParseError{ peek().line, "Expect expression. Found " + found() + "." };
    }
};

#endif // PARSER_H