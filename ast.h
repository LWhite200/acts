#ifndef AST_H
#define AST_H

#include <string>
#include <sstream>
#include <memory>

// 1. Define formatNumber FIRST so the compiler knows it exists
// Utility helper to convert numeric values into a string representation 
// (commonly used when printing or serializing numeric literal nodes).
inline std::string formatNumber(double v) {

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

// Base class for all expression nodes in the Abstract Syntax Tree (AST).
/* - Expression node
- any piece of code or query that evaluates to a value.
- literals (42, "hello", true), identifiers (x, user_id, user,id)
- binary/unary operations (a + b, x * 2, NOT active)
- function calls (sqrt(16), count(*))
- conditional / control expressions (CASE WHEN .. END)
**/
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

// using - Create a custom nickname for a type: long right side becomes
// the short left 'ExprPtr'
//
// std::unique_ptr<Expr>;
// unique - means only one variable can own this
// Only one variable of an <Expr> object can be pointed at one time? 
using ExprPtr = std::unique_ptr<Expr>;

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


// Nil
struct NilExpr : Expr {
    NilExpr(int line) : Expr(line) {}
    std::string print() const override { return "nil"; }
};

struct ParseError {
    int line;
    std::string message;
};

// operator [left child] token - right child
struct BinaryExpr : Expr {

    ExprPtr left;
    Token op;
    ExprPtr right;

    // Constructor taking in three arguments
    // Creates ...
    BinaryExpr(ExprPtr left, Token op, ExprPtr right)
        // Expr(op.line): Calls the constructor of the parent class (Expr), 
        // passing the line number where the operator appears
        // so the base class can track its position.
        //
        //std::move transfers ownership of the incoming pointer parameter into 
        // the class's member variable (this->left).
        : Expr(op.line), left(std::move(left)), op(std::move(op)),
        right(std::move(right)) {
    }

    // print() writes the operator first and then the children: (+ 1 2). 
    // This notation is called an S-expression. Nesting becomes obvious at a glance:
    // the parentheses are the tree.
    std::string print() const override {
        return "(" + op.text + " " + left->print() + " " + right->print() + ")";
    }
};


#endif // AST_H