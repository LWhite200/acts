#ifndef AST_H
#define AST_H

#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <utility>

#include "lexer.h" // Token is used by operator expression nodes.

// -----------------------------------------------------------------------------
// SMALL AST UTILITIES
// -----------------------------------------------------------------------------

// Print a number without unnecessary trailing zeroes (12 instead of 12.000000).
inline std::string formatNumber(double value) {
    std::ostringstream output;
    output << value;
    return output.str();
}

// Add two spaces for each level of indentation in a statement tree.
inline std::string pad(int indent) {
    return std::string(indent * 2, ' ');
}

// -----------------------------------------------------------------------------
// EXPRESSIONS
// An expression is something that produces a value: 5, x, a + b, f(1), xs[0].
// -----------------------------------------------------------------------------

struct Expr {
    int line;
    explicit Expr(int line) : line(line) {}
    virtual ~Expr() = default;
    virtual std::string print() const = 0;
};

// unique_ptr means each AST child has one clear owner.
using ExprPtr = std::unique_ptr<Expr>;

struct NumberExpr : Expr {
    double value;
    NumberExpr(double value, int line) : Expr(line), value(value) {}
    std::string print() const override { return formatNumber(value); }
};

struct StringExpr : Expr {
    std::string value;
    StringExpr(std::string value, int line)
        : Expr(line), value(std::move(value)) {
    }
    std::string print() const override { return "\"" + value + "\""; }
};

struct BoolExpr : Expr {
    bool value;
    BoolExpr(bool value, int line) : Expr(line), value(value) {}
    std::string print() const override { return value ? "true" : "false"; }
};

struct NilExpr : Expr {
    explicit NilExpr(int line) : Expr(line) {}
    std::string print() const override { return "nil"; }
};

struct VariableExpr : Expr {
    std::string name;
    VariableExpr(std::string name, int line)
        : Expr(line), name(std::move(name)) {
    }
    std::string print() const override { return name; }
};

struct UnaryExpr : Expr {
    Token op;
    ExprPtr right;
    UnaryExpr(Token op, ExprPtr right)
        : Expr(op.line), op(std::move(op)), right(std::move(right)) {
    }
    std::string print() const override {
        return "(" + op.text + " " + right->print() + ")";
    }
};

struct BinaryExpr : Expr {
    ExprPtr left;
    Token op;
    ExprPtr right;
    BinaryExpr(ExprPtr left, Token op, ExprPtr right)
        : Expr(op.line), left(std::move(left)), op(std::move(op)),
        right(std::move(right)) {
    }
    std::string print() const override {
        return "(" + op.text + " " + left->print() + " " + right->print() + ")";
    }
};

// Logical expressions are separate so an interpreter can later implement
// short-circuit evaluation for and/or.
struct LogicalExpr : Expr {
    ExprPtr left;
    Token op;
    ExprPtr right;
    LogicalExpr(ExprPtr left, Token op, ExprPtr right)
        : Expr(op.line), left(std::move(left)), op(std::move(op)),
        right(std::move(right)) {
    }
    std::string print() const override {
        return "(" + op.text + " " + left->print() + " " + right->print() + ")";
    }
};

struct AssignExpr : Expr {
    std::string name;
    ExprPtr value;
    AssignExpr(std::string name, ExprPtr value, int line)
        : Expr(line), name(std::move(name)), value(std::move(value)) {
    }
    std::string print() const override {
        return "(= " + name + " " + value->print() + ")";
    }
};

// A function call. The callee is itself an expression, which allows f(1)(2).
struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    CallExpr(ExprPtr callee, std::vector<ExprPtr> args, int line)
        : Expr(line), callee(std::move(callee)), args(std::move(args)) {
    }
    std::string print() const override {
        std::string output = "(call " + callee->print();
        for (const ExprPtr& arg : args) output += " " + arg->print();
        return output + ")";
    }
};

// A list literal, for example [10, 20, 30].
struct ListExpr : Expr {
    std::vector<ExprPtr> elements;
    ListExpr(std::vector<ExprPtr> elements, int line)
        : Expr(line), elements(std::move(elements)) {
    }
    std::string print() const override {
        std::string output = "(list";
        for (const ExprPtr& element : elements) output += " " + element->print();
        return output + ")";
    }
};

// Reading one element: xs[0]. The object can be any expression, not just a name.
struct IndexExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    IndexExpr(ExprPtr object, ExprPtr index, int line)
        : Expr(line), object(std::move(object)), index(std::move(index)) {
    }
    std::string print() const override {
        return "(index " + object->print() + " " + index->print() + ")";
    }
};

// Writing one element: xs[0] = 9.
struct IndexAssignExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    ExprPtr value;
    IndexAssignExpr(ExprPtr object, ExprPtr index, ExprPtr value, int line)
        : Expr(line), object(std::move(object)), index(std::move(index)),
        value(std::move(value)) {
    }
    std::string print() const override {
        return "(index= " + object->print() + " " + index->print() +
            " " + value->print() + ")";
    }
};

// -----------------------------------------------------------------------------
// STATEMENTS
// A statement performs an action: let, print, if, while, function, return, etc.
// -----------------------------------------------------------------------------

struct Stmt {
    int line;
    explicit Stmt(int line) : line(line) {}
    virtual ~Stmt() = default;
    virtual std::string print(int indent) const = 0;
};
using StmtPtr = std::unique_ptr<Stmt>;

struct ParseError {
    int line;
    std::string message;
};

struct LetStmt : Stmt {
    std::string name;
    ExprPtr initializer;
    LetStmt(std::string name, ExprPtr initializer, int line)
        : Stmt(line), name(std::move(name)), initializer(std::move(initializer)) {
    }
    std::string print(int indent) const override {
        return pad(indent) + "(let " + name + " " + initializer->print() + ")";
    }
};

struct PrintStmt : Stmt {
    ExprPtr value;
    PrintStmt(ExprPtr value, int line)
        : Stmt(line), value(std::move(value)) {
    }
    std::string print(int indent) const override {
        return pad(indent) + "(print " + value->print() + ")";
    }
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    ExprStmt(ExprPtr expr, int line)
        : Stmt(line), expr(std::move(expr)) {
    }
    std::string print(int indent) const override {
        return pad(indent) + "(expr " + expr->print() + ")";
    }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    BlockStmt(std::vector<StmtPtr> statements, int line)
        : Stmt(line), statements(std::move(statements)) {
    }
    std::string print(int indent) const override {
        std::string output = pad(indent) + "(block";
        for (const StmtPtr& statement : statements)
            output += "\n" + statement->print(indent + 1);
        return output + ")";
    }
};

struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch; // nullptr when there is no else branch.
    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch, int line)
        : Stmt(line), condition(std::move(condition)),
        thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {
    }
    std::string print(int indent) const override {
        std::string output = pad(indent) + "(if " + condition->print() + "\n" +
            thenBranch->print(indent + 1);
        if (elseBranch) output += "\n" + elseBranch->print(indent + 1);
        return output + ")";
    }
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;
    WhileStmt(ExprPtr condition, StmtPtr body, int line)
        : Stmt(line), condition(std::move(condition)), body(std::move(body)) {
    }
    std::string print(int indent) const override {
        return pad(indent) + "(while " + condition->print() + "\n" +
            body->print(indent + 1) + ")";
    }
};

struct FnStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;
    FnStmt(std::string name, std::vector<std::string> params,
        std::unique_ptr<BlockStmt> body, int line)
        : Stmt(line), name(std::move(name)), params(std::move(params)),
        body(std::move(body)) {
    }
    std::string print(int indent) const override {
        std::string parameterList;
        for (size_t i = 0; i < params.size(); ++i)
            parameterList += (i ? " " : "") + params[i];
        return pad(indent) + "(fn " + name + " (" + parameterList + ")\n" +
            body->print(indent + 1) + ")";
    }
};

struct ReturnStmt : Stmt {
    ExprPtr value; // nullptr means a bare return;
    ReturnStmt(ExprPtr value, int line)
        : Stmt(line), value(std::move(value)) {
    }
    std::string print(int indent) const override {
        return pad(indent) + "(return" +
            (value ? " " + value->print() : "") + ")";
    }
};

#endif // AST_H
