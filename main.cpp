#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "lexer.h"
#include "ast.h"
#include "parser.h"

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Cannot open " << path << "\n";
        std::exit(1);
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Get the tokens from the lexer with the 
std::vector<Token> lexOrExit(const std::string& source) {
    std::vector<Token> tokens = Lexer(source).scanAll();
    bool bad = false;
    for (const Token& t : tokens) {
        if (t.type == TokenType::Error) {
            std::cerr << "[line " << t.line << "] Lex error: " << t.text << "\n";
            bad = true;
        }
    }
    if (bad) std::exit(1);
    return tokens;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <file>\n";
        return 1;
    }

    // Pass the stream of strings into the lexer and get a list of tokens
    Parser parser(lexOrExit(readFile(argv[1])));
    try {
        ExprPtr tree = parser.parseExpression();
        std::cout << tree->print() << "\n";
    }
    catch (const ParseError& e) {
        std::cerr << "[line " << e.line << "] Parse error: " << e.message << "\n";
        return 1;
    }
    return 0;
}