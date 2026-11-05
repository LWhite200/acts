
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "lexer.h"

// Base class for all expression nodes in the Abstract Syntax Tree (AST).
/* - Expression node
* 
* - any piece of code or query that evaluates to a value.
* - literals (42, "hello", true), identifiers (x, user_id, user,id)
* - binary/unary operations (a + b, x * 2, NOT active)
* - function calls (sqrt(16), count(*)) 
* - conditional / control expressions (CASE WHEN .. END)
*
*/
struct Expr {
    int line; // Tracks the source line number for precise compiler error reporting.
    
    // Initializes the line number. 'explicit' prevents 
    // accidental implicit integer-to-Expr conversions.
    // - explicit - don't turn an int into an Expr
    // - Expr(int line) - the function name and the int parameter
    // - line(line) - this.line = line
    explicit Expr(int line) : line(line) {}
    
    // Virtual destructor is mandatory here. Without it, 
    // deleting derived expression nodes 
    // through an Expr* base pointer would cause resource leaks
    // (derived destructors wouldn't run).
    //
    // Jump to child's deconstruction
    // ~ denotes a destructor.
    // '= default' normal memory-cleanup process
    virtual ~Expr() = default;
    
    // Every child must have a print class
    // use that one instead of this one.
    // ''polymorphism'' if animal, use dog.bark()
    // cannot creae a plan expression, must be child.
    // it know no base because '0'
    virtual std::string print() const = 0;
};

// childExpr is a specialized kind of Expr through inheritance.
// so their Expr has a line number

struct NumberExpr : Expr {

    // 3.14
    double value;

    // Constructor - parameters, where they go.
    NumberExpr(double value, int line) : Expr(line), value(value) {}

    // What their print function does
    std::string print() const override { return formatNumber(value); }
};
 
struct StringExpr : Expr {
    std::string value;
    StringExpr(std::string value, int line) : Expr(line), value(std::move(value)) {}
    std::string print() const override { return "\"" + value + "\""; }
};
 
struct BoolExpr : Expr {
    bool value;
    BoolExpr(bool value, int line) : Expr(line), value(value) {}
    std::string print() const override { return value ? "true" : "false"; }
};


// ended on step 3 of round 4

// using - Create a custom nickname for a type: long right side becomes
// the short left 'ExprPtr'
//
// std::unique_ptr<Expr>;
// unique - means only one variable can own this
// )nly one variable of an <Expr> object can be pointed at one time? 
using ExprPtr = std::unique_ptr<Expr>;

// Utility helper to convert numeric values into a string representation 
// (commonly used when printing or serializing numeric literal nodes).
std::string formatNumber(double v) {

    // string builder (java) 
    // use stream since there may be weird trailing decimals with
    // normal string.
    // just makes 12 print as 12 instead of 12.000000
    std::ostringstream os; 

    // Takes v (double) and funnels it into a ostringstream
    os << v;

    // Turn stream into string
    return os.str();
}