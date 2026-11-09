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

    // Open file at given location
    std::ifstream in(path);

    // If file does not exists, show it cannot be opened
    if (!in) {
        std::cerr << "Cannot open " << path << "\n";
        std::exit(1);
    }

    // Create the variable that will hold the file as a stringstream
    std::stringstream buffer;

    // idiomatic C++ trick to slurp the entire file into memory all at once 
    // without having to read it line-by-line in a loop.
    buffer << in.rdbuf();

    // Return the string buffer as a string
    // A std::string is much simpler and lighter—it is 
    // just a contiguous block of memory holding characters 
    // plus a length. You don't want to pass a heavy 
    // stream object around your codebase when all you really care about is the text inside it.
    return buffer.str();
}





// Get the tokens from the lexer with the 
std::vector<Token> lexOrExit(const std::string& source) {

    //Creates a temporary instance of your lexer and feeds it the raw code string.
    // .scanAll(): Tells the lexer to read through the entire string from start to finish, 
    // breaking it down into individual tokens (like keywords, identifiers, numbers, and operators)
    // and storing them in a std::vector<Token>.
    /*
    
    struct Token {
        TokenType type;     // What kind of token it is (e.g., Number, Keyword, Plus sign, Identifier)
        std::string text;   // The exact text snippet from the code (e.g., "42", "x", "+")
        int line;           // The source line number where it was found
    };
    
    */
    std::vector<Token> tokens = Lexer(source).scanAll();

    // A flag used to track if any errors were found during scanning.
    bool bad = false;

    // Loop through the tokens and see if there were any error tokens
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
    /*
    [1] - Readfile and put it all into a string

    [2] - Go through entire file and tokenize it

    [3] - Create Parser instance with these tokens within it
    */
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