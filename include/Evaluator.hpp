#pragma once

#include "AST.hpp"
#include "Matrix.hpp"
#include "DataTypes.hpp"
#include "../src/Plot/PlotManager.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <variant>
#include <stdexcept>
#include <vector>
#include <complex>
#include <memory>

namespace cnlab {

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string& msg) : std::runtime_error(msg) {}
};

using Complex = std::complex<double>;
// Value is now defined in DataTypes.hpp

struct FunctionDef {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<StmtPtr> body;
    bool isAnonymous = false;
    ExprPtr anonymousBody = nullptr;  // For anonymous functions
    bool hasVarargin = false;
    bool hasVarargout = false;
    std::unordered_map<std::string, Value> capturedVariables;  // For anonymous functions
};

class Evaluator : public ASTVisitor {
public:
    Evaluator();
    
    Value evaluate(Program& program);
    Value getResult() const { return result_; }
    void setVariable(const std::string& name, const Value& value);
    Value getVariable(const std::string& name) const;
    
    void visit(NumberLiteral& node) override;
    void visit(ComplexLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(CharArrayLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(Identifier& node) override;
    void visit(MatrixLiteral& node) override;
    void visit(RangeExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(CallExpression& node) override;
    void visit(ColonExpression& node) override;
    void visit(EndExpression& node) override;
    void visit(IndexExpression& node) override;
    void visit(AnonymousFunction& node) override;
    void visit(FunctionHandle& node) override;
    void visit(CellLiteral& node) override;
    void visit(StringArrayLiteral& node) override;
    void visit(FieldAccessExpression& node) override;
    void visit(CellIndexExpression& node) override;
    void visit(AssignmentStatement& node) override;
    void visit(MultiAssignmentStatement& node) override;
    void visit(IndexAssignmentStatement& node) override;
    void visit(CellIndexAssignmentStatement& node) override;
    void visit(FieldAssignmentStatement& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(SwitchStatement& node) override;
    void visit(TryCatchStatement& node) override;
    void visit(GlobalStatement& node) override;
    void visit(Program& node) override;

private:
    std::unordered_map<std::string, Value> variables_;
    std::unordered_map<std::string, FunctionDef> functions_;
    Value result_;
    bool returned_;
    bool break_;
    bool continue_;

    // Global variables support
    static std::unordered_map<std::string, Value> globalVariables_;
    std::unordered_set<std::string> currentScopeGlobals_;

    // Index context for 'end' keyword evaluation
    struct IndexContext {
        size_t rowEnd;      // Number of rows (for 2D indexing)
        size_t colEnd;      // Number of columns (for 2D indexing)
        size_t totalEnd;    // Total number of elements
        bool is2D;          // Whether this is 2D indexing
        size_t currentDim;  // Current dimension being evaluated (0 or 1)
    };
    std::vector<IndexContext> indexContextStack_;

    void pushIndexContext(size_t rows, size_t cols);
    void pushIndexContext(size_t totalSize);
    void popIndexContext();
    size_t getEndValue() const;

    Value evaluate(ExprPtr expr);
    void execute(StmtPtr stmt);

    static Matrix toMatrix(const Value& value);
    static double toDouble(const Value& value);
    static Complex toComplex(const Value& value);
    static bool toBool(const Value& value);
    static std::string toString(const Value& value);
    static std::string valueToString(const Value& value);
    static bool valuesEqual(const Value& a, const Value& b);

    friend class Interpreter;

    Value callFunction(const std::string& name, const std::vector<Value>& args);
    Value callUserFunction(const FunctionDef& funcDef, const std::vector<Value>& args);
    Value callBuiltinFunction(const std::string& name, const std::vector<Value>& args);
    bool isBuiltinFunction(const std::string& name);

    static Value add(const Value& left, const Value& right);
    static Value subtract(const Value& left, const Value& right);
    static Value multiply(const Value& left, const Value& right);
    static Value divide(const Value& left, const Value& right);
    static Value modulo(const Value& left, const Value& right);
    static Value power(const Value& left, const Value& right);
    static Value elementWiseAnd(const Value& left, const Value& right);
    static Value elementWiseOr(const Value& left, const Value& right);
    static Value elementWiseMul(const Value& left, const Value& right);
    static Value elementWiseDiv(const Value& left, const Value& right);
    static Value elementWisePow(const Value& left, const Value& right);
    static Value leftDivide(const Value& left, const Value& right);
    static Value compare(const Value& left, const Value& right, const std::string& op);

    std::unique_ptr<PlotManager> plotManager_;

};

}
