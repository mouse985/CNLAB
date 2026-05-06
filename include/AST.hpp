#pragma once

#include "Matrix.hpp"
#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace cnlab {

class ASTVisitor;

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

struct Expression : ASTNode {};
struct Statement : ASTNode {};

using ExprPtr = std::shared_ptr<Expression>;
using StmtPtr = std::shared_ptr<Statement>;

struct NumberLiteral : Expression {
    double value;
    explicit NumberLiteral(double v) : value(v) {}
    void accept(ASTVisitor& visitor) override;
};

struct ComplexLiteral : Expression {
    double real;
    double imag;
    ComplexLiteral(double r, double i) : real(r), imag(i) {}
    void accept(ASTVisitor& visitor) override;
};

struct StringLiteral : Expression {
    std::string value;
    explicit StringLiteral(std::string v) : value(std::move(v)) {}
    void accept(ASTVisitor& visitor) override;
};

struct CharArrayLiteral : Expression {
    std::string value;
    explicit CharArrayLiteral(std::string v) : value(std::move(v)) {}
    void accept(ASTVisitor& visitor) override;
};

struct BooleanLiteral : Expression {
    bool value;
    explicit BooleanLiteral(bool v) : value(v) {}
    void accept(ASTVisitor& visitor) override;
};

struct Identifier : Expression {
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}
    void accept(ASTVisitor& visitor) override;
};

struct MatrixLiteral : Expression {
    std::vector<std::vector<ExprPtr>> rows;
    void accept(ASTVisitor& visitor) override;
};

struct RangeExpression : Expression {
    ExprPtr start;
    ExprPtr step;
    ExprPtr end;
    RangeExpression(ExprPtr s, ExprPtr st, ExprPtr e) 
        : start(std::move(s)), step(std::move(st)), end(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

struct BinaryExpression : Expression {
    ExprPtr left;
    std::string op;
    ExprPtr right;
    BinaryExpression(ExprPtr l, std::string o, ExprPtr r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
    void accept(ASTVisitor& visitor) override;
};

struct UnaryExpression : Expression {
    std::string op;
    ExprPtr operand;
    UnaryExpression(std::string o, ExprPtr e)
        : op(std::move(o)), operand(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

struct CallExpression : Expression {
    ExprPtr callee;
    std::vector<ExprPtr> arguments;
    CallExpression(ExprPtr c, std::vector<ExprPtr> args)
        : callee(std::move(c)), arguments(std::move(args)) {}
    void accept(ASTVisitor& visitor) override;
};

struct ColonExpression : Expression {
    void accept(ASTVisitor& visitor) override;
};

struct EndExpression : Expression {
    void accept(ASTVisitor& visitor) override;
};

struct IndexExpression : Expression {
    ExprPtr object;
    std::vector<ExprPtr> indices;
    IndexExpression(ExprPtr obj, std::vector<ExprPtr> idx)
        : object(std::move(obj)), indices(std::move(idx)) {}
    void accept(ASTVisitor& visitor) override;
};

struct AnonymousFunction : Expression {
    std::vector<std::string> parameters;
    ExprPtr body;
    AnonymousFunction(std::vector<std::string> params, ExprPtr b)
        : parameters(std::move(params)), body(std::move(b)) {}
    void accept(ASTVisitor& visitor) override;
};

struct FunctionHandle : Expression {
    std::string functionName;
    explicit FunctionHandle(std::string name) : functionName(std::move(name)) {}
    void accept(ASTVisitor& visitor) override;
};

struct CellLiteral : Expression {
    std::vector<ExprPtr> elements;
    explicit CellLiteral(std::vector<ExprPtr> elems) : elements(std::move(elems)) {}
    void accept(ASTVisitor& visitor) override;
};

struct StringArrayLiteral : Expression {
    std::vector<ExprPtr> elements;
    explicit StringArrayLiteral(std::vector<ExprPtr> elems) : elements(std::move(elems)) {}
    void accept(ASTVisitor& visitor) override;
};

struct CellIndexExpression : Expression {
    ExprPtr cell;
    std::vector<ExprPtr> indices;
    CellIndexExpression(ExprPtr c, std::vector<ExprPtr> idx)
        : cell(std::move(c)), indices(std::move(idx)) {}
    void accept(ASTVisitor& visitor) override;
};

struct FieldAccessExpression : Expression {
    ExprPtr object;
    std::string fieldName;
    FieldAccessExpression(ExprPtr obj, std::string field)
        : object(std::move(obj)), fieldName(std::move(field)) {}
    void accept(ASTVisitor& visitor) override;
};

struct AssignmentStatement : Statement {
    std::string name;
    ExprPtr value;
    AssignmentStatement(std::string n, ExprPtr v)
        : name(std::move(n)), value(std::move(v)) {}
    void accept(ASTVisitor& visitor) override;
};

struct MultiAssignmentStatement : Statement {
    std::vector<std::string> names;
    std::vector<bool> isIgnored;
    ExprPtr value;
    MultiAssignmentStatement(std::vector<std::string> n, std::vector<bool> ign, ExprPtr v)
        : names(std::move(n)), isIgnored(std::move(ign)), value(std::move(v)) {}
    void accept(ASTVisitor& visitor) override;
};

struct IndexAssignmentStatement : Statement {
    std::string name;
    std::vector<ExprPtr> indices;
    ExprPtr value;
    IndexAssignmentStatement(std::string n, std::vector<ExprPtr> idx, ExprPtr v)
        : name(std::move(n)), indices(std::move(idx)), value(std::move(v)) {}
    void accept(ASTVisitor& visitor) override;
};

struct CellIndexAssignmentStatement : Statement {
    std::string name;
    std::vector<ExprPtr> indices;
    ExprPtr value;
    CellIndexAssignmentStatement(std::string n, std::vector<ExprPtr> idx, ExprPtr v)
        : name(std::move(n)), indices(std::move(idx)), value(std::move(v)) {}
    void accept(ASTVisitor& visitor) override;
};

struct FieldAssignmentStatement : Statement {
    ExprPtr object;
    std::string fieldName;
    ExprPtr value;
    FieldAssignmentStatement(ExprPtr obj, std::string field, ExprPtr v)
        : object(std::move(obj)), fieldName(std::move(field)), value(std::move(v)) {}
    void accept(ASTVisitor& visitor) override;
};

struct ExpressionStatement : Statement {
    ExprPtr expression;
    explicit ExpressionStatement(ExprPtr e) : expression(std::move(e)) {}
    void accept(ASTVisitor& visitor) override;
};

struct IfStatement : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> thenBranch;
    std::vector<std::pair<ExprPtr, std::vector<StmtPtr>>> elseifBranches;
    std::vector<StmtPtr> elseBranch;
    void accept(ASTVisitor& visitor) override;
};

struct WhileStatement : Statement {
    ExprPtr condition;
    std::vector<StmtPtr> body;
    void accept(ASTVisitor& visitor) override;
};

struct ForStatement : Statement {
    std::string variable;
    ExprPtr range;
    std::vector<StmtPtr> body;
    void accept(ASTVisitor& visitor) override;
};

struct ReturnStatement : Statement {
	ExprPtr value;
	explicit ReturnStatement(ExprPtr v = nullptr) : value(std::move(v)) {}
	void accept(ASTVisitor& visitor) override;
};

struct BreakStatement : Statement {
	void accept(ASTVisitor& visitor) override;
};

struct ContinueStatement : Statement {
	void accept(ASTVisitor& visitor) override;
};

struct FunctionDeclaration : Statement {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<StmtPtr> body;
    bool hasVarargin = false;
    bool hasVarargout = false;
    void accept(ASTVisitor& visitor) override;
};

struct CaseClause {
    std::vector<ExprPtr> values;
    std::vector<StmtPtr> body;
};

struct SwitchStatement : Statement {
    ExprPtr expression;
    std::vector<CaseClause> cases;
    std::vector<StmtPtr> otherwiseBranch;
    void accept(ASTVisitor& visitor) override;
};

struct TryCatchStatement : Statement {
    std::vector<StmtPtr> tryBody;
    std::string exceptionVar;
    std::vector<StmtPtr> catchBody;
    void accept(ASTVisitor& visitor) override;
};

struct GlobalStatement : Statement {
    std::vector<std::string> variables;
    explicit GlobalStatement(std::vector<std::string> vars) : variables(std::move(vars)) {}
    void accept(ASTVisitor& visitor) override;
};

struct Program : ASTNode {
    std::vector<StmtPtr> statements;
    void accept(ASTVisitor& visitor) override;
};

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(NumberLiteral& node) = 0;
    virtual void visit(ComplexLiteral& node) = 0;
    virtual void visit(StringLiteral& node) = 0;
    virtual void visit(CharArrayLiteral& node) = 0;
    virtual void visit(BooleanLiteral& node) = 0;
    virtual void visit(Identifier& node) = 0;
    virtual void visit(MatrixLiteral& node) = 0;
    virtual void visit(RangeExpression& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(CallExpression& node) = 0;
    virtual void visit(ColonExpression& node) = 0;
    virtual void visit(EndExpression& node) = 0;
    virtual void visit(IndexExpression& node) = 0;
    virtual void visit(AnonymousFunction& node) = 0;
    virtual void visit(FunctionHandle& node) = 0;
    virtual void visit(CellLiteral& node) = 0;
    virtual void visit(StringArrayLiteral& node) = 0;
    virtual void visit(CellIndexExpression& node) = 0;
    virtual void visit(FieldAccessExpression& node) = 0;
    virtual void visit(AssignmentStatement& node) = 0;
    virtual void visit(MultiAssignmentStatement& node) = 0;
    virtual void visit(IndexAssignmentStatement& node) = 0;
    virtual void visit(CellIndexAssignmentStatement& node) = 0;
    virtual void visit(FieldAssignmentStatement& node) = 0;
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(BreakStatement& node) = 0;
    virtual void visit(ContinueStatement& node) = 0;
    virtual void visit(FunctionDeclaration& node) = 0;
    virtual void visit(SwitchStatement& node) = 0;
    virtual void visit(TryCatchStatement& node) = 0;
    virtual void visit(GlobalStatement& node) = 0;
    virtual void visit(Program& node) = 0;
};

}
