
#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <initializer_list>

#include "lexer.h"
#include "ast.h"

// ============================================================================
// PARSER
//
// The lexer turns source code into tokens.
// The parser takes those tokens and builds an Abstract Syntax Tree (AST).
//
// Example:
//     print 1 + 2;
//
// The parser builds something similar to:
//     PrintStmt
//         BinaryExpr (+)
//             NumberExpr (1)
//             NumberExpr (2)
// ============================================================================

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : tokens_(std::move(tokens)) {
    }

    // Parse the entire file.
    // Failed statements are skipped, and their errors are saved in errors_.
    std::vector<StmtPtr> parseProgram() {
        std::vector<StmtPtr> statements;

        while (!isAtEnd()) {
            StmtPtr statement = parseDeclaration();

            if (statement) {
                statements.push_back(std::move(statement));
            }
        }

        return statements;
    }

    // Allow main.cpp to retrieve all parser errors.
    const std::vector<ParseError>& errors() const {
        return errors_;
    }

    // Public entry point for parsing a single expression.
    ExprPtr parseExpression() {
        return parseAssignment();
    }

private:
    // ========================================================================
    // PARSER DATA
    // ========================================================================

    std::vector<Token> tokens_;

    // current_ tells us which token we are currently looking at.
    size_t current_ = 0;

    // Stores errors so the parser can report multiple problems in one run.
    std::vector<ParseError> errors_;

    // ========================================================================
    // TOKEN HELPERS
    // ========================================================================

    // Return the current token without consuming it.
    const Token& peek() const {
        return tokens_[current_];
    }

    // Return the token we most recently consumed.
    const Token& previous() const {
        return tokens_[current_ - 1];
    }

    // Check whether we have reached the end of the token list.
    bool isAtEnd() const {
        return peek().type == TokenType::EndOfFile;
    }

    // Consume the current token and move forward.
    const Token& advance() {
        if (!isAtEnd()) {
            ++current_;
        }

        return previous();
    }

    // Check the type of the current token without consuming it.
    bool check(TokenType type) const {
        return peek().type == type;
    }

    // If the current token matches, consume it and return true.
    // Otherwise, leave it alone and return false.
    bool match(TokenType type) {
        if (!check(type)) {
            return false;
        }

        advance();
        return true;
    }

    // Try several token types.
    // Useful when multiple operators share the same precedence.
    bool matchAny(std::initializer_list<TokenType> types) {
        for (TokenType type : types) {
            if (match(type)) {
                return true;
            }
        }

        return false;
    }

    // Get the most recently consumed line number.
    int lastLine() const {
        return current_ > 0 ? previous().line : peek().line;
    }

    // Describe the current token for error messages.
    std::string found() const {
        if (isAtEnd()) {
            return "end of file";
        }

        return "'" + peek().text + "'";
    }

    // Require a specific token.
    // If it is missing, throw a ParseError.
    const Token& consume(TokenType type, const std::string& message) {
        if (check(type)) {
            return advance();
        }

        throw ParseError{
            lastLine(),
            message + " Found " + found() + "."
        };
    }

    // ========================================================================
    // ROUND 8: ERROR RECOVERY
    // ========================================================================

    // This wrapper catches errors from individual declarations.
    //
    // Instead of allowing one error to terminate the entire parser:
    // 1. Save the error.
    // 2. Skip damaged tokens.
    // 3. Resume at a likely statement boundary.
    // 4. Return nullptr so the caller knows parsing failed.
    StmtPtr parseDeclaration() {
        const size_t start = current_;

        try {
            return parseDeclarationInner();
        }
        catch (const ParseError& error) {
            errors_.push_back(error);

            synchronize();

            // Safety check: make sure we made progress.
            // This prevents an infinite loop on malformed input.
            if (current_ == start) {
                advance();
            }

            return nullptr;
        }
    }

    // Skip tokens until we find a safe place to resume parsing.
    void synchronize() {
        while (!isAtEnd()) {

            // A semicolon means the previous statement has ended.
            if (current_ > 0 &&
                previous().type == TokenType::Semicolon) {
                return;
            }

            // Leave the closing brace for parseBlock() to consume.
            if (check(TokenType::RightBrace)) {
                return;
            }

            // These tokens can begin a new statement or declaration.
            switch (peek().type) {
            case TokenType::Let:
            case TokenType::Fn:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Print:
            case TokenType::Return:
                return;

            default:
                break;
            }

            advance();
        }
    }

    // The actual declaration parser.
    // Errors are caught by the parseDeclaration() wrapper.
    StmtPtr parseDeclarationInner() {
        if (match(TokenType::Fn)) {
            return parseFunction();
        }

        if (match(TokenType::Let)) {
            return parseLet();
        }

        return parseStatement();
    }

    // ========================================================================
    // DECLARATIONS AND STATEMENTS
    // ========================================================================

    // Parse:
    //     let name = expression;
    StmtPtr parseLet() {
        const int line = previous().line;

        Token name = consume(
            TokenType::Identifier,
            "Expect variable name after 'let'."
        );

        consume(
            TokenType::Equal,
            "Expect '=' after variable name."
        );

        ExprPtr initializer = parseExpression();

        consume(
            TokenType::Semicolon,
            "Expect ';' after variable declaration."
        );

        return std::make_unique<LetStmt>(
            name.text,
            std::move(initializer),
            line
        );
    }

    // Determine which kind of statement we are parsing.
    StmtPtr parseStatement() {
        if (match(TokenType::Print)) {
            return parsePrint();
        }

        if (match(TokenType::If)) {
            return parseIf();
        }

        if (match(TokenType::While)) {
            return parseWhile();
        }

        if (match(TokenType::Return)) {
            return parseReturn();
        }

        if (check(TokenType::LeftBrace)) {
            return parseBlock();
        }

        return parseExprStmt();
    }

    // Parse:
    //     print expression;
    StmtPtr parsePrint() {
        const int line = previous().line;

        ExprPtr value = parseExpression();

        consume(
            TokenType::Semicolon,
            "Expect ';' after value."
        );

        return std::make_unique<PrintStmt>(
            std::move(value),
            line
        );
    }

    // Parse a standalone expression:
    //     x = 5;
    //     functionCall();
    StmtPtr parseExprStmt() {
        const int line = peek().line;

        ExprPtr expression = parseExpression();

        consume(
            TokenType::Semicolon,
            "Expect ';' after expression."
        );

        return std::make_unique<ExprStmt>(
            std::move(expression),
            line
        );
    }

    // Parse:
    //     if condition { ... }
    //     if condition { ... } else { ... }
    StmtPtr parseIf() {
        const int line = previous().line;

        ExprPtr condition = parseExpression();
        StmtPtr thenBranch = parseBlock();

        StmtPtr elseBranch = nullptr;

        if (match(TokenType::Else)) {
            if (match(TokenType::If)) {
                elseBranch = parseIf();
            }
            else {
                elseBranch = parseBlock();
            }
        }

        return std::make_unique<IfStmt>(
            std::move(condition),
            std::move(thenBranch),
            std::move(elseBranch),
            line
        );
    }

    // Parse:
    //     while condition { ... }
    StmtPtr parseWhile() {
        const int line = previous().line;

        ExprPtr condition = parseExpression();
        StmtPtr body = parseBlock();

        return std::make_unique<WhileStmt>(
            std::move(condition),
            std::move(body),
            line
        );
    }

    // Parse a block surrounded by braces.
    //
    // Example:
    //     {
    //         print 1;
    //         print 2;
    //     }
    //
    // We use parseDeclaration() here so errors inside blocks can also
    // be recovered from.
    std::unique_ptr<BlockStmt> parseBlock() {
        const int line = consume(
            TokenType::LeftBrace,
            "Expect '{' to start a block."
        ).line;

        std::vector<StmtPtr> statements;

        while (!check(TokenType::RightBrace) && !isAtEnd()) {
            StmtPtr statement = parseDeclaration();

            if (statement) {
                statements.push_back(std::move(statement));
            }
        }

        consume(
            TokenType::RightBrace,
            "Expect '}' after block."
        );

        return std::make_unique<BlockStmt>(
            std::move(statements),
            line
        );
    }

    // ========================================================================
    // ROUND 7: FUNCTIONS AND RETURN
    // ========================================================================

    // Parse a function declaration:
    //
    //     fn add(a, b) {
    //         return a + b;
    //     }
    StmtPtr parseFunction() {
        Token name = consume(
            TokenType::Identifier,
            "Expect function name after 'fn'."
        );

        consume(
            TokenType::LeftParen,
            "Expect '(' after function name."
        );

        std::vector<std::string> parameters;

        // Parameters are optional.
        // If there are any, read them separated by commas.
        if (!check(TokenType::RightParen)) {
            do {
                Token parameter = consume(
                    TokenType::Identifier,
                    "Expect parameter name."
                );

                parameters.push_back(parameter.text);

            } while (match(TokenType::Comma));
        }

        consume(
            TokenType::RightParen,
            "Expect ')' after parameters."
        );

        std::unique_ptr<BlockStmt> body = parseBlock();

        return std::make_unique<FnStmt>(
            name.text,
            std::move(parameters),
            std::move(body),
            name.line
        );
    }

    // Parse:
    //     return;
    //     return expression;
    StmtPtr parseReturn() {
        const int line = previous().line;

        ExprPtr value = nullptr;

        // If the next token is not a semicolon, a value must follow.
        if (!check(TokenType::Semicolon)) {
            value = parseExpression();
        }

        consume(
            TokenType::Semicolon,
            "Expect ';' after return value."
        );

        return std::make_unique<ReturnStmt>(
            std::move(value),
            line
        );
    }

    // ========================================================================
    // EXPRESSIONS
    //
    // These functions form a precedence ladder.
    //
    // Assignment has the lowest precedence.
    // Primary expressions have the highest precedence.
    //
    // For example:
    //     1 + 2 * 3
    //
    // Multiplication is parsed before addition.
    // ========================================================================

    // ------------------------------------------------------------------------
    // Assignment
    // ------------------------------------------------------------------------

    // Parse:
    //     x = 5
    //     xs[0] = 5
    ExprPtr parseAssignment() {
        ExprPtr expression = parseLogicOr();

        if (match(TokenType::Equal)) {
            Token equals = previous();

            // Assignment is right-associative.
            // This allows: a = b = 5
            ExprPtr value = parseAssignment();

            // Check whether the left side is a variable.
            if (auto variable =
                dynamic_cast<VariableExpr*>(expression.get())) {

                return std::make_unique<AssignExpr>(
                    variable->name,
                    std::move(value),
                    equals.line
                );
            }

            // Also allow assignment to an indexed list element.
            if (auto indexed =
                dynamic_cast<IndexExpr*>(expression.get())) {

                return std::make_unique<IndexAssignExpr>(
                    std::move(indexed->object),
                    std::move(indexed->index),
                    std::move(value),
                    equals.line
                );
            }

            throw ParseError{
                equals.line,
                "Invalid assignment target."
            };
        }

        return expression;
    }

    // ------------------------------------------------------------------------
    // Logical OR
    // ------------------------------------------------------------------------

    ExprPtr parseLogicOr() {
        ExprPtr left = parseLogicAnd();

        while (match(TokenType::Or)) {
            Token op = previous();
            ExprPtr right = parseLogicAnd();

            left = std::make_unique<LogicalExpr>(
                std::move(left),
                op,
                std::move(right)
            );
        }

        return left;
    }

    // ------------------------------------------------------------------------
    // Logical AND
    // ------------------------------------------------------------------------

    ExprPtr parseLogicAnd() {
        ExprPtr left = parseEquality();

        while (match(TokenType::And)) {
            Token op = previous();
            ExprPtr right = parseEquality();

            left = std::make_unique<LogicalExpr>(
                std::move(left),
                op,
                std::move(right)
            );
        }

        return left;
    }

    // ------------------------------------------------------------------------
    // Equality: == and !=
    // ------------------------------------------------------------------------

    ExprPtr parseEquality() {
        ExprPtr left = parseComparison();

        while (matchAny({
            TokenType::EqualEqual,
            TokenType::BangEqual
            })) {
            Token op = previous();
            ExprPtr right = parseComparison();

            left = std::make_unique<BinaryExpr>(
                std::move(left),
                op,
                std::move(right)
            );
        }

        return left;
    }

    // ------------------------------------------------------------------------
    // Comparison: <, <=, >, >=
    // ------------------------------------------------------------------------

    ExprPtr parseComparison() {
        ExprPtr left = parseTerm();

        while (matchAny({
            TokenType::Less,
            TokenType::LessEqual,
            TokenType::Greater,
            TokenType::GreaterEqual
            })) {
            Token op = previous();
            ExprPtr right = parseTerm();

            left = std::make_unique<BinaryExpr>(
                std::move(left),
                op,
                std::move(right)
            );
        }

        return left;
    }

    // ------------------------------------------------------------------------
    // Addition and subtraction
    // ------------------------------------------------------------------------

    ExprPtr parseTerm() {
        ExprPtr left = parseFactor();

        while (matchAny({
            TokenType::Plus,
            TokenType::Minus
            })) {
            Token op = previous();
            ExprPtr right = parseFactor();

            left = std::make_unique<BinaryExpr>(
                std::move(left),
                op,
                std::move(right)
            );
        }

        return left;
    }

    // ------------------------------------------------------------------------
    // Multiplication, division, and remainder
    // ------------------------------------------------------------------------

    ExprPtr parseFactor() {
        ExprPtr left = parseUnary();

        while (matchAny({
            TokenType::Star,
            TokenType::Slash,
            TokenType::Percent
            })) {
            Token op = previous();
            ExprPtr right = parseUnary();

            left = std::make_unique<BinaryExpr>(
                std::move(left),
                op,
                std::move(right)
            );
        }

        return left;
    }

    // ------------------------------------------------------------------------
    // Unary operators: - and !
    // ------------------------------------------------------------------------

    ExprPtr parseUnary() {
        if (matchAny({
            TokenType::Minus,
            TokenType::Not
            })) {
            Token op = previous();

            ExprPtr right = parseUnary();

            return std::make_unique<UnaryExpr>(
                op,
                std::move(right)
            );
        }

        // If there is no unary operator, continue to calls and indexing.
        return parseCall();
    }

    // ========================================================================
    // ROUND 7 AND LIST SUPPORT: CALLS AND INDEXING
    // ========================================================================

    // Calls and indexing are postfix operations.
    //
    // The loop allows them to be chained:
    //     f(1)(2)
    //     grid[1][2]
    //     f(x)[0]
    ExprPtr parseCall() {
        ExprPtr expression = parsePrimary();

        while (true) {

            // Function call: someFunction(...)
            if (match(TokenType::LeftParen)) {
                expression = finishCall(std::move(expression));
            }

            // List indexing: someList[...]
            else if (match(TokenType::LeftBracket)) {
                expression = finishIndex(std::move(expression));
            }

            else {
                break;
            }
        }

        return expression;
    }

    // Finish parsing a function call after its opening parenthesis.
    ExprPtr finishCall(ExprPtr callee) {
        std::vector<ExprPtr> arguments;

        if (!check(TokenType::RightParen)) {
            do {
                arguments.push_back(parseExpression());

            } while (match(TokenType::Comma));
        }

        Token closingParen = consume(
            TokenType::RightParen,
            "Expect ')' after arguments."
        );

        return std::make_unique<CallExpr>(
            std::move(callee),
            std::move(arguments),
            closingParen.line
        );
    }

    // Finish parsing an index operation after its opening bracket.
    ExprPtr finishIndex(ExprPtr object) {
        ExprPtr index = parseExpression();

        Token closingBracket = consume(
            TokenType::RightBracket,
            "Expect ']' after list index."
        );

        return std::make_unique<IndexExpr>(
            std::move(object),
            std::move(index),
            closingBracket.line
        );
    }

    // ========================================================================
    // PRIMARY EXPRESSIONS
    //
    // Primary expressions are the basic building blocks:
    // numbers, strings, variables, grouped expressions, and lists.
    // ========================================================================

    ExprPtr parsePrimary() {

        // Number literal.
        if (match(TokenType::Number)) {
            Token token = previous();

            return std::make_unique<NumberExpr>(
                std::stod(token.text),
                token.line
            );
        }

        // String literal.
        if (match(TokenType::String)) {
            Token token = previous();

            return std::make_unique<StringExpr>(
                token.text,
                token.line
            );
        }

        // Boolean true.
        if (match(TokenType::True)) {
            return std::make_unique<BoolExpr>(
                true,
                previous().line
            );
        }

        // Boolean false.
        if (match(TokenType::False)) {
            return std::make_unique<BoolExpr>(
                false,
                previous().line
            );
        }

        // Nil literal.
        if (match(TokenType::Nil)) {
            return std::make_unique<NilExpr>(
                previous().line
            );
        }

        // Variable name.
        if (match(TokenType::Identifier)) {
            Token token = previous();

            return std::make_unique<VariableExpr>(
                token.text,
                token.line
            );
        }

        // Grouped expression:
        //     (1 + 2)
        if (match(TokenType::LeftParen)) {
            ExprPtr expression = parseExpression();

            consume(
                TokenType::RightParen,
                "Expect ')' after expression."
            );

            return expression;
        }

        // List literal:
        //     [1, 2, 3]
        //     []
        if (match(TokenType::LeftBracket)) {
            const int line = previous().line;

            std::vector<ExprPtr> elements;

            // An immediate closing bracket means an empty list.
            if (!check(TokenType::RightBracket)) {
                do {
                    elements.push_back(parseExpression());

                } while (match(TokenType::Comma));
            }

            consume(
                TokenType::RightBracket,
                "Expect ']' after list elements."
            );

            return std::make_unique<ListExpr>(
                std::move(elements),
                line
            );
        }

        // None of the valid primary expressions matched.
        throw ParseError{
            peek().line,
            "Expect expression. Found " + found() + "."
        };
    }
};

#endif // PARSER_H
