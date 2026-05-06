#include "Evaluator.hpp"
#include "GPUSimple.hpp"
#include "LinearAlgebra.hpp"
#include "FileIO.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <unordered_set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace cnlab {

// Static global variables storage
std::unordered_map<std::string, Value> Evaluator::globalVariables_;

Evaluator::Evaluator() : result_(0.0), returned_(false), break_(false), continue_(false) {
    variables_["pi"] = M_PI;
    variables_["eps"] = std::numeric_limits<double>::epsilon();
    variables_["inf"] = std::numeric_limits<double>::infinity();
    variables_["nan"] = std::numeric_limits<double>::quiet_NaN();
    plotManager_ = std::make_unique<PlotManager>();
}

// Index context management for 'end' keyword
void Evaluator::pushIndexContext(size_t rows, size_t cols) {
    indexContextStack_.push_back({rows, cols, rows * cols, true, 0});
}

void Evaluator::pushIndexContext(size_t totalSize) {
    indexContextStack_.push_back({0, 0, totalSize, false, 0});
}

void Evaluator::popIndexContext() {
    if (!indexContextStack_.empty()) {
        indexContextStack_.pop_back();
    }
}

size_t Evaluator::getEndValue() const {
    if (indexContextStack_.empty()) {
        throw RuntimeError("end can only be used in indexing");
    }
    const auto& ctx = indexContextStack_.back();
    if (ctx.is2D) {
        // For 2D indexing, return rowEnd for first dimension, colEnd for second
        return (ctx.currentDim == 0) ? ctx.rowEnd : ctx.colEnd;
    }
    return ctx.totalEnd;
}

Value Evaluator::evaluate(Program& program) {
    returned_ = false;
    program.accept(*this);
    return result_;
}

void Evaluator::setVariable(const std::string& name, const Value& value) {
    // Check if this variable is declared as global in current scope
    if (currentScopeGlobals_.find(name) != currentScopeGlobals_.end()) {
        globalVariables_[name] = value;
        return;
    }
    variables_[name] = value;
}

Value Evaluator::getVariable(const std::string& name) const {
    // Check if this variable is declared as global in current scope
    if (currentScopeGlobals_.find(name) != currentScopeGlobals_.end()) {
        auto it = globalVariables_.find(name);
        if (it == globalVariables_.end()) {
            throw RuntimeError("Undefined global variable: " + name);
        }
        return it->second;
    }
    auto it = variables_.find(name);
    if (it == variables_.end()) {
        throw RuntimeError("Undefined variable: " + name);
    }
    return it->second;
}

Value Evaluator::evaluate(ExprPtr expr) {
    expr->accept(*this);
    return result_;
}

void Evaluator::execute(StmtPtr stmt) {
	if (returned_ || break_ || continue_) return;
	stmt->accept(*this);
}

void Evaluator::visit(NumberLiteral& node) {
    result_ = node.value;
}

void Evaluator::visit(ComplexLiteral& node) {
    result_ = Complex(node.real, node.imag);
}

void Evaluator::visit(StringLiteral& node) {
    result_ = node.value;
}

void Evaluator::visit(CharArrayLiteral& node) {
    result_ = std::make_shared<CharArray>(node.value);
}

void Evaluator::visit(BooleanLiteral& node) {
    result_ = node.value;
}

void Evaluator::visit(ColonExpression& node) {
    // Colon expression is handled specially in IndexExpression
    // It represents "all elements" in that dimension
    throw RuntimeError("Colon expression can only be used in indexing");
}

void Evaluator::visit(EndExpression& node) {
    // End expression represents the last index in the current dimension
    // It requires an index context to be set up by visit(IndexExpression)
    result_ = static_cast<double>(getEndValue());
}

void Evaluator::visit(AnonymousFunction& node) {
    // Create a unique name for this anonymous function
    static int anonFuncCounter = 0;
    std::string funcName = "__anon_" + std::to_string(anonFuncCounter++);

    // Store the anonymous function definition
    FunctionDef funcDef;
    funcDef.name = funcName;
    funcDef.parameters = node.parameters;
    funcDef.isAnonymous = true;
    funcDef.anonymousBody = node.body;
    // Capture current variables (for closures)
    funcDef.capturedVariables = variables_;
    functions_[funcName] = std::move(funcDef);

    // Return a function handle
    result_ = std::make_shared<FuncHandle>(node.parameters, funcName);
}

void Evaluator::visit(FunctionHandle& node) {
    // Check if the function exists
    if (functions_.find(node.functionName) == functions_.end() &&
        !isBuiltinFunction(node.functionName)) {
        throw RuntimeError("Undefined function: " + node.functionName);
    }
    // Return a function handle object
    result_ = std::make_shared<FuncHandle>(node.functionName);
}

void Evaluator::visit(FieldAccessExpression& node) {
    Value obj = evaluate(node.object);
    
    // Support struct field access
    if (std::holds_alternative<std::shared_ptr<StructArray>>(obj)) {
        auto s = std::get<std::shared_ptr<StructArray>>(obj);
        result_ = s->getField(node.fieldName);
        return;
    }
    
    // Support table column access
    if (std::holds_alternative<std::shared_ptr<Table>>(obj)) {
        auto t = std::get<std::shared_ptr<Table>>(obj);
        result_ = t->getColumn(node.fieldName);
        return;
    }
    
    throw RuntimeError("Can only access fields on struct or columns on table");
}

void Evaluator::visit(CellLiteral& node) {
    auto cell = std::make_shared<Cell>();
    for (const auto& elem : node.elements) {
        cell->append(evaluate(elem));
    }
    result_ = cell;
}

void Evaluator::visit(StringArrayLiteral& node) {
    std::vector<std::string> strings;
    for (const auto& elem : node.elements) {
        Value val = evaluate(elem);
        strings.push_back(toString(val));
    }
    result_ = std::make_shared<StringArray>(strings);
}

void Evaluator::visit(CellIndexExpression& node) {
    Value cellValue = evaluate(node.cell);
    
    if (!std::holds_alternative<std::shared_ptr<Cell>>(cellValue)) {
        throw RuntimeError("Can only index cell arrays with {}");
    }
    
    auto cell = std::get<std::shared_ptr<Cell>>(cellValue);
    
    if (node.indices.size() == 1) {
        // Single index - linear indexing
        size_t index = static_cast<size_t>(toDouble(evaluate(node.indices[0])));
        result_ = cell->getElement(index);
    } else if (node.indices.size() == 2) {
        // Two indices - row, column indexing
        size_t row = static_cast<size_t>(toDouble(evaluate(node.indices[0])));
        size_t col = static_cast<size_t>(toDouble(evaluate(node.indices[1])));
        result_ = cell->getElement(row, col);
    } else {
        throw RuntimeError("Cell array indexing supports 1 or 2 indices");
    }
}

void Evaluator::visit(Identifier& node) {
    result_ = getVariable(node.name);
}

void Evaluator::visit(MatrixLiteral& node) {
    if (node.rows.empty()) {
        result_ = Matrix(0, 0);
        return;
    }
    
    // First pass: evaluate all elements and check for range expressions
    std::vector<std::vector<Value>> evaluatedRows;
    size_t totalCols = 0;
    
    for (const auto& row : node.rows) {
        std::vector<Value> evaluatedRow;
        for (const auto& elem : row) {
            Value val = evaluate(elem);
            evaluatedRow.push_back(val);
        }
        evaluatedRows.push_back(evaluatedRow);
        if (evaluatedRow.size() > totalCols) {
            totalCols = evaluatedRow.size();
        }
    }
    
    // Second pass: flatten any range expressions and build final matrix
    std::vector<double> flatData;
    size_t finalRows = evaluatedRows.size();
    
    for (size_t i = 0; i < evaluatedRows.size(); ++i) {
        for (size_t j = 0; j < evaluatedRows[i].size(); ++j) {
            Value& val = evaluatedRows[i][j];
            
            if (std::holds_alternative<Matrix>(val)) {
                Matrix& m = std::get<Matrix>(val);
                // Check if this is a row vector (1 x n)
                if (m.rows() == 1) {
                    // Flatten row vector into the matrix row
                    for (size_t k = 0; k < m.cols(); ++k) {
                        flatData.push_back(m(0, k));
                    }
                    // Update column count
                    if (j == 0) {
                        totalCols = m.cols();
                    }
                } else if (m.cols() == 1) {
                    // Column vector
                    flatData.push_back(m(0, 0));
                } else {
                    // Regular matrix element
                    flatData.push_back(m(0, 0));
                }
            } else if (std::holds_alternative<double>(val)) {
                flatData.push_back(std::get<double>(val));
            } else {
                throw RuntimeError("Cannot convert matrix element to scalar");
            }
        }
    }
    
    // Build final matrix
    if (flatData.empty()) {
        result_ = Matrix(0, 0);
        return;
    }
    
    // If we have a single row with range expressions, create a row vector
    if (finalRows == 1) {
        Matrix mat(1, flatData.size());
        for (size_t i = 0; i < flatData.size(); ++i) {
            mat(0, i) = flatData[i];
        }
        result_ = mat;
    } else {
        // Multi-row matrix
        Matrix mat(finalRows, totalCols);
        size_t idx = 0;
        for (size_t i = 0; i < finalRows && idx < flatData.size(); ++i) {
            for (size_t j = 0; j < totalCols && idx < flatData.size(); ++j) {
                mat(i, j) = flatData[idx++];
            }
        }
        result_ = mat;
    }
}

void Evaluator::visit(RangeExpression& node) {
    double start = toDouble(evaluate(node.start));
    double step = node.step ? toDouble(evaluate(node.step)) : 1.0;
    double end = node.end ? toDouble(evaluate(node.end)) : start;
    
    if (step == 0) {
        throw RuntimeError("Range step cannot be zero");
    }
    
    std::vector<double> values;
    if (step > 0) {
        for (double v = start; v <= end; v += step) {
            values.push_back(v);
        }
    } else {
        for (double v = start; v >= end; v += step) {
            values.push_back(v);
        }
    }
    
    Matrix mat(1, values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        mat(0, i) = values[i];
    }
    
    result_ = mat;
}

void Evaluator::visit(BinaryExpression& node) {
    Value left = evaluate(node.left);
    Value right = evaluate(node.right);
    
    if (node.op == "+") {
        result_ = add(left, right);
    } else if (node.op == "-") {
        result_ = subtract(left, right);
    } else if (node.op == "*") {
        result_ = multiply(left, right);
    } else if (node.op == "/") {
        result_ = divide(left, right);
    } else if (node.op == "%") {
        result_ = modulo(left, right);
    } else if (node.op == "^") {
        result_ = power(left, right);
    } else if (node.op == "==") {
        result_ = compare(left, right, "==");
    } else if (node.op == "~=") {
        result_ = compare(left, right, "~=");
    } else if (node.op == "<") {
        result_ = compare(left, right, "<");
    } else if (node.op == ">") {
        result_ = compare(left, right, ">");
    } else if (node.op == "<=") {
        result_ = compare(left, right, "<=");
    } else if (node.op == ">=") {
        result_ = compare(left, right, ">=");
    } else if (node.op == "&&") {
        result_ = (toBool(left) && toBool(right));
    } else if (node.op == "||") {
        result_ = (toBool(left) || toBool(right));
    } else if (node.op == "&") {
        result_ = elementWiseAnd(left, right);
    } else if (node.op == "|") {
        result_ = elementWiseOr(left, right);
    } else if (node.op == ".*") {
        result_ = elementWiseMul(left, right);
    } else if (node.op == "./") {
        result_ = elementWiseDiv(left, right);
    } else if (node.op == ".^") {
        result_ = elementWisePow(left, right);
    } else if (node.op == "\\") {
        result_ = leftDivide(left, right);
    } else {
        throw RuntimeError("Unknown operator: " + node.op);
    }
}

void Evaluator::visit(UnaryExpression& node) {
    Value operand = evaluate(node.operand);
    
    if (node.op == "-") {
        if (std::holds_alternative<double>(operand)) {
            result_ = -std::get<double>(operand);
        } else if (std::holds_alternative<Matrix>(operand)) {
            result_ = -std::get<Matrix>(operand);
        }
    } else if (node.op == "+") {
        result_ = operand;
    } else if (node.op == "~") {
        result_ = !toBool(operand);
    } else {
        throw RuntimeError("Unknown unary operator: " + node.op);
    }
}

void Evaluator::visit(CallExpression& node) {
    std::string funcName;
    if (auto* id = dynamic_cast<Identifier*>(node.callee.get())) {
        std::string name = id->name;
        // Check if this identifier is a variable containing a function handle
        auto it = variables_.find(name);
        if (it != variables_.end()) {
            // It's a variable, check if it's a function handle (string or FuncHandle)
            if (std::holds_alternative<std::string>(it->second)) {
                funcName = std::get<std::string>(it->second);
            } else if (std::holds_alternative<std::shared_ptr<FuncHandle>>(it->second)) {
                funcName = std::get<std::shared_ptr<FuncHandle>>(it->second)->name();
            } else {
                throw RuntimeError("Variable '" + name + "' is not a function handle");
            }
        } else {
            // Not a variable, treat as function name
            funcName = name;
        }
    } else if (auto* strLit = dynamic_cast<StringLiteral*>(node.callee.get())) {
        // Function handle stored as string
        funcName = strLit->value;
    } else if (auto* funcHandle = dynamic_cast<cnlab::FunctionHandle*>(node.callee.get())) {
        // Direct function handle: @sin(3.14)
        funcName = funcHandle->functionName;
    } else {
        throw RuntimeError("Callee must be an identifier or function handle");
    }

    std::vector<Value> args;
    for (auto& arg : node.arguments) {
        args.push_back(evaluate(arg));
    }

    result_ = callFunction(funcName, args);
}

void Evaluator::visit(IndexExpression& node) {
    // First check if this is a function call disguised as indexing
    if (auto* id = dynamic_cast<Identifier*>(node.object.get())) {
        auto it = variables_.find(id->name);
        if (it == variables_.end()) {
            // Not a variable, treat as function name
            std::vector<Value> args;
            for (auto& arg : node.indices) {
                args.push_back(evaluate(arg));
            }
            result_ = callFunction(id->name, args);
            return;
        } else if (std::holds_alternative<std::shared_ptr<StructArray>>(it->second)) {
            // Struct array indexing: s(1)
            auto s = std::get<std::shared_ptr<StructArray>>(it->second);
            if (node.indices.size() != 1) {
                throw RuntimeError("Struct array indexing requires exactly one index");
            }
            size_t idx = static_cast<size_t>(toDouble(evaluate(node.indices[0])));
            result_ = s->getElement(idx);
            return;
        } else if (std::holds_alternative<std::shared_ptr<FuncHandle>>(it->second)) {
            // Function handle stored in variable
            std::string funcName = std::get<std::shared_ptr<FuncHandle>>(it->second)->name();
            std::vector<Value> args;
            for (auto& arg : node.indices) {
                args.push_back(evaluate(arg));
            }
            result_ = callFunction(funcName, args);
            return;
        } else if (!std::holds_alternative<Matrix>(it->second)) {
            // It's a variable but not a matrix - check if it's a string function handle (legacy)
            if (std::holds_alternative<std::string>(it->second)) {
                // Function handle stored in variable
                std::string funcName = std::get<std::string>(it->second);
                std::vector<Value> args;
                for (auto& arg : node.indices) {
                    args.push_back(evaluate(arg));
                }
                result_ = callFunction(funcName, args);
                return;
            } else {
                throw RuntimeError("Variable '" + id->name + "' is not a matrix or function handle");
            }
        }
    }

    Value obj = evaluate(node.object);
    
    // Handle struct array indexing on expression result
    if (std::holds_alternative<std::shared_ptr<StructArray>>(obj)) {
        auto s = std::get<std::shared_ptr<StructArray>>(obj);
        if (node.indices.size() != 1) {
            throw RuntimeError("Struct array indexing requires exactly one index");
        }
        size_t idx = static_cast<size_t>(toDouble(evaluate(node.indices[0])));
        result_ = s->getElement(idx);
        return;
    }
    
    if (!std::holds_alternative<Matrix>(obj)) {
        throw RuntimeError("Can only index matrices");
    }

    Matrix& mat = std::get<Matrix>(obj);

    if (node.indices.size() == 1) {
        // Single index - use total size as end value
        pushIndexContext(mat.size());
        
        // Check if it's a colon (all elements)
        if (dynamic_cast<ColonExpression*>(node.indices[0].get())) {
            popIndexContext();
            // Return all elements as a column vector
            Matrix result(mat.size(), 1);
            for (size_t i = 0; i < mat.size(); ++i) {
                result(i, 0) = mat(i);
            }
            result_ = result;
            return;
        }
        
        // Check if it's a range expression
        if (dynamic_cast<RangeExpression*>(node.indices[0].get())) {
            auto* rangeExpr = dynamic_cast<RangeExpression*>(node.indices[0].get());
            double startVal = toDouble(evaluate(rangeExpr->start));
            double stepVal = rangeExpr->step ? toDouble(evaluate(rangeExpr->step)) : 1.0;
            double endVal = rangeExpr->end ? toDouble(evaluate(rangeExpr->end)) : mat.size();
            popIndexContext();
            
            std::vector<double> indices;
            if (stepVal > 0) {
                for (double v = startVal; v <= endVal; v += stepVal) {
                    indices.push_back(v);
                }
            } else {
                for (double v = startVal; v >= endVal; v += stepVal) {
                    indices.push_back(v);
                }
            }
            
            Matrix result(indices.size(), 1);
            for (size_t i = 0; i < indices.size(); ++i) {
                size_t idx = static_cast<size_t>(indices[i]) - 1;
                if (idx < mat.size()) {
                    result(i, 0) = mat(idx);
                }
            }
            result_ = result;
            return;
        }
        
        // Evaluate the index expression (with end context)
        Value idxValue = evaluate(node.indices[0]);
        popIndexContext();
        
        // Check if it's a logical matrix (for logical indexing)
        if (std::holds_alternative<Matrix>(idxValue)) {
            Matrix& logicalMat = std::get<Matrix>(idxValue);
            // Count true elements
            size_t count = 0;
            for (size_t i = 0; i < logicalMat.rows(); ++i) {
                for (size_t j = 0; j < logicalMat.cols(); ++j) {
                    if (logicalMat(i, j) != 0) {
                        count++;
                    }
                }
            }
            // Create result with matching elements
            Matrix result(count, 1);
            size_t idx = 0;
            for (size_t i = 0; i < mat.size() && idx < count; ++i) {
                if (i < logicalMat.size() && logicalMat(i) != 0) {
                    result(idx, 0) = mat(i);
                    idx++;
                }
            }
            result_ = result;
            return;
        }
        
        size_t idx = static_cast<size_t>(toDouble(idxValue)) - 1;
        result_ = mat(idx);
    } else if (node.indices.size() == 2) {
        // Two-dimensional indexing - use rows/cols as end values
        pushIndexContext(mat.rows(), mat.cols());
        
        bool rowIsColon = dynamic_cast<ColonExpression*>(node.indices[0].get());
        bool colIsColon = dynamic_cast<ColonExpression*>(node.indices[1].get());
        bool rowIsRange = dynamic_cast<RangeExpression*>(node.indices[0].get());
        bool colIsRange = dynamic_cast<RangeExpression*>(node.indices[1].get());

        if (rowIsColon && colIsColon) {
            popIndexContext();
            // A(:,:) - return all elements as column vector
            Matrix result(mat.size(), 1);
            for (size_t i = 0; i < mat.size(); ++i) {
                result(i, 0) = mat(i);
            }
            result_ = result;
        } else if (rowIsColon) {
            // A(:, j) or A(:, j:k) - return entire column(s)
            // Set current dimension to 1 (columns)
            if (!indexContextStack_.empty()) {
                indexContextStack_.back().currentDim = 1;
            }
            
            if (colIsRange) {
                // A(:, j:k)
                auto* rangeExpr = dynamic_cast<RangeExpression*>(node.indices[1].get());
                double startCol = toDouble(evaluate(rangeExpr->start));
                double stepCol = rangeExpr->step ? toDouble(evaluate(rangeExpr->step)) : 1.0;
                double endCol = rangeExpr->end ? toDouble(evaluate(rangeExpr->end)) : mat.cols();
                popIndexContext();
                
                std::vector<size_t> colIndices;
                if (stepCol > 0) {
                    for (double v = startCol; v <= endCol; v += stepCol) {
                        colIndices.push_back(static_cast<size_t>(v) - 1);
                    }
                } else {
                    for (double v = startCol; v >= endCol; v += stepCol) {
                        colIndices.push_back(static_cast<size_t>(v) - 1);
                    }
                }
                
                Matrix result(mat.rows(), colIndices.size());
                for (size_t i = 0; i < mat.rows(); ++i) {
                    for (size_t j = 0; j < colIndices.size(); ++j) {
                        result(i, j) = mat(i, colIndices[j]);
                    }
                }
                result_ = result;
            } else {
                // A(:, j)
                size_t col = static_cast<size_t>(toDouble(evaluate(node.indices[1]))) - 1;
                popIndexContext();
                Matrix result(mat.rows(), 1);
                for (size_t i = 0; i < mat.rows(); ++i) {
                    result(i, 0) = mat(i, col);
                }
                result_ = result;
            }
        } else if (colIsColon) {
            // A(i, :) or A(i:j, :) - return entire row(s)
            // Set current dimension to 0 (rows)
            if (!indexContextStack_.empty()) {
                indexContextStack_.back().currentDim = 0;
            }
            
            if (rowIsRange) {
                // A(i:j, :)
                auto* rangeExpr = dynamic_cast<RangeExpression*>(node.indices[0].get());
                double startRow = toDouble(evaluate(rangeExpr->start));
                double stepRow = rangeExpr->step ? toDouble(evaluate(rangeExpr->step)) : 1.0;
                double endRow = rangeExpr->end ? toDouble(evaluate(rangeExpr->end)) : mat.rows();
                popIndexContext();
                
                std::vector<size_t> rowIndices;
                if (stepRow > 0) {
                    for (double v = startRow; v <= endRow; v += stepRow) {
                        rowIndices.push_back(static_cast<size_t>(v) - 1);
                    }
                } else {
                    for (double v = startRow; v >= endRow; v += stepRow) {
                        rowIndices.push_back(static_cast<size_t>(v) - 1);
                    }
                }
                
                Matrix result(rowIndices.size(), mat.cols());
                for (size_t i = 0; i < rowIndices.size(); ++i) {
                    for (size_t j = 0; j < mat.cols(); ++j) {
                        result(i, j) = mat(rowIndices[i], j);
                    }
                }
                result_ = result;
            } else {
                // A(i, :)
                size_t row = static_cast<size_t>(toDouble(evaluate(node.indices[0]))) - 1;
                popIndexContext();
                Matrix result(1, mat.cols());
                for (size_t j = 0; j < mat.cols(); ++j) {
                    result(0, j) = mat(row, j);
                }
                result_ = result;
            }
        } else if (rowIsRange || colIsRange) {
            // A(i:j, k) or A(i, j:k) or A(i:j, k:l)
            // Handle row range
            std::vector<size_t> rowIndices;
            if (rowIsRange) {
                auto* rangeExpr = dynamic_cast<RangeExpression*>(node.indices[0].get());
                if (!indexContextStack_.empty()) {
                    indexContextStack_.back().currentDim = 0;
                }
                double startRow = toDouble(evaluate(rangeExpr->start));
                double stepRow = rangeExpr->step ? toDouble(evaluate(rangeExpr->step)) : 1.0;
                double endRow = rangeExpr->end ? toDouble(evaluate(rangeExpr->end)) : mat.rows();
                
                if (stepRow > 0) {
                    for (double v = startRow; v <= endRow; v += stepRow) {
                        rowIndices.push_back(static_cast<size_t>(v) - 1);
                    }
                } else {
                    for (double v = startRow; v >= endRow; v += stepRow) {
                        rowIndices.push_back(static_cast<size_t>(v) - 1);
                    }
                }
            } else {
                if (!indexContextStack_.empty()) {
                    indexContextStack_.back().currentDim = 0;
                }
                rowIndices.push_back(static_cast<size_t>(toDouble(evaluate(node.indices[0]))) - 1);
            }
            
            // Handle column range
            std::vector<size_t> colIndices;
            if (colIsRange) {
                auto* rangeExpr = dynamic_cast<RangeExpression*>(node.indices[1].get());
                if (!indexContextStack_.empty()) {
                    indexContextStack_.back().currentDim = 1;
                }
                double startCol = toDouble(evaluate(rangeExpr->start));
                double stepCol = rangeExpr->step ? toDouble(evaluate(rangeExpr->step)) : 1.0;
                double endCol = rangeExpr->end ? toDouble(evaluate(rangeExpr->end)) : mat.cols();
                
                if (stepCol > 0) {
                    for (double v = startCol; v <= endCol; v += stepCol) {
                        colIndices.push_back(static_cast<size_t>(v) - 1);
                    }
                } else {
                    for (double v = startCol; v >= endCol; v += stepCol) {
                        colIndices.push_back(static_cast<size_t>(v) - 1);
                    }
                }
            } else {
                if (!indexContextStack_.empty()) {
                    indexContextStack_.back().currentDim = 1;
                }
                colIndices.push_back(static_cast<size_t>(toDouble(evaluate(node.indices[1]))) - 1);
            }
            
            popIndexContext();
            
            Matrix result(rowIndices.size(), colIndices.size());
            for (size_t i = 0; i < rowIndices.size(); ++i) {
                for (size_t j = 0; j < colIndices.size(); ++j) {
                    result(i, j) = mat(rowIndices[i], colIndices[j]);
                }
            }
            result_ = result;
        } else {
            // A(i, j) - single element
            // First evaluate row index (dimension 0)
            if (!indexContextStack_.empty()) {
                indexContextStack_.back().currentDim = 0;
            }
            size_t row = static_cast<size_t>(toDouble(evaluate(node.indices[0]))) - 1;
            // Then evaluate column index (dimension 1)
            if (!indexContextStack_.empty()) {
                indexContextStack_.back().currentDim = 1;
            }
            size_t col = static_cast<size_t>(toDouble(evaluate(node.indices[1]))) - 1;
            popIndexContext();
            result_ = mat(row, col);
        }
    } else {
        throw RuntimeError("Invalid number of indices");
    }
}

void Evaluator::visit(AssignmentStatement& node) {
    Value value = evaluate(node.value);
    
    // If value is MultiValue, extract the first element for single assignment
    if (std::holds_alternative<std::shared_ptr<MultiValue>>(value)) {
        auto multiVal = std::get<std::shared_ptr<MultiValue>>(value);
        if (!multiVal->values().empty()) {
            value = multiVal->values()[0];
        }
    }
    
    setVariable(node.name, value);
    result_ = value;
}

void Evaluator::visit(MultiAssignmentStatement& node) {
    Value value = evaluate(node.value);
    
    std::vector<Value> valuesToAssign;
    
    if (std::holds_alternative<std::shared_ptr<MultiValue>>(value)) {
        auto multiVal = std::get<std::shared_ptr<MultiValue>>(value);
        for (const auto& v : multiVal->values()) {
            valuesToAssign.push_back(v);
        }
    } else {
        valuesToAssign.push_back(value);
    }
    
    for (size_t i = 0; i < node.names.size(); ++i) {
        if (node.isIgnored[i]) {
            continue;
        }
        if (i < valuesToAssign.size()) {
            setVariable(node.names[i], valuesToAssign[i]);
        } else {
            setVariable(node.names[i], std::monostate{});
        }
    }
    
    result_ = value;
}

void Evaluator::visit(IndexAssignmentStatement& node) {
    Value newValue = evaluate(node.value);
    Value varValue = getVariable(node.name);
    
    if (!std::holds_alternative<Matrix>(varValue)) {
        throw RuntimeError("Can only index assign to matrices");
    }
    
    Matrix mat = std::get<Matrix>(varValue);
    
    if (node.indices.size() == 1) {
        Value idxValue = evaluate(node.indices[0]);
        
        // Check if it's a logical matrix (for logical indexing)
        if (std::holds_alternative<Matrix>(idxValue)) {
            Matrix& logicalMat = std::get<Matrix>(idxValue);
            double val = toDouble(newValue);
            
            // Assign value to all positions where logical matrix is true (non-zero)
            for (size_t i = 0; i < mat.size() && i < logicalMat.size(); ++i) {
                if (logicalMat(i) != 0) {
                    mat(i) = val;
                }
            }
            
            setVariable(node.name, mat);
            result_ = val;
            return;
        }
        
        // Regular numeric indexing
        double val = toDouble(newValue);
        size_t idx = static_cast<size_t>(toDouble(idxValue)) - 1;
        mat(idx) = val;
        result_ = val;
    } else if (node.indices.size() == 2) {
        double val = toDouble(newValue);
        size_t row = static_cast<size_t>(toDouble(evaluate(node.indices[0]))) - 1;
        size_t col = static_cast<size_t>(toDouble(evaluate(node.indices[1]))) - 1;
        mat(row, col) = val;
        result_ = val;
    } else {
        throw RuntimeError("Invalid number of indices for assignment");
    }
    
    setVariable(node.name, mat);
}

void Evaluator::visit(CellIndexAssignmentStatement& node) {
    Value newValue = evaluate(node.value);
    Value varValue;
    
    // Get or create the cell array
    try {
        varValue = getVariable(node.name);
    } catch (const RuntimeError&) {
        // Variable doesn't exist, create a new cell array
        varValue = std::make_shared<Cell>();
    }
    
    if (!std::holds_alternative<std::shared_ptr<Cell>>(varValue)) {
        throw RuntimeError("Can only cell index assign to cell arrays");
    }
    
    auto cell = std::get<std::shared_ptr<Cell>>(varValue);
    
    if (node.indices.size() == 1) {
        // Single index - linear indexing
        size_t idx = static_cast<size_t>(toDouble(evaluate(node.indices[0])));
        cell->setElement(idx, newValue);
    } else if (node.indices.size() == 2) {
        // Two indices - row, column indexing
        size_t row = static_cast<size_t>(toDouble(evaluate(node.indices[0])));
        size_t col = static_cast<size_t>(toDouble(evaluate(node.indices[1])));
        cell->setElement(row, col, newValue);
    } else {
        throw RuntimeError("Cell array assignment supports 1 or 2 indices");
    }
    
    setVariable(node.name, cell);
    result_ = newValue;
}

void Evaluator::visit(FieldAssignmentStatement& node) {
	Value value = evaluate(node.value);
	
	// Get the variable name from the object (should be an identifier or indexed expression)
	std::string varName;
	ExprPtr objExpr = node.object;
	
	// Check if this is struct array element field assignment: s(1).field = value
	if (auto* idxExpr = dynamic_cast<IndexExpression*>(node.object.get())) {
		if (auto* id = dynamic_cast<Identifier*>(idxExpr->object.get())) {
			varName = id->name;
			
			// Get the struct array
			Value varValue;
			try {
				varValue = getVariable(varName);
			} catch (const RuntimeError&) {
				// Variable doesn't exist, create a new struct array
				varValue = std::make_shared<StructArray>();
			}
			
			if (std::holds_alternative<std::shared_ptr<StructArray>>(varValue)) {
				auto arr = std::get<std::shared_ptr<StructArray>>(varValue);
				
				// Get the index
				if (idxExpr->indices.size() != 1) {
					throw RuntimeError("Struct array indexing requires exactly one index");
				}
				size_t idx = static_cast<size_t>(toDouble(evaluate(idxExpr->indices[0])));
				
				// Get or create the element
				std::shared_ptr<StructArray> elem;
				if (idx <= arr->size()) {
					elem = arr->getElement(idx);
				} else {
					elem = std::make_shared<StructArray>();
				}
				
				// Set the field
				elem->setField(node.fieldName, value);
				arr->setElement(idx, elem);
				setVariable(varName, arr);
				result_ = value;
				return;
			}
		}
		throw RuntimeError("Can only assign fields to struct array elements");
	}
	
	if (auto* id = dynamic_cast<Identifier*>(node.object.get())) {
		varName = id->name;
	} else {
		throw RuntimeError("Can only assign fields to variable");
	}
	
	// Get or create the struct or table
	Value varValue;
	try {
		varValue = getVariable(varName);
	} catch (const RuntimeError&) {
		// Variable doesn't exist, create a new struct by default
		varValue = std::make_shared<StructArray>();
	}
	
	// Handle struct field assignment
	if (std::holds_alternative<std::shared_ptr<StructArray>>(varValue)) {
		auto s = std::get<std::shared_ptr<StructArray>>(varValue);
		s->setField(node.fieldName, value);
		setVariable(varName, s);
		result_ = value;
		return;
	}
	
	// Handle table column assignment
	if (std::holds_alternative<std::shared_ptr<Table>>(varValue)) {
		auto t = std::get<std::shared_ptr<Table>>(varValue);
		t->addColumn(node.fieldName, value);
		setVariable(varName, t);
		result_ = value;
		return;
	}
	
	throw RuntimeError("Can only assign fields to struct or columns to table");
}

void Evaluator::visit(ExpressionStatement& node) {
    result_ = evaluate(node.expression);
}

void Evaluator::visit(IfStatement& node) {
    if (toBool(evaluate(node.condition))) {
        for (auto& stmt : node.thenBranch) {
            execute(stmt);
            if (returned_) return;
        }
    } else {
        bool executed = false;
        for (auto& elseif : node.elseifBranches) {
            if (toBool(evaluate(elseif.first))) {
                for (auto& stmt : elseif.second) {
                    execute(stmt);
                    if (returned_) return;
                }
                executed = true;
                break;
            }
        }
        
        if (!executed) {
            for (auto& stmt : node.elseBranch) {
                execute(stmt);
                if (returned_) return;
            }
        }
    }
}

void Evaluator::visit(WhileStatement& node) {
    while (toBool(evaluate(node.condition))) {
        for (auto& stmt : node.body) {
            execute(stmt);
            if (returned_) return;
            if (break_) {
                break_ = false;
                return;
            }
            if (continue_) {
                continue_ = false;
                break;
            }
        }
    }
}

void Evaluator::visit(ForStatement& node) {
	Value rangeVal = evaluate(node.range);
	
	if (!std::holds_alternative<Matrix>(rangeVal)) {
		throw RuntimeError("For loop range must be a matrix");
	}
	
	Matrix& range = std::get<Matrix>(rangeVal);
	
	for (size_t i = 0; i < range.size(); ++i) {
		setVariable(node.variable, range(i));
		for (auto& stmt : node.body) {
			execute(stmt);
			if (returned_) return;
			if (break_) {
				break_ = false;
				return;
			}
			if (continue_) {
				continue_ = false;
				break;
			}
		}
	}
}

void Evaluator::visit(ReturnStatement& node) {
	if (node.value) {
		result_ = evaluate(node.value);
	}
	returned_ = true;
}

void Evaluator::visit(BreakStatement& node) {
	break_ = true;
}

void Evaluator::visit(ContinueStatement& node) {
	continue_ = true;
}

void Evaluator::visit(FunctionDeclaration& node) {
    FunctionDef funcDef;
    funcDef.name = node.name;
    funcDef.parameters = node.parameters;
    funcDef.body = node.body;
    funcDef.hasVarargin = node.hasVarargin;
    funcDef.hasVarargout = node.hasVarargout;
    functions_[node.name] = std::move(funcDef);
}

void Evaluator::visit(SwitchStatement& node) {
    Value switchValue = evaluate(node.expression);
    bool matched = false;
    
    for (auto& caseClause : node.cases) {
        for (auto& caseValue : caseClause.values) {
            Value caseVal = evaluate(caseValue);
            if (valuesEqual(switchValue, caseVal)) {
                matched = true;
                for (auto& stmt : caseClause.body) {
                    execute(stmt);
                    if (returned_ || break_ || continue_) return;
                }
                break;
            }
        }
        if (matched) break;
    }
    
    // Execute otherwise branch if no case matched
    if (!matched) {
        for (auto& stmt : node.otherwiseBranch) {
            execute(stmt);
            if (returned_ || break_ || continue_) return;
        }
    }
}

void Evaluator::visit(TryCatchStatement& node) {
    bool exceptionOccurred = false;
    std::string errorMessage;
    
    // Execute try body
    for (auto& stmt : node.tryBody) {
        try {
            execute(stmt);
            if (returned_ || break_ || continue_) return;
        } catch (const RuntimeError& e) {
            exceptionOccurred = true;
            errorMessage = e.what();
            break;
        } catch (const std::exception& e) {
            exceptionOccurred = true;
            errorMessage = e.what();
            break;
        }
    }
    
    // Execute catch body if exception occurred
    if (exceptionOccurred) {
        // Store exception info in the exception variable if specified
        if (!node.exceptionVar.empty()) {
            setVariable(node.exceptionVar, errorMessage);
        }
        
        for (auto& stmt : node.catchBody) {
            execute(stmt);
            if (returned_ || break_ || continue_) return;
        }
    }
}

void Evaluator::visit(GlobalStatement& node) {
    for (const auto& varName : node.variables) {
        // Add variable to current scope's global set
        currentScopeGlobals_.insert(varName);
        // If global variable already exists, sync it to current scope
        auto it = globalVariables_.find(varName);
        if (it != globalVariables_.end()) {
            variables_[varName] = it->second;
        }
    }
}

void Evaluator::visit(Program& node) {
    for (auto& stmt : node.statements) {
        execute(stmt);
        if (returned_) break;
    }
}

Matrix Evaluator::toMatrix(const Value& value) {
    if (std::holds_alternative<Matrix>(value)) {
        return std::get<Matrix>(value);
    }
    if (std::holds_alternative<double>(value)) {
        Matrix m(1, 1);
        m(0, 0) = std::get<double>(value);
        return m;
    }
    throw RuntimeError("Cannot convert to matrix");
}

double Evaluator::toDouble(const Value& value) {
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value);
    }
    if (std::holds_alternative<Matrix>(value)) {
        const Matrix& m = std::get<Matrix>(value);
        if (m.isScalar()) {
            return m(0, 0);
        }
    }
    throw RuntimeError("Cannot convert to scalar");
}

Complex Evaluator::toComplex(const Value& value) {
    if (std::holds_alternative<Complex>(value)) {
        return std::get<Complex>(value);
    }
    if (std::holds_alternative<double>(value)) {
        return Complex(std::get<double>(value), 0.0);
    }
    throw RuntimeError("Cannot convert to complex");
}

bool Evaluator::toBool(const Value& value) {
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }
    if (std::holds_alternative<double>(value)) {
        return std::get<double>(value) != 0;
    }
    if (std::holds_alternative<Matrix>(value)) {
        return !std::get<Matrix>(value).isEmpty();
    }
    return true;
}

std::string Evaluator::toString(const Value& value) {
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<std::shared_ptr<CharArray>>(value)) {
        auto charArray = std::get<std::shared_ptr<CharArray>>(value);
        return charArray->toString();
    }
    if (std::holds_alternative<std::shared_ptr<StringArray>>(value)) {
        auto arr = std::get<std::shared_ptr<StringArray>>(value);
        if (arr->size() == 0) {
            return "";
        }
        if (arr->size() == 1) {
            return arr->get(1);
        }
        return arr->join("");
    }
    if (std::holds_alternative<double>(value)) {
        double val = std::get<double>(value);
        std::string str = std::to_string(val);
        // Remove trailing zeros
        str.erase(str.find_last_not_of('0') + 1, std::string::npos);
        if (str.back() == '.') str.pop_back();
        return str;
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<Matrix>(value)) {
        return std::get<Matrix>(value).toString();
    }
    if (std::holds_alternative<Complex>(value)) {
        const Complex& c = std::get<Complex>(value);
        std::string result = std::to_string(c.real());
        if (c.imag() >= 0) {
            result += "+";
        }
        result += std::to_string(c.imag()) + "i";
        return result;
    }
    if (std::holds_alternative<std::monostate>(value)) {
        return "";
    }
    throw RuntimeError("Cannot convert to string");
}

std::string Evaluator::valueToString(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) {
        return "";
    }
    if (std::holds_alternative<double>(value)) {
        return std::to_string(std::get<double>(value));
    }
    if (std::holds_alternative<Complex>(value)) {
        const Complex& c = std::get<Complex>(value);
        std::string result = std::to_string(c.real());
        if (c.imag() >= 0) {
            result += "+";
        }
        result += std::to_string(c.imag()) + "i";
        return result;
    }
    if (std::holds_alternative<Matrix>(value)) {
        return std::get<Matrix>(value).toString();
    }
    if (std::holds_alternative<std::string>(value)) {
        return std::get<std::string>(value);
    }
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value) ? "true" : "false";
    }
    if (std::holds_alternative<std::shared_ptr<StructArray>>(value)) {
        return std::get<std::shared_ptr<StructArray>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<Cell>>(value)) {
        return std::get<std::shared_ptr<Cell>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<Table>>(value)) {
        return std::get<std::shared_ptr<Table>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<Categorical>>(value)) {
        return std::get<std::shared_ptr<Categorical>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<DateTime>>(value)) {
        return std::get<std::shared_ptr<DateTime>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<Duration>>(value)) {
        return std::get<std::shared_ptr<Duration>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<NDArray>>(value)) {
        return std::get<std::shared_ptr<NDArray>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<Regex>>(value)) {
        return std::get<std::shared_ptr<Regex>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<StringArray>>(value)) {
        return std::get<std::shared_ptr<StringArray>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<SparseMatrix>>(value)) {
        return std::get<std::shared_ptr<SparseMatrix>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<LogicalArray>>(value)) {
        return std::get<std::shared_ptr<LogicalArray>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<CharArray>>(value)) {
        return std::get<std::shared_ptr<CharArray>>(value)->toDisplayString();
    }
    if (std::holds_alternative<std::shared_ptr<FuncHandle>>(value)) {
        return std::get<std::shared_ptr<FuncHandle>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<Map>>(value)) {
        return std::get<std::shared_ptr<Map>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<FileHandle>>(value)) {
        return std::get<std::shared_ptr<FileHandle>>(value)->toString();
    }
    if (std::holds_alternative<std::shared_ptr<MultiValue>>(value)) {
        return std::get<std::shared_ptr<MultiValue>>(value)->toString();
    }
    return "unknown";
}

bool Evaluator::valuesEqual(const Value& a, const Value& b) {
    if (a.index() != b.index()) return false;
    
    if (std::holds_alternative<std::monostate>(a)) return true;
    if (std::holds_alternative<double>(a)) {
        return std::get<double>(a) == std::get<double>(b);
    }
    if (std::holds_alternative<Complex>(a)) {
        return std::get<Complex>(a) == std::get<Complex>(b);
    }
    if (std::holds_alternative<Matrix>(a)) {
        const Matrix& ma = std::get<Matrix>(a);
        const Matrix& mb = std::get<Matrix>(b);
        if (ma.rows() != mb.rows() || ma.cols() != mb.cols()) return false;
        for (size_t i = 0; i < ma.size(); ++i) {
            if (ma(i) != mb(i)) return false;
        }
        return true;
    }
    if (std::holds_alternative<std::string>(a)) {
        return std::get<std::string>(a) == std::get<std::string>(b);
    }
    if (std::holds_alternative<bool>(a)) {
        return std::get<bool>(a) == std::get<bool>(b);
    }
    return false;
}

Value Evaluator::callFunction(const std::string& name, const std::vector<Value>& args) {
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        return callUserFunction(it->second, args);
    }
    return callBuiltinFunction(name, args);
}

Value Evaluator::callUserFunction(const FunctionDef& funcDef, const std::vector<Value>& args) {
    // Handle anonymous function
    if (funcDef.isAnonymous) {
        if (args.size() != funcDef.parameters.size()) {
            throw RuntimeError("Anonymous function expects " +
                              std::to_string(funcDef.parameters.size()) + " arguments, got " +
                              std::to_string(args.size()));
        }

        auto savedVariables = std::move(variables_);
        auto savedGlobals = std::move(currentScopeGlobals_);
        // Start with captured variables from when the function was defined
        variables_ = funcDef.capturedVariables;
        currentScopeGlobals_.clear();

        // Bind input arguments (may override captured variables)
        for (size_t i = 0; i < funcDef.parameters.size(); ++i) {
            variables_[funcDef.parameters[i]] = args[i];
        }

        // Evaluate the anonymous function body expression
        Value returnValue = evaluate(funcDef.anonymousBody);

        variables_ = std::move(savedVariables);
        currentScopeGlobals_ = std::move(savedGlobals);
        return returnValue;
    }
    
    // Handle output variable parameter (e.g., "__output__y" for "function y = square(x)")
    std::string outputVarName;
    size_t actualParamCount = funcDef.parameters.size();
    
    for (const auto& param : funcDef.parameters) {
        if (param.find("__output__") == 0) {
            outputVarName = param.substr(10); // Remove "__output__" prefix
            actualParamCount--;
            break;
        }
    }
    
    // Handle varargin
    if (funcDef.hasVarargin) {
        actualParamCount--; // Don't count varargin in required parameters
    }
    
    // Check argument count (allow more args if hasVarargin)
    if (args.size() < actualParamCount) {
        throw RuntimeError("Function " + funcDef.name + " expects at least " +
                          std::to_string(actualParamCount) + " arguments, got " +
                          std::to_string(args.size()));
    }

    auto savedVariables = std::move(variables_);
    auto savedGlobals = std::move(currentScopeGlobals_);
    variables_.clear();
    currentScopeGlobals_.clear();

    // Set nargin and nargout
    variables_["nargin"] = static_cast<double>(args.size());
    variables_["nargout"] = 1.0;  // Default to 1 output argument

    // Bind input arguments
    size_t argIdx = 0;
    for (const auto& param : funcDef.parameters) {
        if (param.find("__output__") == 0) continue; // Skip output variable marker
        if (param == "varargin") {
            // Pack remaining arguments into varargin cell array (as a row matrix)
            size_t remainingArgs = args.size() - argIdx;
            Matrix vararginMat(1, remainingArgs);
            for (size_t i = 0; i < remainingArgs; ++i) {
                vararginMat(0, i) = toDouble(args[argIdx++]);
            }
            variables_["varargin"] = vararginMat;
        } else {
            variables_[param] = args[argIdx++];
        }
    }

    bool savedReturned = returned_;
    returned_ = false;
    Value savedResult = result_;

    for (const auto& stmt : funcDef.body) {
        execute(stmt);
        if (returned_) break;
    }

    // Get return value: either from return statement or from output variable
    Value returnValue;
    if (!outputVarName.empty() && variables_.find(outputVarName) != variables_.end()) {
        returnValue = variables_[outputVarName];
    } else {
        returnValue = result_;
    }

    variables_ = std::move(savedVariables);
    currentScopeGlobals_ = std::move(savedGlobals);
    returned_ = savedReturned;
    result_ = savedResult;

    return returnValue;
}

Value Evaluator::callBuiltinFunction(const std::string& name, const std::vector<Value>& args) {
    if (name == "zeros") {
        if (args.size() == 2) {
            size_t rows = static_cast<size_t>(toDouble(args[0]));
            size_t cols = static_cast<size_t>(toDouble(args[1]));
            return Matrix::zeros(rows, cols);
        }
        throw RuntimeError("zeros requires 2 arguments");
    }

    if (name == "ones") {
        if (args.size() == 2) {
            size_t rows = static_cast<size_t>(toDouble(args[0]));
            size_t cols = static_cast<size_t>(toDouble(args[1]));
            return Matrix::ones(rows, cols);
        }
        throw RuntimeError("ones requires 2 arguments");
    }

    if (name == "eye") {
        if (args.size() == 1) {
            size_t n = static_cast<size_t>(toDouble(args[0]));
            return Matrix::eye(n);
        }
        throw RuntimeError("eye requires 1 argument");
    }

    if (name == "rand") {
        if (args.size() == 2) {
            size_t rows = static_cast<size_t>(toDouble(args[0]));
            size_t cols = static_cast<size_t>(toDouble(args[1]));
            return Matrix::rand(rows, cols);
        }
        throw RuntimeError("rand requires 2 arguments");
    }

    if (name == "inv") {
        if (args.size() == 1) {
            return toMatrix(args[0]).inverse();
        }
        throw RuntimeError("inv requires 1 argument");
    }

    if (name == "det") {
        if (args.size() == 1) {
            return toMatrix(args[0]).det();
        }
        throw RuntimeError("det requires 1 argument");
    }

    if (name == "transpose" || name == "'") {
        if (args.size() == 1) {
            return toMatrix(args[0]).transpose();
        }
        throw RuntimeError("transpose requires 1 argument");
    }

    if (name == "sin") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::sin(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).sin();
        }
        throw RuntimeError("sin requires 1 argument");
    }

    if (name == "cos") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::cos(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).cos();
        }
        throw RuntimeError("cos requires 1 argument");
    }

    if (name == "sqrt") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::sqrt(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).sqrt();
        }
        throw RuntimeError("sqrt requires 1 argument");
    }

    if (name == "tan") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::tan(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).tan();
        }
        throw RuntimeError("tan requires 1 argument");
    }

    if (name == "asin") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::asin(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).asin();
        }
        throw RuntimeError("asin requires 1 argument");
    }

    if (name == "acos") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::acos(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).acos();
        }
        throw RuntimeError("acos requires 1 argument");
    }

    if (name == "atan") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::atan(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).atan();
        }
        throw RuntimeError("atan requires 1 argument");
    }

    if (name == "sinh") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::sinh(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).sinh();
        }
        throw RuntimeError("sinh requires 1 argument");
    }

    if (name == "cosh") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::cosh(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).cosh();
        }
        throw RuntimeError("cosh requires 1 argument");
    }

    if (name == "tanh") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::tanh(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).tanh();
        }
        throw RuntimeError("tanh requires 1 argument");
    }

    if (name == "asinh") {
        if (args.size() == 1) {
            return toMatrix(args[0]).asinh();
        }
        throw RuntimeError("asinh requires 1 argument");
    }

    if (name == "acosh") {
        if (args.size() == 1) {
            return toMatrix(args[0]).acosh();
        }
        throw RuntimeError("acosh requires 1 argument");
    }

    if (name == "atanh") {
        if (args.size() == 1) {
            return toMatrix(args[0]).atanh();
        }
        throw RuntimeError("atanh requires 1 argument");
    }

    if (name == "sech") {
        if (args.size() == 1) {
            return toMatrix(args[0]).sech();
        }
        throw RuntimeError("sech requires 1 argument");
    }

    if (name == "csch") {
        if (args.size() == 1) {
            return toMatrix(args[0]).csch();
        }
        throw RuntimeError("csch requires 1 argument");
    }

    if (name == "coth") {
        if (args.size() == 1) {
            return toMatrix(args[0]).coth();
        }
        throw RuntimeError("coth requires 1 argument");
    }

    if (name == "sec") {
        if (args.size() == 1) {
            return toMatrix(args[0]).sec();
        }
        throw RuntimeError("sec requires 1 argument");
    }

    if (name == "csc") {
        if (args.size() == 1) {
            return toMatrix(args[0]).csc();
        }
        throw RuntimeError("csc requires 1 argument");
    }

    if (name == "cot") {
        if (args.size() == 1) {
            return toMatrix(args[0]).cot();
        }
        throw RuntimeError("cot requires 1 argument");
    }

    if (name == "asec") {
        if (args.size() == 1) {
            return toMatrix(args[0]).asec();
        }
        throw RuntimeError("asec requires 1 argument");
    }

    if (name == "acsc") {
        if (args.size() == 1) {
            return toMatrix(args[0]).acsc();
        }
        throw RuntimeError("acsc requires 1 argument");
    }

    if (name == "acot") {
        if (args.size() == 1) {
            return toMatrix(args[0]).acot();
        }
        throw RuntimeError("acot requires 1 argument");
    }

    if (name == "log10") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::log10(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).log10();
        }
        throw RuntimeError("log10 requires 1 argument");
    }

    if (name == "abs") {
        if (args.size() == 1) {
            return toMatrix(args[0]).abs();
        }
        throw RuntimeError("abs requires 1 argument");
    }

    if (name == "prod") {
        if (args.size() == 1) {
            return toMatrix(args[0]).prod();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).prod(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("prod requires 1 or 2 arguments");
    }

    if (name == "randn") {
        if (args.size() == 2) {
            size_t rows = static_cast<size_t>(toDouble(args[0]));
            size_t cols = static_cast<size_t>(toDouble(args[1]));
            return Matrix::randn(rows, cols);
        }
        throw RuntimeError("randn requires 2 arguments");
    }

    if (name == "randi") {
        if (args.size() == 3) {
            int imax = static_cast<int>(toDouble(args[0]));
            size_t rows = static_cast<size_t>(toDouble(args[1]));
            size_t cols = static_cast<size_t>(toDouble(args[2]));
            return Matrix::randi(imax, rows, cols);
        }
        if (args.size() == 4) {
            int imin = static_cast<int>(toDouble(args[0]));
            int imax = static_cast<int>(toDouble(args[1]));
            size_t rows = static_cast<size_t>(toDouble(args[2]));
            size_t cols = static_cast<size_t>(toDouble(args[3]));
            return Matrix::randi(imin, imax, rows, cols);
        }
        throw RuntimeError("randi requires 3 or 4 arguments");
    }

    if (name == "randperm") {
        if (args.size() == 1) {
            size_t n = static_cast<size_t>(toDouble(args[0]));
            return Matrix::randperm(n);
        }
        if (args.size() == 2) {
            size_t n = static_cast<size_t>(toDouble(args[0]));
            size_t k = static_cast<size_t>(toDouble(args[1]));
            return Matrix::randperm(n, k);
        }
        throw RuntimeError("randperm requires 1 or 2 arguments");
    }

    if (name == "exp") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::exp(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).exp();
        }
        throw RuntimeError("exp requires 1 argument");
    }

    if (name == "log") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::log(std::get<Complex>(args[0]));
            }
            return toMatrix(args[0]).log();
        }
        throw RuntimeError("log requires 1 argument");
    }

    if (name == "log2") {
        if (args.size() == 1) {
            return toMatrix(args[0]).log2();
        }
        throw RuntimeError("log2 requires 1 argument");
    }

    if (name == "sum") {
        if (args.size() == 1) {
            return toMatrix(args[0]).sum();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).sum(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("sum requires 1 or 2 arguments");
    }

    if (name == "mean") {
        if (args.size() == 1) {
            return toMatrix(args[0]).mean();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).mean(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("mean requires 1 or 2 arguments");
    }

    if (name == "max") {
        if (args.size() == 1) {
            Matrix m = toMatrix(args[0]);
            auto multiVal = std::make_shared<MultiValue>();
            
            if (m.isVector()) {
                // For vector, find max and its index
                double maxVal = m(0);
                size_t maxIdx = 1;
                for (size_t i = 1; i < m.size(); ++i) {
                    if (m(i) > maxVal) {
                        maxVal = m(i);
                        maxIdx = i + 1; // 1-based index
                    }
                }
                multiVal->push_back(maxVal);
                multiVal->push_back(static_cast<double>(maxIdx));
            } else {
                // For matrix, return overall max and its linear index
                double maxVal = m(0);
                size_t maxIdx = 1;
                for (size_t i = 1; i < m.size(); ++i) {
                    if (m(i) > maxVal) {
                        maxVal = m(i);
                        maxIdx = i + 1; // 1-based index
                    }
                }
                multiVal->push_back(maxVal);
                multiVal->push_back(static_cast<double>(maxIdx));
            }
            return multiVal;
        }
        if (args.size() == 2) {
            int dim = static_cast<int>(toDouble(args[1]));
            Matrix m = toMatrix(args[0]);
            auto multiVal = std::make_shared<MultiValue>();
            
            if (dim == 1) {
                // Max along each column
                Matrix maxVals(1, m.cols());
                Matrix maxIdxs(1, m.cols());
                for (size_t j = 0; j < m.cols(); ++j) {
                    double maxVal = m(0, j);
                    size_t maxIdx = 1;
                    for (size_t i = 1; i < m.rows(); ++i) {
                        if (m(i, j) > maxVal) {
                            maxVal = m(i, j);
                            maxIdx = i + 1;
                        }
                    }
                    maxVals(0, j) = maxVal;
                    maxIdxs(0, j) = static_cast<double>(maxIdx);
                }
                multiVal->push_back(maxVals);
                multiVal->push_back(maxIdxs);
            } else if (dim == 2) {
                // Max along each row
                Matrix maxVals(m.rows(), 1);
                Matrix maxIdxs(m.rows(), 1);
                for (size_t i = 0; i < m.rows(); ++i) {
                    double maxVal = m(i, 0);
                    size_t maxIdx = 1;
                    for (size_t j = 1; j < m.cols(); ++j) {
                        if (m(i, j) > maxVal) {
                            maxVal = m(i, j);
                            maxIdx = j + 1;
                        }
                    }
                    maxVals(i, 0) = maxVal;
                    maxIdxs(i, 0) = static_cast<double>(maxIdx);
                }
                multiVal->push_back(maxVals);
                multiVal->push_back(maxIdxs);
            }
            return multiVal;
        }
        throw RuntimeError("max requires 1 or 2 arguments");
    }

    if (name == "min") {
        if (args.size() == 1) {
            Matrix m = toMatrix(args[0]);
            auto multiVal = std::make_shared<MultiValue>();
            
            if (m.isVector()) {
                // For vector, find min and its index
                double minVal = m(0);
                size_t minIdx = 1;
                for (size_t i = 1; i < m.size(); ++i) {
                    if (m(i) < minVal) {
                        minVal = m(i);
                        minIdx = i + 1; // 1-based index
                    }
                }
                multiVal->push_back(minVal);
                multiVal->push_back(static_cast<double>(minIdx));
            } else {
                // For matrix, return overall min and its linear index
                double minVal = m(0);
                size_t minIdx = 1;
                for (size_t i = 1; i < m.size(); ++i) {
                    if (m(i) < minVal) {
                        minVal = m(i);
                        minIdx = i + 1; // 1-based index
                    }
                }
                multiVal->push_back(minVal);
                multiVal->push_back(static_cast<double>(minIdx));
            }
            return multiVal;
        }
        if (args.size() == 2) {
            int dim = static_cast<int>(toDouble(args[1]));
            Matrix m = toMatrix(args[0]);
            auto multiVal = std::make_shared<MultiValue>();
            
            if (dim == 1) {
                // Min along each column
                Matrix minVals(1, m.cols());
                Matrix minIdxs(1, m.cols());
                for (size_t j = 0; j < m.cols(); ++j) {
                    double minVal = m(0, j);
                    size_t minIdx = 1;
                    for (size_t i = 1; i < m.rows(); ++i) {
                        if (m(i, j) < minVal) {
                            minVal = m(i, j);
                            minIdx = i + 1;
                        }
                    }
                    minVals(0, j) = minVal;
                    minIdxs(0, j) = static_cast<double>(minIdx);
                }
                multiVal->push_back(minVals);
                multiVal->push_back(minIdxs);
            } else if (dim == 2) {
                // Min along each row
                Matrix minVals(m.rows(), 1);
                Matrix minIdxs(m.rows(), 1);
                for (size_t i = 0; i < m.rows(); ++i) {
                    double minVal = m(i, 0);
                    size_t minIdx = 1;
                    for (size_t j = 1; j < m.cols(); ++j) {
                        if (m(i, j) < minVal) {
                            minVal = m(i, j);
                            minIdx = j + 1;
                        }
                    }
                    minVals(i, 0) = minVal;
                    minIdxs(i, 0) = static_cast<double>(minIdx);
                }
                multiVal->push_back(minVals);
                multiVal->push_back(minIdxs);
            }
            return multiVal;
        }
        throw RuntimeError("min requires 1 or 2 arguments");
    }

    if (name == "std") {
        if (args.size() == 1) {
            return toMatrix(args[0]).std();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).std(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("std requires 1 or 2 arguments");
    }

    if (name == "var") {
        if (args.size() == 1) {
            return toMatrix(args[0]).var();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).var(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("var requires 1 or 2 arguments");
    }

    if (name == "cumsum") {
        if (args.size() == 1) {
            return toMatrix(args[0]).cumsum();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).cumsum(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("cumsum requires 1 or 2 arguments");
    }

    if (name == "cumprod") {
        if (args.size() == 1) {
            return toMatrix(args[0]).cumprod();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).cumprod(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("cumprod requires 1 or 2 arguments");
    }

    if (name == "diff") {
        if (args.size() == 1) {
            return toMatrix(args[0]).diff();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).diff(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("diff requires 1 or 2 arguments");
    }

    if (name == "floor") {
        if (args.size() == 1) {
            return toMatrix(args[0]).floor();
        }
        throw RuntimeError("floor requires 1 argument");
    }

    if (name == "ceil") {
        if (args.size() == 1) {
            return toMatrix(args[0]).ceil();
        }
        throw RuntimeError("ceil requires 1 argument");
    }

    if (name == "round") {
        if (args.size() == 1) {
            return toMatrix(args[0]).round();
        }
        throw RuntimeError("round requires 1 argument");
    }

    if (name == "fix") {
        if (args.size() == 1) {
            return toMatrix(args[0]).fix();
        }
        throw RuntimeError("fix requires 1 argument");
    }

    if (name == "sign") {
        if (args.size() == 1) {
            return toMatrix(args[0]).sign();
        }
        throw RuntimeError("sign requires 1 argument");
    }

    if (name == "expm1") {
        if (args.size() == 1) {
            return toMatrix(args[0]).expm1();
        }
        throw RuntimeError("expm1 requires 1 argument");
    }

    if (name == "log1p") {
        if (args.size() == 1) {
            return toMatrix(args[0]).log1p();
        }
        throw RuntimeError("log1p requires 1 argument");
    }

    if (name == "hypot") {
        if (args.size() == 2) {
            return Matrix::hypot(toMatrix(args[0]), toMatrix(args[1]));
        }
        throw RuntimeError("hypot requires 2 arguments");
    }

    if (name == "nextpow2") {
        if (args.size() == 1) {
            int n = static_cast<int>(toDouble(args[0]));
            return static_cast<double>(Matrix::nextpow2(n));
        }
        throw RuntimeError("nextpow2 requires 1 argument");
    }

    if (name == "factorial") {
        if (args.size() == 1) {
            int n = static_cast<int>(toDouble(args[0]));
            if (n < 0) throw RuntimeError("factorial requires non-negative integer");
            if (n > 170) throw RuntimeError("factorial result would overflow");
            double result = 1.0;
            for (int i = 2; i <= n; ++i) {
                result *= i;
            }
            return result;
        }
        throw RuntimeError("factorial requires 1 argument");
    }

    if (name == "nchoosek") {
        if (args.size() == 2) {
            int n = static_cast<int>(toDouble(args[0]));
            int k = static_cast<int>(toDouble(args[1]));
            if (n < 0 || k < 0) throw RuntimeError("nchoosek requires non-negative integers");
            if (k > n) return 0.0;
            if (k > n - k) k = n - k;
            double result = 1.0;
            for (int i = 0; i < k; ++i) {
                result = result * (n - i) / (i + 1);
            }
            return result;
        }
        throw RuntimeError("nchoosek requires 2 arguments");
    }

    if (name == "gcd") {
        if (args.size() == 2) {
            int a = static_cast<int>(std::abs(toDouble(args[0])));
            int b = static_cast<int>(std::abs(toDouble(args[1])));
            while (b != 0) {
                int temp = b;
                b = a % b;
                a = temp;
            }
            return static_cast<double>(a);
        }
        throw RuntimeError("gcd requires 2 arguments");
    }

    if (name == "lcm") {
        if (args.size() == 2) {
            double a = toDouble(args[0]);
            double b = toDouble(args[1]);
            if (a == 0 || b == 0) return 0.0;
            double gcd_val = std::get<double>(callBuiltinFunction("gcd", args));
            return std::abs(a * b) / gcd_val;
        }
        throw RuntimeError("lcm requires 2 arguments");
    }

    if (name == "isprime") {
        if (args.size() == 1) {
            int n = static_cast<int>(toDouble(args[0]));
            if (n < 2) return 0.0;
            if (n == 2) return 1.0;
            if (n % 2 == 0) return 0.0;
            for (int i = 3; i * i <= n; i += 2) {
                if (n % i == 0) return 0.0;
            }
            return 1.0;
        }
        throw RuntimeError("isprime requires 1 argument");
    }

    if (name == "primes") {
        if (args.size() == 1) {
            int n = static_cast<int>(toDouble(args[0]));
            if (n < 2) return Matrix(0, 0);
            std::vector<double> primeList;
            std::vector<bool> isPrime(n + 1, true);
            isPrime[0] = isPrime[1] = false;
            for (int i = 2; i * i <= n; ++i) {
                if (isPrime[i]) {
                    for (int j = i * i; j <= n; j += i) {
                        isPrime[j] = false;
                    }
                }
            }
            for (int i = 2; i <= n; ++i) {
                if (isPrime[i]) primeList.push_back(static_cast<double>(i));
            }
            Matrix result(1, primeList.size());
            for (size_t i = 0; i < primeList.size(); ++i) {
                result(0, i) = primeList[i];
            }
            return result;
        }
        throw RuntimeError("primes requires 1 argument");
    }

    if (name == "gamma") {
        if (args.size() == 1) {
            double x = toDouble(args[0]);
            return std::tgamma(x);
        }
        throw RuntimeError("gamma requires 1 argument");
    }

    if (name == "gammaln") {
        if (args.size() == 1) {
            double x = toDouble(args[0]);
            return std::lgamma(x);
        }
        throw RuntimeError("gammaln requires 1 argument");
    }

    if (name == "erf") {
        if (args.size() == 1) {
            double x = toDouble(args[0]);
            return std::erf(x);
        }
        throw RuntimeError("erf requires 1 argument");
    }

    if (name == "erfc") {
        if (args.size() == 1) {
            double x = toDouble(args[0]);
            return std::erfc(x);
        }
        throw RuntimeError("erfc requires 1 argument");
    }

    if (name == "atan2") {
        if (args.size() == 2) {
            return Matrix::atan2(toMatrix(args[0]), toMatrix(args[1]));
        }
        throw RuntimeError("atan2 requires 2 arguments");
    }

    if (name == "diag") {
        if (args.size() == 1) {
            return toMatrix(args[0]).diag();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).diag(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("diag requires 1 or 2 arguments");
    }

    if (name == "trace") {
        if (args.size() == 1) {
            return toMatrix(args[0]).trace();
        }
        throw RuntimeError("trace requires 1 argument");
    }

    if (name == "tril") {
        if (args.size() == 1) {
            return toMatrix(args[0]).tril();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).tril(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("tril requires 1 or 2 arguments");
    }

    if (name == "triu") {
        if (args.size() == 1) {
            return toMatrix(args[0]).triu();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).triu(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("triu requires 1 or 2 arguments");
    }

    if (name == "horzcat") {
        if (args.size() == 2) {
            return Matrix::horzcat(toMatrix(args[0]), toMatrix(args[1]));
        }
        throw RuntimeError("horzcat requires 2 arguments");
    }

    if (name == "vertcat") {
        if (args.size() == 2) {
            return Matrix::vertcat(toMatrix(args[0]), toMatrix(args[1]));
        }
        throw RuntimeError("vertcat requires 2 arguments");
    }

    if (name == "mod") {
        if (args.size() == 2) {
            double x = toDouble(args[0]);
            double y = toDouble(args[1]);
            if (y == 0) throw RuntimeError("Modulo by zero");
            double result = std::fmod(x, y);
            if (y < 0 && result > 0) result += y;
            if (y > 0 && result < 0) result += y;
            return result;
        }
        throw RuntimeError("mod requires 2 arguments");
    }

    if (name == "rem") {
        if (args.size() == 2) {
            double x = toDouble(args[0]);
            double y = toDouble(args[1]);
            if (y == 0) throw RuntimeError("Division by zero");
            return std::fmod(x, y);
        }
        throw RuntimeError("rem requires 2 arguments");
    }

    if (name == "disp" || name == "print") {
        for (const auto& arg : args) {
            std::cout << valueToString(arg) << std::endl;
        }
        return std::monostate{};
    }

    if (name == "size") {
        if (args.size() == 1) {
            Matrix m = toMatrix(args[0]);
            auto multiVal = std::make_shared<MultiValue>();
            multiVal->push_back(static_cast<double>(m.rows()));
            multiVal->push_back(static_cast<double>(m.cols()));
            return multiVal;
        }
        throw RuntimeError("size requires 1 argument");
    }

    if (name == "length") {
        if (args.size() == 1) {
            return static_cast<double>(toMatrix(args[0]).length());
        }
        throw RuntimeError("length requires 1 argument");
    }

    if (name == "numel") {
        if (args.size() == 1) {
            return static_cast<double>(toMatrix(args[0]).numel());
        }
        throw RuntimeError("numel requires 1 argument");
    }

    if (name == "isempty") {
        if (args.size() == 1) {
            return toMatrix(args[0]).isempty() ? 1.0 : 0.0;
        }
        throw RuntimeError("isempty requires 1 argument");
    }

    if (name == "printf") {
        for (const auto& arg : args) {
            std::cout << valueToString(arg);
        }
        return 0.0;
    }

    if (name == "enable_gpu") {
        cnlab::gpu::enableGPU();
        return 1.0;
    }

    if (name == "disable_gpu") {
        cnlab::gpu::disableGPU();
        return 0.0;
    }

    if (name == "set_gpu_threshold") {
        if (args.size() == 1) {
            size_t threshold = static_cast<size_t>(toDouble(args[0]));
            cnlab::gpu::setGPUThreshold(threshold);
            return static_cast<double>(threshold);
        }
        throw RuntimeError("set_gpu_threshold requires 1 argument");
    }

    if (name == "gpu_info") {
        std::string info = cnlab::gpu::getGPUInfo();
        std::cout << info << std::endl;
        return 0.0;
    }

    if (name == "lu") {
        if (args.size() == 1) {
            Matrix m = toMatrix(args[0]);
            LUResult result = lu_decomposition(m);
            
            auto multiVal = std::make_shared<MultiValue>();
            multiVal->push_back(result.L);  // Lower triangular
            multiVal->push_back(result.U);  // Upper triangular
            
            // Create permutation matrix P from perm vector
            size_t n = m.rows();
            Matrix P = Matrix::eye(n);
            for (size_t i = 0; i < n; ++i) {
                if (result.perm[i] != i) {
                    // Swap rows i and perm[i]
                    for (size_t j = 0; j < n; ++j) {
                        double temp = P(i, j);
                        P(i, j) = P(result.perm[i], j);
                        P(result.perm[i], j) = temp;
                    }
                }
            }
            multiVal->push_back(P);  // Permutation matrix
            
            return multiVal;
        }
        throw RuntimeError("lu requires 1 argument");
    }

    if (name == "qr") {
        if (args.size() == 1) {
            return toMatrix(args[0]).qr();
        }
        throw RuntimeError("qr requires 1 argument");
    }

    if (name == "svd") {
        if (args.size() == 1) {
            Matrix m = toMatrix(args[0]);
            SVDResult result = svd_decomposition(m);
            
            auto multiVal = std::make_shared<MultiValue>();
            multiVal->push_back(result.U);  // Left singular vectors
            multiVal->push_back(result.S);  // Singular values (diagonal matrix)
            multiVal->push_back(result.V);  // Right singular vectors
            
            return multiVal;
        }
        throw RuntimeError("svd requires 1 argument");
    }

    if (name == "eig") {
        if (args.size() == 1) {
            Matrix m = toMatrix(args[0]);
            EigResult result = eig_decomposition(m);
            
            auto multiVal = std::make_shared<MultiValue>();
            multiVal->push_back(result.V);  // Eigenvectors
            multiVal->push_back(result.D);  // Eigenvalues (diagonal matrix)
            
            return multiVal;
        }
        throw RuntimeError("eig requires 1 argument");
    }

    if (name == "chol") {
        if (args.size() == 1) {
            return toMatrix(args[0]).chol();
        }
        throw RuntimeError("chol requires 1 argument");
    }

    if (name == "solve") {
        if (args.size() == 2) {
            return Matrix::solve(toMatrix(args[0]), toMatrix(args[1]));
        }
        throw RuntimeError("solve requires 2 arguments");
    }

    if (name == "cond") {
        if (args.size() == 1) {
            return toMatrix(args[0]).cond();
        }
        throw RuntimeError("cond requires 1 argument");
    }

    if (name == "rank") {
        if (args.size() == 1) {
            return static_cast<double>(toMatrix(args[0]).rank());
        }
        throw RuntimeError("rank requires 1 argument");
    }

    if (name == "norm") {
        if (args.size() == 1) {
            return toMatrix(args[0]).norm();
        }
        throw RuntimeError("norm requires 1 argument");
    }

    if (name == "sort") {
        Matrix m = toMatrix(args[0]);
        int dim = 1;
        if (args.size() >= 2) {
            dim = static_cast<int>(toDouble(args[1]));
        }
        
        auto multiVal = std::make_shared<MultiValue>();
        
        if (m.isVector()) {
            // For vector, sort and return indices
            std::vector<std::pair<double, size_t>> vec;
            for (size_t i = 0; i < m.size(); ++i) {
                vec.push_back({m(i), i + 1}); // 1-based index
            }
            std::sort(vec.begin(), vec.end());
            
            Matrix sortedVals(1, m.size());
            Matrix sortedIdxs(1, m.size());
            for (size_t i = 0; i < vec.size(); ++i) {
                sortedVals(0, i) = vec[i].first;
                sortedIdxs(0, i) = static_cast<double>(vec[i].second);
            }
            multiVal->push_back(sortedVals);
            multiVal->push_back(sortedIdxs);
        } else if (dim == 1) {
            // Sort along each column
            Matrix sortedVals(m.rows(), m.cols());
            Matrix sortedIdxs(m.rows(), m.cols());
            for (size_t j = 0; j < m.cols(); ++j) {
                std::vector<std::pair<double, size_t>> col;
                for (size_t i = 0; i < m.rows(); ++i) {
                    col.push_back({m(i, j), i + 1});
                }
                std::sort(col.begin(), col.end());
                for (size_t i = 0; i < col.size(); ++i) {
                    sortedVals(i, j) = col[i].first;
                    sortedIdxs(i, j) = static_cast<double>(col[i].second);
                }
            }
            multiVal->push_back(sortedVals);
            multiVal->push_back(sortedIdxs);
        } else if (dim == 2) {
            // Sort along each row
            Matrix sortedVals(m.rows(), m.cols());
            Matrix sortedIdxs(m.rows(), m.cols());
            for (size_t i = 0; i < m.rows(); ++i) {
                std::vector<std::pair<double, size_t>> row;
                for (size_t j = 0; j < m.cols(); ++j) {
                    row.push_back({m(i, j), j + 1});
                }
                std::sort(row.begin(), row.end());
                for (size_t j = 0; j < row.size(); ++j) {
                    sortedVals(i, j) = row[j].first;
                    sortedIdxs(i, j) = static_cast<double>(row[j].second);
                }
            }
            multiVal->push_back(sortedVals);
            multiVal->push_back(sortedIdxs);
        }
        return multiVal;
    }

    if (name == "find") {
        Matrix m = toMatrix(args[0]);
        int k = static_cast<int>(m.size());
        if (args.size() >= 2) {
            k = static_cast<int>(toDouble(args[1]));
        }
        
        auto multiVal = std::make_shared<MultiValue>();
        
        // Find non-zero elements
        std::vector<std::pair<size_t, size_t>> indices; // (row, col) pairs
        for (size_t i = 0; i < m.size() && static_cast<int>(indices.size()) < k; ++i) {
            if (m(i) != 0.0) {
                size_t row = i % m.rows();
                size_t col = i / m.rows();
                indices.push_back({row + 1, col + 1}); // 1-based indexing
            }
        }
        
        // Create row indices matrix
        Matrix rowIdx(indices.size(), 1);
        for (size_t i = 0; i < indices.size(); ++i) {
            rowIdx(i, 0) = static_cast<double>(indices[i].first);
        }
        
        // Create column indices matrix
        Matrix colIdx(indices.size(), 1);
        for (size_t i = 0; i < indices.size(); ++i) {
            colIdx(i, 0) = static_cast<double>(indices[i].second);
        }
        
        multiVal->push_back(rowIdx);
        multiVal->push_back(colIdx);
        
        return multiVal;
    }

    if (name == "unique") {
        if (args.size() == 1) {
            return toMatrix(args[0]).unique();
        }
        throw RuntimeError("unique requires 1 argument");
    }

    if (name == "repmat") {
        if (args.size() == 3) {
            size_t m = static_cast<size_t>(toDouble(args[1]));
            size_t n = static_cast<size_t>(toDouble(args[2]));
            return Matrix::repmat(toMatrix(args[0]), m, n);
        }
        throw RuntimeError("repmat requires 3 arguments");
    }

    if (name == "flipud") {
        if (args.size() == 1) {
            return toMatrix(args[0]).flipud();
        }
        throw RuntimeError("flipud requires 1 argument");
    }

    if (name == "fliplr") {
        if (args.size() == 1) {
            return toMatrix(args[0]).fliplr();
        }
        throw RuntimeError("fliplr requires 1 argument");
    }

    if (name == "rot90") {
        if (args.size() == 1) {
            return toMatrix(args[0]).rot90();
        }
        if (args.size() == 2) {
            return toMatrix(args[0]).rot90(static_cast<int>(toDouble(args[1])));
        }
        throw RuntimeError("rot90 requires 1 or 2 arguments");
    }

    if (name == "linspace") {
        if (args.size() == 2 || args.size() == 3) {
            double start = toDouble(args[0]);
            double end = toDouble(args[1]);
            size_t n = (args.size() == 3) ? static_cast<size_t>(toDouble(args[2])) : 100;
            if (n < 2) n = 2;
            Matrix result(n, 1);
            for (size_t i = 0; i < n; ++i) {
                result(i, 0) = start + (end - start) * static_cast<double>(i) / static_cast<double>(n - 1);
            }
            return result;
        }
        throw RuntimeError("linspace requires 2 or 3 arguments");
    }

    if (name == "logspace") {
        if (args.size() == 2 || args.size() == 3) {
            double start = toDouble(args[0]);
            double end = toDouble(args[1]);
            size_t n = (args.size() == 3) ? static_cast<size_t>(toDouble(args[2])) : 50;
            if (n < 2) n = 2;
            Matrix result(n, 1);
            for (size_t i = 0; i < n; ++i) {
                double exponent = start + (end - start) * static_cast<double>(i) / static_cast<double>(n - 1);
                result(i, 0) = std::pow(10.0, exponent);
            }
            return result;
        }
        throw RuntimeError("logspace requires 2 or 3 arguments");
    }

    if (name == "magic") {
        if (args.size() == 1) {
            size_t n = static_cast<size_t>(toDouble(args[0]));
            return Matrix::magic(n);
        }
        throw RuntimeError("magic requires 1 argument");
    }

    if (name == "hilb") {
        if (args.size() == 1) {
            size_t n = static_cast<size_t>(toDouble(args[0]));
            return Matrix::hilb(n);
        }
        throw RuntimeError("hilb requires 1 argument");
    }

    if (name == "pascal") {
        if (args.size() == 1) {
            size_t n = static_cast<size_t>(toDouble(args[0]));
            return Matrix::pascal(n);
        }
        throw RuntimeError("pascal requires 1 argument");
    }

    if (name == "vander") {
        if (args.size() == 1) {
            return Matrix::vander(toMatrix(args[0]));
        }
        throw RuntimeError("vander requires 1 argument");
    }

    if (name == "meshgrid") {
        if (args.size() == 1) {
            auto [X, Y] = Matrix::meshgrid(toMatrix(args[0]));
            return Matrix::horzcat(X, Y);
        }
        if (args.size() == 2) {
            auto [X, Y] = Matrix::meshgrid(toMatrix(args[0]), toMatrix(args[1]));
            return Matrix::horzcat(X, Y);
        }
        throw RuntimeError("meshgrid requires 1 or 2 arguments");
    }

    if (name == "blkdiag") {
        if (args.size() == 2) {
            return Matrix::blkdiag(toMatrix(args[0]), toMatrix(args[1]));
        }
        throw RuntimeError("blkdiag requires 2 arguments");
    }

    if (name == "kron") {
        if (args.size() == 2) {
            return Matrix::kron(toMatrix(args[0]), toMatrix(args[1]));
        }
        throw RuntimeError("kron requires 2 arguments");
    }

    // String functions
    if (name == "strcat") {
        if (args.size() >= 2) {
            std::string result;
            for (const auto& arg : args) {
                if (std::holds_alternative<std::string>(arg)) {
                    result += std::get<std::string>(arg);
                } else if (std::holds_alternative<double>(arg)) {
                    result += std::to_string(std::get<double>(arg));
                } else if (std::holds_alternative<std::shared_ptr<StringArray>>(arg)) {
                    auto arr = std::get<std::shared_ptr<StringArray>>(arg);
                    result += arr->join("");
                } else {
                    throw RuntimeError("strcat requires string, number, or string array arguments");
                }
            }
            return result;
        }
        throw RuntimeError("strcat requires at least 2 arguments");
    }

    if (name == "strcmp") {
        if (args.size() == 2) {
            std::string s1 = toString(args[0]);
            std::string s2 = toString(args[1]);
            return s1 == s2;
        }
        throw RuntimeError("strcmp requires 2 arguments");
    }

    if (name == "strcmpi") {
        if (args.size() == 2) {
            std::string s1 = toString(args[0]);
            std::string s2 = toString(args[1]);
            std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
            std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
            return s1 == s2;
        }
        throw RuntimeError("strcmpi requires 2 arguments");
    }

    if (name == "strfind") {
        if (args.size() == 2) {
            std::string str = toString(args[0]);
            std::string pattern = toString(args[1]);
            
            // Find all occurrences
            std::vector<double> positions;
            size_t pos = 0;
            while ((pos = str.find(pattern, pos)) != std::string::npos) {
                positions.push_back(static_cast<double>(pos + 1));  // 1-based index
                pos += 1;  // Move forward to find overlapping matches
            }
            
            if (positions.empty()) {
                return static_cast<double>(0);  // Not found
            }
            
            // Return as column vector
            Matrix result(positions.size(), 1);
            for (size_t i = 0; i < positions.size(); ++i) {
                result(i, 0) = positions[i];
            }
            return result;
        }
        throw RuntimeError("strfind requires 2 arguments");
    }

    if (name == "strrep") {
        if (args.size() == 3) {
            std::string str = toString(args[0]);
            std::string oldStr = toString(args[1]);
            std::string newStr = toString(args[2]);
            size_t pos = 0;
            while ((pos = str.find(oldStr, pos)) != std::string::npos) {
                str.replace(pos, oldStr.length(), newStr);
                pos += newStr.length();
            }
            return str;
        }
        throw RuntimeError("strrep requires 3 arguments");
    }

    if (name == "strtrim") {
        if (args.size() == 1) {
            std::string str = toString(args[0]);
            // Trim leading whitespace
            size_t start = str.find_first_not_of(" \t\n\r");
            if (start == std::string::npos) return std::string("");
            // Trim trailing whitespace
            size_t end = str.find_last_not_of(" \t\n\r");
            return str.substr(start, end - start + 1);
        }
        throw RuntimeError("strtrim requires 1 argument");
    }

    if (name == "num2str") {
        if (args.size() == 1) {
            if (std::holds_alternative<double>(args[0])) {
                double val = std::get<double>(args[0]);
                // Remove trailing zeros
                std::string str = std::to_string(val);
                str.erase(str.find_last_not_of('0') + 1, std::string::npos);
                if (str.back() == '.') str.pop_back();
                return str;
            } else if (std::holds_alternative<Matrix>(args[0])) {
                return std::get<Matrix>(args[0]).toString();
            }
            throw RuntimeError("num2str requires a number or matrix argument");
        }
        throw RuntimeError("num2str requires 1 argument");
    }

    if (name == "str2num") {
        if (args.size() == 1) {
            std::string str = toString(args[0]);
            try {
                return std::stod(str);
            } catch (...) {
                return static_cast<double>(0);
            }
        }
        throw RuntimeError("str2num requires 1 argument");
    }

    if (name == "startsWith") {
        if (args.size() == 2) {
            std::string str = toString(args[0]);
            std::string prefix = toString(args[1]);
            return str.find(prefix) == 0 ? 1.0 : 0.0;
        }
        throw RuntimeError("startsWith requires 2 arguments");
    }

    if (name == "endsWith") {
        if (args.size() == 2) {
            std::string str = toString(args[0]);
            std::string suffix = toString(args[1]);
            if (suffix.length() > str.length()) return 0.0;
            return str.rfind(suffix) == str.length() - suffix.length() ? 1.0 : 0.0;
        }
        throw RuntimeError("endsWith requires 2 arguments");
    }

    if (name == "contains") {
        if (args.size() == 2) {
            std::string str = toString(args[0]);
            std::string substr = toString(args[1]);
            return str.find(substr) != std::string::npos ? 1.0 : 0.0;
        }
        throw RuntimeError("contains requires 2 arguments");
    }

    if (name == "join") {
        if (args.size() == 2) {
            std::string delimiter = toString(args[1]);
            if (std::holds_alternative<Matrix>(args[0])) {
                Matrix mat = std::get<Matrix>(args[0]);
                std::string result;
                for (size_t i = 0; i < mat.size(); ++i) {
                    if (i > 0) result += delimiter;
                    result += std::to_string(mat(i));
                }
                return result;
            } else if (std::holds_alternative<std::shared_ptr<StringArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<StringArray>>(args[0]);
                return arr->join(delimiter);
            } else if (std::holds_alternative<std::shared_ptr<Cell>>(args[0])) {
                auto cell = std::get<std::shared_ptr<Cell>>(args[0]);
                std::string result;
                for (size_t i = 1; i <= cell->size(); ++i) {
                    if (i > 1) result += delimiter;
                    result += toString(cell->getElement(i));
                }
                return result;
            }
            throw RuntimeError("join requires matrix, string array, or cell array as first argument");
        }
        throw RuntimeError("join requires 2 arguments");
    }

    if (name == "extractBefore") {
        if (args.size() == 2) {
            if (std::holds_alternative<std::string>(args[0]) && std::holds_alternative<double>(args[1])) {
                std::string str = std::get<std::string>(args[0]);
                size_t pos = static_cast<size_t>(std::get<double>(args[1]));
                if (pos <= 1 || pos > str.length() + 1) return std::string("");
                return str.substr(0, pos - 1);
            }
            throw RuntimeError("extractBefore requires string and position arguments");
        }
        throw RuntimeError("extractBefore requires 2 arguments");
    }

    if (name == "extractAfter") {
        if (args.size() == 2) {
            if (std::holds_alternative<std::string>(args[0]) && std::holds_alternative<double>(args[1])) {
                std::string str = std::get<std::string>(args[0]);
                size_t pos = static_cast<size_t>(std::get<double>(args[1]));
                if (pos >= str.length()) return std::string("");
                return str.substr(pos);
            }
            throw RuntimeError("extractAfter requires string and position arguments");
        }
        throw RuntimeError("extractAfter requires 2 arguments");
    }

    if (name == "extractBetween") {
        if (args.size() == 3) {
            if (std::holds_alternative<std::string>(args[0]) && 
                std::holds_alternative<double>(args[1]) && 
                std::holds_alternative<double>(args[2])) {
                std::string str = std::get<std::string>(args[0]);
                size_t start = static_cast<size_t>(std::get<double>(args[1]));
                size_t end = static_cast<size_t>(std::get<double>(args[2]));
                if (start < 1 || end > str.length() || start > end) return std::string("");
                return str.substr(start - 1, end - start + 1);
            }
            throw RuntimeError("extractBetween requires string and position arguments");
        }
        throw RuntimeError("extractBetween requires 3 arguments");
    }

    if (name == "replace") {
        if (args.size() == 3) {
            if (std::holds_alternative<std::string>(args[0]) && 
                std::holds_alternative<std::string>(args[1]) && 
                std::holds_alternative<std::string>(args[2])) {
                std::string str = std::get<std::string>(args[0]);
                std::string oldStr = std::get<std::string>(args[1]);
                std::string newStr = std::get<std::string>(args[2]);
                size_t pos = str.find(oldStr);
                if (pos != std::string::npos) {
                    str.replace(pos, oldStr.length(), newStr);
                }
                return str;
            }
            throw RuntimeError("replace requires string arguments");
        }
        throw RuntimeError("replace requires 3 arguments");
    }

    if (name == "reverse") {
        if (args.size() == 1) {
            if (std::holds_alternative<std::string>(args[0])) {
                std::string str = std::get<std::string>(args[0]);
                std::reverse(str.begin(), str.end());
                return str;
            }
            throw RuntimeError("reverse requires a string argument");
        }
        throw RuntimeError("reverse requires 1 argument");
    }

    if (name == "sprintf") {
        if (args.size() >= 1) {
            if (std::holds_alternative<std::string>(args[0])) {
                std::string format = std::get<std::string>(args[0]);
                std::string result;
                size_t argIdx = 1;
                for (size_t i = 0; i < format.length(); ++i) {
                    if (format[i] == '%' && i + 1 < format.length()) {
                        char spec = format[i + 1];
                        if (spec == '%') {
                            result += '%';
                            i++;
                        } else if (argIdx < args.size()) {
                            std::ostringstream oss;
                            if (spec == 'd' || spec == 'i') {
                                oss << static_cast<int>(toDouble(args[argIdx]));
                            } else if (spec == 'f') {
                                oss << std::fixed << toDouble(args[argIdx]);
                            } else if (spec == 'e') {
                                oss << std::scientific << toDouble(args[argIdx]);
                            } else if (spec == 'g') {
                                oss << toDouble(args[argIdx]);
                            } else if (spec == 's') {
                                if (std::holds_alternative<std::string>(args[argIdx])) {
                                    oss << std::get<std::string>(args[argIdx]);
                                } else {
                                    oss << valueToString(args[argIdx]);
                                }
                            } else if (spec == 'c') {
                                if (std::holds_alternative<std::string>(args[argIdx]) && 
                                    std::get<std::string>(args[argIdx]).length() > 0) {
                                    oss << std::get<std::string>(args[argIdx])[0];
                                }
                            } else {
                                result += format[i];
                                continue;
                            }
                            result += oss.str();
                            argIdx++;
                            i++;
                        } else {
                            result += format[i];
                        }
                    } else {
                        result += format[i];
                    }
                }
                return result;
            }
            throw RuntimeError("sprintf requires a format string as first argument");
        }
        throw RuntimeError("sprintf requires at least 1 argument");
    }

    if (name == "split") {
        if (args.size() == 2) {
            if (std::holds_alternative<std::string>(args[0]) && std::holds_alternative<std::string>(args[1])) {
                std::string str = std::get<std::string>(args[0]);
                std::string delimiter = std::get<std::string>(args[1]);
                std::vector<std::string> parts;
                size_t start = 0;
                size_t end = str.find(delimiter);
                while (end != std::string::npos) {
                    parts.push_back(str.substr(start, end - start));
                    start = end + delimiter.length();
                    end = str.find(delimiter, start);
                }
                parts.push_back(str.substr(start));
                // Return as cell array (MATLAB compatible)
                auto cell = std::make_shared<Cell>(parts.size());
                for (size_t i = 0; i < parts.size(); ++i) {
                    cell->setElement(i + 1, parts[i]);
                }
                return cell;
            }
            throw RuntimeError("split requires string arguments");
        }
        throw RuntimeError("split requires 2 arguments");
    }

    // Complex number functions
    if (name == "real") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::get<Complex>(args[0]).real();
            }
            return toDouble(args[0]);
        }
        throw RuntimeError("real requires 1 argument");
    }

    if (name == "imag") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::get<Complex>(args[0]).imag();
            }
            return 0.0;
        }
        throw RuntimeError("imag requires 1 argument");
    }

    if (name == "abs") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::abs(std::get<Complex>(args[0]));
            }
            return std::abs(toDouble(args[0]));
        }
        throw RuntimeError("abs requires 1 argument");
    }

    if (name == "angle") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::arg(std::get<Complex>(args[0]));
            }
            return 0.0;
        }
        throw RuntimeError("angle requires 1 argument");
    }

    if (name == "conj") {
        if (args.size() == 1) {
            if (std::holds_alternative<Complex>(args[0])) {
                return std::conj(std::get<Complex>(args[0]));
            }
            return args[0];
        }
        throw RuntimeError("conj requires 1 argument");
    }

    if (name == "complex") {
        if (args.size() == 2) {
            double real = toDouble(args[0]);
            double imag = toDouble(args[1]);
            return Complex(real, imag);
        }
        throw RuntimeError("complex requires 2 arguments");
    }

    // Variable management commands
    if (name == "who") {
        std::cout << "Variables in workspace:" << std::endl;
        for (const auto& [name, value] : variables_) {
            if (name.find("__") != 0) {  // Skip internal variables
                std::cout << "  " << name << std::endl;
            }
        }
        return 0.0;
    }

    if (name == "whos") {
        std::cout << "Variables in workspace:" << std::endl;
        std::cout << "  Name        Size        Bytes    Class" << std::endl;
        std::cout << "  ----        ----        -----    -----" << std::endl;
        for (const auto& [name, value] : variables_) {
            if (name.find("__") != 0) {  // Skip internal variables
                std::cout << "  " << std::left << std::setw(12) << name;
                if (std::holds_alternative<Matrix>(value)) {
                    const Matrix& m = std::get<Matrix>(value);
                    std::cout << std::setw(12) << (std::to_string(m.rows()) + "x" + std::to_string(m.cols()));
                    std::cout << std::setw(9) << (m.size() * sizeof(double));
                    std::cout << "double";
                } else if (std::holds_alternative<double>(value)) {
                    std::cout << std::setw(12) << "1x1";
                    std::cout << std::setw(9) << sizeof(double);
                    std::cout << "double";
                } else if (std::holds_alternative<std::string>(value)) {
                    std::cout << std::setw(12) << "1x1";
                    std::cout << std::setw(9) << std::get<std::string>(value).size();
                    std::cout << "string";
                } else if (std::holds_alternative<Complex>(value)) {
                    std::cout << std::setw(12) << "1x1";
                    std::cout << std::setw(9) << sizeof(Complex);
                    std::cout << "complex";
                } else if (std::holds_alternative<bool>(value)) {
                    std::cout << std::setw(12) << "1x1";
                    std::cout << std::setw(9) << sizeof(bool);
                    std::cout << "bool";
                }
                std::cout << std::endl;
            }
        }
        return 0.0;
    }

    if (name == "clear") {
        if (args.empty()) {
            // Clear all user variables (keep built-in constants)
            std::vector<std::string> toRemove;
            for (const auto& [name, value] : variables_) {
                if (name != "pi" && name != "eps" && name != "inf" && name != "nan") {
                    toRemove.push_back(name);
                }
            }
            for (const auto& name : toRemove) {
                variables_.erase(name);
            }
        } else {
            // Clear specific variables or all
            for (const auto& arg : args) {
                std::string varName;
                if (std::holds_alternative<std::string>(arg)) {
                    varName = std::get<std::string>(arg);
                } else {
                    varName = valueToString(arg);
                }
                if (varName == "all") {
                    // Clear all variables and functions
                    variables_.clear();
                    functions_.clear();
                    // Restore built-in constants
                    variables_["pi"] = M_PI;
                    variables_["eps"] = std::numeric_limits<double>::epsilon();
                    variables_["inf"] = std::numeric_limits<double>::infinity();
                    variables_["nan"] = std::numeric_limits<double>::quiet_NaN();
                } else {
                    // Delete variable if exists (MATLAB compatible: no error if not exists)
                    variables_.erase(varName);
                    // Also try to delete function if exists
                    functions_.erase(varName);
                }
            }
        }
        return 0.0;
    }

    if (name == "figure") {
        int figNum = -1;
        if (!args.empty()) {
            figNum = static_cast<int>(toDouble(args[0]));
        }
        plotManager_->figure(figNum);
        return static_cast<double>(figNum > 0 ? figNum : 1);
    }

    if (name == "plot") {
        // Support multiple curves: plot(x1, y1, x2, y2, ...) or plot(x1, y1, 'r-', x2, y2, 'g--')
        if (args.empty()) return 0.0;
        
        // Check if we have multiple curves (more than 2 args and alternating matrix/string pattern)
        bool hasMultipleCurves = false;
        if (args.size() >= 4) {
            // Check for pattern: matrix, matrix, [string], matrix, matrix, [string], ...
            size_t matrixCount = 0;
            for (const auto& arg : args) {
                if (std::holds_alternative<Matrix>(arg)) matrixCount++;
            }
            // If we have 4+ matrices or 2+ pairs of x,y, it's multiple curves
            hasMultipleCurves = matrixCount >= 4 || (matrixCount >= 2 && args.size() >= 4);
        }
        
        if (hasMultipleCurves) {
            // Parse multiple curves - enable hold mode to keep all curves
            // First, ensure a figure exists by calling figure()
            plotManager_->figure();
            
            // Enable hold mode
            plotManager_->hold(true);
            
            // Parse multiple curves
            size_t i = 0;
            while (i < args.size()) {
                PlotStyle style;
                std::vector<double> x, y;
                
                // Get x and y
                if (i < args.size() && std::holds_alternative<Matrix>(args[i])) {
                    const Matrix& mx = std::get<Matrix>(args[i]);
                    x.reserve(mx.size());
                    for (size_t k = 0; k < mx.size(); k++) {
                        x.push_back(mx(k));
                    }
                    i++;
                } else {
                    break; // Invalid pattern
                }
                
                if (i < args.size() && std::holds_alternative<Matrix>(args[i])) {
                    const Matrix& my = std::get<Matrix>(args[i]);
                    y.reserve(my.size());
                    for (size_t k = 0; k < my.size(); k++) {
                        y.push_back(my(k));
                    }
                    i++;
                } else {
                    break; // Invalid pattern
                }
                
                // Check for style string (MATLAB 'r' is CharArray, not std::string)
                if (i < args.size()) {
                    std::string styleStr;
                    bool hasStyle = false;
                    if (std::holds_alternative<std::string>(args[i])) {
                        styleStr = std::get<std::string>(args[i]);
                        hasStyle = true;
                    } else if (std::holds_alternative<std::shared_ptr<CharArray>>(args[i])) {
                        styleStr = std::get<std::shared_ptr<CharArray>>(args[i])->toString();
                        hasStyle = true;
                    }
                    if (hasStyle) {
                        style.parseSpec(styleStr);
                        i++;
                    }
                }
                
                if (!x.empty() && !y.empty()) {
                    plotManager_->plot(x, y, style);
                }
            }
            
            // Disable hold mode after plotting all curves
            plotManager_->hold(false);
        } else {
            // Single curve
            PlotStyle style;
            std::vector<double> x, y;
            
            if (args.size() == 1) {
                y = toMatrix(args[0]).toStdVector();
                for (size_t i = 0; i < y.size(); i++) x.push_back(static_cast<double>(i));
            } else if (args.size() >= 2) {
                x = toMatrix(args[0]).toStdVector();
                y = toMatrix(args[1]).toStdVector();
                if (args.size() >= 3 && std::holds_alternative<std::string>(args[2])) {
                    style.parseSpec(std::get<std::string>(args[2]));
                }
            }
            plotManager_->plot(x, y, style);
        }
        return 0.0;
    }

    if (name == "scatter") {
        PlotStyle style;
        std::vector<double> x, y;
        
        if (args.size() >= 2) {
            x = toMatrix(args[0]).toStdVector();
            y = toMatrix(args[1]).toStdVector();
            if (args.size() >= 3 && std::holds_alternative<std::string>(args[2])) {
                style.parseSpec(std::get<std::string>(args[2]));
            }
        }
        plotManager_->scatter(x, y, style);
        return 0.0;
    }

    if (name == "bar") {
        PlotStyle style;
        std::vector<double> x, y;
        
        if (args.size() == 1) {
            y = toMatrix(args[0]).toStdVector();
            for (size_t i = 0; i < y.size(); i++) x.push_back(static_cast<double>(i));
        } else if (args.size() >= 2) {
            x = toMatrix(args[0]).toStdVector();
            y = toMatrix(args[1]).toStdVector();
        }
        plotManager_->bar(x, y, style);
        return 0.0;
    }

    if (name == "histogram" || name == "hist") {
        PlotStyle style;
        std::vector<double> data = toMatrix(args[0]).toStdVector();
        int bins = 10;
        if (args.size() >= 2) {
            bins = static_cast<int>(toDouble(args[1]));
        }
        plotManager_->hist(data, bins, style);
        return 0.0;
    }

    if (name == "title") {
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            plotManager_->title(std::get<std::string>(args[0]));
        }
        return 0.0;
    }

    if (name == "xlabel") {
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            plotManager_->xlabel(std::get<std::string>(args[0]));
        }
        return 0.0;
    }

    if (name == "ylabel") {
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            plotManager_->ylabel(std::get<std::string>(args[0]));
        }
        return 0.0;
    }

    if (name == "legend") {
        std::vector<std::string> labels;
        for (const auto& arg : args) {
            if (std::holds_alternative<std::string>(arg)) {
                labels.push_back(std::get<std::string>(arg));
            }
        }
        plotManager_->legend(labels);
        return 0.0;
    }

    if (name == "grid") {
        bool on = true;
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            on = (std::get<std::string>(args[0]) == "on");
        }
        plotManager_->grid(on);
        return 0.0;
    }

    if (name == "hold") {
        bool on = true;
        if (!args.empty() && std::holds_alternative<std::string>(args[0])) {
            on = (std::get<std::string>(args[0]) == "on");
        }
        plotManager_->hold(on);
        return 0.0;
    }

    if (name == "xlim") {
        if (args.size() >= 2) {
            plotManager_->xlim(toDouble(args[0]), toDouble(args[1]));
        }
        return 0.0;
    }

    if (name == "ylim") {
        if (args.size() >= 2) {
            plotManager_->ylim(toDouble(args[0]), toDouble(args[1]));
        }
        return 0.0;
    }

    if (name == "axis") {
        if (args.size() == 1 && std::holds_alternative<Matrix>(args[0])) {
            Matrix m = toMatrix(args[0]);
            std::vector<double> limits;
            for (size_t i = 0; i < m.size() && i < 4; i++) {
                limits.push_back(m(i));
            }
            plotManager_->axis(limits);
        }
        return 0.0;
    }

    if (name == "clf") {
        plotManager_->clf();
        return 0.0;
    }

    if (name == "subplot") {
        if (args.size() >= 3) {
            int rows = static_cast<int>(toDouble(args[0]));
            int cols = static_cast<int>(toDouble(args[1]));
            int index = static_cast<int>(toDouble(args[2]));
            plotManager_->subplot(rows, cols, index);
        }
        return 0.0;
    }

    if (name == "show") {
        plotManager_->show();
        return 0.0;
    }

    if (name == "close") {
        int figNum = -1;
        if (!args.empty()) {
            figNum = static_cast<int>(toDouble(args[0]));
        }
        plotManager_->close(figNum);
        return 0.0;
    }

    if (name == "struct") {
        // Create empty struct
        return std::make_shared<StructArray>();
    }

    if (name == "isstruct") {
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<StructArray>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("isstruct requires 1 argument");
    }

    if (name == "fieldnames") {
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<StructArray>>(args[0])) {
                auto s = std::get<std::shared_ptr<StructArray>>(args[0]);
                auto names = s->getFieldNames();
                Matrix result(1, names.size());
                for (size_t i = 0; i < names.size(); ++i) {
                    result(0, i) = static_cast<double>(i + 1);
                }
                return result;
            }
            throw RuntimeError("fieldnames requires a struct argument");
        }
        throw RuntimeError("fieldnames requires 1 argument");
    }

    if (name == "cell") {
        // Create empty cell or cell with specified size
        if (args.empty()) {
            return std::make_shared<Cell>();
        }
        if (args.size() == 1) {
            size_t size = static_cast<size_t>(toDouble(args[0]));
            return std::make_shared<Cell>(size);
        }
        if (args.size() == 2) {
            size_t rows = static_cast<size_t>(toDouble(args[0]));
            size_t cols = static_cast<size_t>(toDouble(args[1]));
            return std::make_shared<Cell>(rows, cols);
        }
        throw RuntimeError("cell requires 0, 1 or 2 arguments");
    }

    if (name == "iscell") {
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<Cell>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("iscell requires 1 argument");
    }

    if (name == "table") {
        // Create empty table
        return std::make_shared<Table>();
    }

    if (name == "istable") {
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<Table>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("istable requires 1 argument");
    }

    if (name == "height") {
        // Get number of rows in a table
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<Table>>(args[0])) {
                auto t = std::get<std::shared_ptr<Table>>(args[0]);
                return static_cast<double>(t->numRows());
            }
            throw RuntimeError("height requires a table argument");
        }
        throw RuntimeError("height requires 1 argument");
    }

    if (name == "width") {
        // Get number of columns in a table
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<Table>>(args[0])) {
                auto t = std::get<std::shared_ptr<Table>>(args[0]);
                return static_cast<double>(t->numColumns());
            }
            throw RuntimeError("width requires a table argument");
        }
        throw RuntimeError("width requires 1 argument");
    }

    if (name == "addrow") {
        // Add a row to a table: addrow(T, value1, value2, ...)
        if (args.size() >= 2) {
            if (std::holds_alternative<std::shared_ptr<Table>>(args[0])) {
                auto t = std::get<std::shared_ptr<Table>>(args[0]);
                std::vector<Value> rowData;
                for (size_t i = 1; i < args.size(); ++i) {
                    rowData.push_back(args[i]);
                }
                t->addRow(rowData);
                return t;
            }
            throw RuntimeError("addrow requires a table as first argument");
        }
        throw RuntimeError("addrow requires at least 2 arguments");
    }

    if (name == "removerow") {
        // Remove a row from a table: removerow(T, rowIndex)
        if (args.size() == 2) {
            if (std::holds_alternative<std::shared_ptr<Table>>(args[0])) {
                auto t = std::get<std::shared_ptr<Table>>(args[0]);
                size_t rowIdx = static_cast<size_t>(toDouble(args[1]));
                t->removeRow(rowIdx);
                return t;
            }
            throw RuntimeError("removerow requires a table as first argument");
        }
        throw RuntimeError("removerow requires 2 arguments");
    }

    if (name == "getrow") {
        // Get a row from a table as a cell array: getrow(T, rowIndex)
        if (args.size() == 2) {
            if (std::holds_alternative<std::shared_ptr<Table>>(args[0])) {
                auto t = std::get<std::shared_ptr<Table>>(args[0]);
                size_t rowIdx = static_cast<size_t>(toDouble(args[1]));
                return t->getRow(rowIdx);
            }
            throw RuntimeError("getrow requires a table as first argument");
        }
        throw RuntimeError("getrow requires 2 arguments");
    }

    if (name == "categorical") {
        // Create categorical from cell array of strings
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<Cell>>(args[0])) {
                auto cell = std::get<std::shared_ptr<Cell>>(args[0]);
                auto cat = std::make_shared<Categorical>();
                for (size_t i = 1; i <= cell->size(); ++i) {
                    Value elem = cell->getElement(i);
                    if (std::holds_alternative<std::string>(elem)) {
                        cat->addValue(std::get<std::string>(elem));
                    }
                }
                return cat;
            }
        }
        throw RuntimeError("categorical requires 1 argument (cell array)");
    }

    if (name == "iscategorical") {
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<Categorical>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("iscategorical requires 1 argument");
    }

    if (name == "categories") {
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<Categorical>>(args[0])) {
                auto cat = std::get<std::shared_ptr<Categorical>>(args[0]);
                auto cats = cat->getCategories();
                // Return as cell array
                auto cell = std::make_shared<Cell>(cats.size());
                for (size_t i = 0; i < cats.size(); ++i) {
                    cell->setElement(i + 1, cats[i]);
                }
                return cell;
            }
            throw RuntimeError("categories requires a categorical argument");
        }
        throw RuntimeError("categories requires 1 argument");
    }

    if (name == "datetime") {
        // Create datetime from string
        if (args.size() == 1) {
            if (std::holds_alternative<std::string>(args[0])) {
                return std::make_shared<DateTime>(std::get<std::string>(args[0]));
            }
        }
        throw RuntimeError("datetime requires 1 argument (string)");
    }

    if (name == "isdatetime") {
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<DateTime>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("isdatetime requires 1 argument");
    }

    if (name == "years") {
        if (args.size() == 1) {
            return std::make_shared<Duration>(Duration::fromYears(toDouble(args[0])));
        }
        throw RuntimeError("years requires 1 argument");
    }

    if (name == "days") {
        if (args.size() == 1) {
            return std::make_shared<Duration>(Duration::fromDays(toDouble(args[0])));
        }
        throw RuntimeError("days requires 1 argument");
    }

    if (name == "hours") {
        if (args.size() == 1) {
            return std::make_shared<Duration>(Duration::fromHours(toDouble(args[0])));
        }
        throw RuntimeError("hours requires 1 argument");
    }

    if (name == "minutes") {
        if (args.size() == 1) {
            return std::make_shared<Duration>(Duration::fromMinutes(toDouble(args[0])));
        }
        throw RuntimeError("minutes requires 1 argument");
    }

    if (name == "seconds") {
        if (args.size() == 1) {
            return std::make_shared<Duration>(Duration::fromSeconds(toDouble(args[0])));
        }
        throw RuntimeError("seconds requires 1 argument");
    }

    if (name == "isduration") {
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<Duration>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("isduration requires 1 argument");
    }

    // NDArray functions
    if (name == "ndarray") {
        // Create NDArray with specified dimensions
        if (args.empty()) {
            return std::make_shared<NDArray>();
        }
        std::vector<size_t> dims;
        for (const auto& arg : args) {
            dims.push_back(static_cast<size_t>(toDouble(arg)));
        }
        return std::make_shared<NDArray>(dims);
    }

    if (name == "isndarray") {
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<NDArray>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("isndarray requires 1 argument");
    }

    if (name == "ndims") {
        // Get number of dimensions
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<NDArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<NDArray>>(args[0]);
                return static_cast<double>(arr->ndims());
            }
            if (std::holds_alternative<Matrix>(args[0])) {
                return 2.0;  // Matrix is always 2D
            }
            throw RuntimeError("ndims requires an array argument");
        }
        throw RuntimeError("ndims requires 1 argument");
    }

    if (name == "numel") {
        // Get total number of elements
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<NDArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<NDArray>>(args[0]);
                return static_cast<double>(arr->numel());
            }
            if (std::holds_alternative<Matrix>(args[0])) {
                auto mat = std::get<Matrix>(args[0]);
                return static_cast<double>(mat.size());
            }
            throw RuntimeError("numel requires an array argument");
        }
        throw RuntimeError("numel requires 1 argument");
    }

    if (name == "squeeze") {
        // Remove singleton dimensions
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<NDArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<NDArray>>(args[0]);
                return std::make_shared<NDArray>(arr->squeeze());
            }
            throw RuntimeError("squeeze requires an NDArray argument");
        }
        throw RuntimeError("squeeze requires 1 argument");
    }

    if (name == "permute") {
        // Permute dimensions: permute(A, dim1, dim2, ...)
        if (args.size() >= 2) {
            if (std::holds_alternative<std::shared_ptr<NDArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<NDArray>>(args[0]);
                std::vector<size_t> order;
                for (size_t i = 1; i < args.size(); ++i) {
                    order.push_back(static_cast<size_t>(toDouble(args[i])));
                }
                return std::make_shared<NDArray>(arr->permute(order));
            }
            throw RuntimeError("permute requires an NDArray as first argument");
        }
        throw RuntimeError("permute requires at least 2 arguments");
    }

    if (name == "reshape") {
        // Reshape array: reshape(A, dim1, dim2, ...)
        if (args.size() >= 2) {
            std::vector<size_t> newDims;
            for (size_t i = 1; i < args.size(); ++i) {
                newDims.push_back(static_cast<size_t>(toDouble(args[i])));
            }
            if (std::holds_alternative<std::shared_ptr<NDArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<NDArray>>(args[0]);
                return std::make_shared<NDArray>(arr->reshape(newDims));
            }
            if (std::holds_alternative<Matrix>(args[0])) {
                auto mat = std::get<Matrix>(args[0]);
                NDArray arr(mat);
                return std::make_shared<NDArray>(arr.reshape(newDims));
            }
            throw RuntimeError("reshape requires an array as first argument");
        }
        throw RuntimeError("reshape requires at least 2 arguments");
    }

    // Regex functions
    if (name == "regexp") {
        // regexp(str, pattern, options) - find matches with options
        // options can be: 'match', 'start', 'end', 'tokens', 'split'
        if (args.size() >= 2) {
            std::string str = toString(args[0]);
            std::string pattern = toString(args[1]);
            std::string option = "match";  // default
            if (args.size() >= 3) {
                option = toString(args[2]);
            }
            
            Regex regex(pattern);
            auto matches = regex.matchAll(str);
            
            if (option == "match") {
                // Return matches as cell array
                auto result = std::make_shared<Cell>(matches.size());
                for (size_t i = 0; i < matches.size(); ++i) {
                    result->setElement(i + 1, matches[i].match);
                }
                return result;
            } else if (option == "start") {
                // Return start positions
                Matrix result(matches.size(), 1);
                for (size_t i = 0; i < matches.size(); ++i) {
                    result(i, 0) = static_cast<double>(matches[i].start + 1);  // 1-based
                }
                return result;
            } else if (option == "end") {
                // Return end positions
                Matrix result(matches.size(), 1);
                for (size_t i = 0; i < matches.size(); ++i) {
                    result(i, 0) = static_cast<double>(matches[i].end);  // 1-based, inclusive
                }
                return result;
            } else if (option == "tokens") {
                // Return captured groups as nested cell array
                auto matchesWithGroups = regex.matchAllWithGroups(str);
                auto result = std::make_shared<Cell>(matchesWithGroups.size());
                for (size_t i = 0; i < matchesWithGroups.size(); ++i) {
                    auto groupCell = std::make_shared<Cell>(matchesWithGroups[i].groups.size());
                    for (size_t j = 0; j < matchesWithGroups[i].groups.size(); ++j) {
                        groupCell->setElement(j + 1, matchesWithGroups[i].groups[j]);
                    }
                    result->setElement(i + 1, groupCell);
                }
                return result;
            } else if (option == "split") {
                // Split by pattern
                auto parts = regex.split(str);
                auto result = std::make_shared<Cell>(parts.size());
                for (size_t i = 0; i < parts.size(); ++i) {
                    result->setElement(i + 1, parts[i]);
                }
                return result;
            } else {
                throw RuntimeError("regexp: unknown option '" + option + "'");
            }
        }
        throw RuntimeError("regexp requires at least 2 arguments");
    }

    if (name == "regexpi") {
        // regexpi(str, pattern, options) - case-insensitive match
        if (args.size() >= 2) {
            std::string str = toString(args[0]);
            std::string pattern = toString(args[1]);
            std::string option = "match";
            if (args.size() >= 3) {
                option = toString(args[2]);
            }
            
            // Use case-insensitive regex
            Regex regex(pattern, true);
            auto matches = regex.matchAll(str);
            
            if (option == "match") {
                auto result = std::make_shared<Cell>(matches.size());
                for (size_t i = 0; i < matches.size(); ++i) {
                    result->setElement(i + 1, matches[i].match);
                }
                return result;
            } else if (option == "start") {
                Matrix result(matches.size(), 1);
                for (size_t i = 0; i < matches.size(); ++i) {
                    result(i, 0) = static_cast<double>(matches[i].start + 1);
                }
                return result;
            } else if (option == "end") {
                Matrix result(matches.size(), 1);
                for (size_t i = 0; i < matches.size(); ++i) {
                    result(i, 0) = static_cast<double>(matches[i].end);
                }
                return result;
            } else if (option == "tokens") {
                auto matchesWithGroups = regex.matchAllWithGroups(str);
                auto result = std::make_shared<Cell>(matchesWithGroups.size());
                for (size_t i = 0; i < matchesWithGroups.size(); ++i) {
                    auto groupCell = std::make_shared<Cell>(matchesWithGroups[i].groups.size());
                    for (size_t j = 0; j < matchesWithGroups[i].groups.size(); ++j) {
                        groupCell->setElement(j + 1, matchesWithGroups[i].groups[j]);
                    }
                    result->setElement(i + 1, groupCell);
                }
                return result;
            } else if (option == "split") {
                auto parts = regex.split(str);
                auto result = std::make_shared<Cell>(parts.size());
                for (size_t i = 0; i < parts.size(); ++i) {
                    result->setElement(i + 1, parts[i]);
                }
                return result;
            } else {
                throw RuntimeError("regexpi: unknown option '" + option + "'");
            }
        }
        throw RuntimeError("regexpi requires at least 2 arguments");
    }

    if (name == "regexprep") {
        // regexprep(str, pattern, replacement) - replace matches
        if (args.size() >= 3) {
            std::string str = toString(args[0]);
            std::string pattern = toString(args[1]);
            std::string replacement = toString(args[2]);
            Regex regex(pattern);
            return regex.replace(str, replacement);
        }
        throw RuntimeError("regexprep requires 3 arguments");
    }

    if (name == "regexptranslate") {
        // regexptranslate(type, str) - escape special characters
        if (args.size() >= 2) {
            std::string type = toString(args[0]);
            std::string str = toString(args[1]);
            if (type == "escape") {
                return Regex::escape(str);
            }
            throw RuntimeError("regexptranslate: unknown type " + type);
        }
        throw RuntimeError("regexptranslate requires 2 arguments");
    }

    if (name == "regex") {
        // Create regex object
        if (args.size() >= 1) {
            std::string pattern = toString(args[0]);
            return std::make_shared<Regex>(pattern);
        }
        throw RuntimeError("regex requires 1 argument");
    }

    if (name == "isregex") {
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<Regex>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("isregex requires 1 argument");
    }

    // Common regex patterns
    if (name == "regexpattern") {
        // Return common regex patterns
        if (args.size() >= 1) {
            std::string type = toString(args[0]);
            if (type == "email") {
                return std::string(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
            } else if (type == "url") {
                return std::string(R"(https?://[a-zA-Z0-9.-]+(?:/[\w./?%&=-]*)?)");
            } else if (type == "ip") {
                return std::string(R"((?:\d{1,3}\.){3}\d{1,3})");
            } else if (type == "phone") {
                return std::string(R"(\+?\d{1,3}[-.\s]?\(?\d{3}\)?[-.\s]?\d{3}[-.\s]?\d{4})");
            } else if (type == "date") {
                return std::string(R"(\d{4}-\d{2}-\d{2}|\d{2}/\d{2}/\d{4})");
            } else if (type == "time") {
                return std::string(R"(\d{1,2}:\d{2}(?::\d{2})?)");
            } else if (type == "number") {
                return std::string(R"(-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)");
            } else if (type == "word") {
                return std::string(R"(\b\w+\b)");
            } else if (type == "whitespace") {
                return std::string(R"(\s+)");
            } else {
                throw RuntimeError("regexpattern: unknown pattern type '" + type + "'");
            }
        }
        throw RuntimeError("regexpattern requires 1 argument");
    }

    // String array functions
    if (name == "string") {
        // Create string array from arguments
        std::vector<std::string> strings;
        for (const auto& arg : args) {
            strings.push_back(toString(arg));
        }
        return std::make_shared<StringArray>(strings);
    }

    if (name == "isstring") {
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<StringArray>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("isstring requires 1 argument");
    }

    if (name == "strjoin") {
        // Join strings with delimiter
        if (args.size() >= 1) {
            std::string delimiter = "";
            if (args.size() >= 2) {
                delimiter = toString(args[1]);
            }
            if (std::holds_alternative<std::shared_ptr<StringArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<StringArray>>(args[0]);
                return arr->join(delimiter);
            }
            throw RuntimeError("strjoin requires a string array as first argument");
        }
        throw RuntimeError("strjoin requires at least 1 argument");
    }

    if (name == "strsplit") {
        // Split string by delimiter
        if (args.size() >= 1) {
            std::string str = toString(args[0]);
            std::string delimiter = " ";
            if (args.size() >= 2) {
                delimiter = toString(args[1]);
            }
            
            std::vector<std::string> parts;
            size_t start = 0;
            size_t end = str.find(delimiter);
            
            while (end != std::string::npos) {
                parts.push_back(str.substr(start, end - start));
                start = end + delimiter.length();
                end = str.find(delimiter, start);
            }
            parts.push_back(str.substr(start));
            
            return std::make_shared<StringArray>(parts);
        }
        throw RuntimeError("strsplit requires at least 1 argument");
    }

    if (name == "strlength") {
        // Get length of each string in array
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<StringArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<StringArray>>(args[0]);
                Matrix result(arr->size(), 1);
                for (size_t i = 0; i < arr->size(); ++i) {
                    result(i, 0) = static_cast<double>(arr->get(i + 1).length());
                }
                return result;
            }
            if (std::holds_alternative<std::string>(args[0])) {
                return static_cast<double>(std::get<std::string>(args[0]).length());
            }
            throw RuntimeError("strlength requires a string or string array argument");
        }
        throw RuntimeError("strlength requires 1 argument");
    }

    if (name == "upper") {
        // Convert to uppercase
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<StringArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<StringArray>>(args[0]);
                std::vector<std::string> upperStrings;
                for (size_t i = 0; i < arr->size(); ++i) {
                    std::string s = arr->get(i + 1);
                    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
                    upperStrings.push_back(s);
                }
                return std::make_shared<StringArray>(upperStrings);
            }
            std::string s = toString(args[0]);
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return s;
        }
        throw RuntimeError("upper requires 1 argument");
    }

    if (name == "lower") {
        // Convert to lowercase
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<StringArray>>(args[0])) {
                auto arr = std::get<std::shared_ptr<StringArray>>(args[0]);
                std::vector<std::string> lowerStrings;
                for (size_t i = 0; i < arr->size(); ++i) {
                    std::string s = arr->get(i + 1);
                    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                    lowerStrings.push_back(s);
                }
                return std::make_shared<StringArray>(lowerStrings);
            }
            std::string s = toString(args[0]);
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        }
        throw RuntimeError("lower requires 1 argument");
    }

    if (name == "contains") {
        // Check if string contains substring
        if (args.size() >= 2) {
            std::string str = toString(args[0]);
            std::string pattern = toString(args[1]);
            return str.find(pattern) != std::string::npos ? 1.0 : 0.0;
        }
        throw RuntimeError("contains requires 2 arguments");
    }

    if (name == "startsWith") {
        // Check if string starts with prefix
        if (args.size() >= 2) {
            std::string str = toString(args[0]);
            std::string prefix = toString(args[1]);
            return str.find(prefix) == 0 ? 1.0 : 0.0;
        }
        throw RuntimeError("startsWith requires 2 arguments");
    }

    if (name == "endsWith") {
        // Check if string ends with suffix
        if (args.size() >= 2) {
            std::string str = toString(args[0]);
            std::string suffix = toString(args[1]);
            if (str.length() >= suffix.length()) {
                return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0 ? 1.0 : 0.0;
            }
            return 0.0;
        }
        throw RuntimeError("endsWith requires 2 arguments");
    }

    // Sparse matrix functions
    if (name == "sparse") {
        // sparse(i, j, v, m, n) - create from coordinates
        // sparse(A) - convert from dense
        // sparse(m, n) - create empty
        if (args.size() == 5) {
            // sparse(i, j, v, m, n)
            auto i_mat = std::get<Matrix>(args[0]);
            auto j_mat = std::get<Matrix>(args[1]);
            auto v_mat = std::get<Matrix>(args[2]);
            size_t m = static_cast<size_t>(toDouble(args[3]));
            size_t n = static_cast<size_t>(toDouble(args[4]));
            
            std::vector<size_t> row_indices, col_indices;
            std::vector<double> values;
            
            for (size_t idx = 0; idx < i_mat.size(); ++idx) {
                row_indices.push_back(static_cast<size_t>(i_mat(idx)));
                col_indices.push_back(static_cast<size_t>(j_mat(idx)));
                values.push_back(v_mat(idx));
            }
            
            return SparseMatrix::fromCOO(m, n, row_indices, col_indices, values);
        } else if (args.size() == 3) {
            // sparse(i, j, v) - infer dimensions
            auto i_mat = std::get<Matrix>(args[0]);
            auto j_mat = std::get<Matrix>(args[1]);
            auto v_mat = std::get<Matrix>(args[2]);
            
            std::vector<size_t> row_indices, col_indices;
            std::vector<double> values;
            size_t max_row = 0, max_col = 0;
            
            for (size_t idx = 0; idx < i_mat.size(); ++idx) {
                size_t r = static_cast<size_t>(i_mat(idx));
                size_t c = static_cast<size_t>(j_mat(idx));
                row_indices.push_back(r);
                col_indices.push_back(c);
                values.push_back(v_mat(idx));
                max_row = std::max(max_row, r);
                max_col = std::max(max_col, c);
            }
            
            return SparseMatrix::fromCOO(max_row, max_col, row_indices, col_indices, values);
        } else if (args.size() == 1) {
            if (std::holds_alternative<Matrix>(args[0])) {
                // Convert dense to sparse
                return SparseMatrix::fromDense(std::get<Matrix>(args[0]));
            } else if (std::holds_alternative<std::shared_ptr<SparseMatrix>>(args[0])) {
                // Already sparse, return as-is
                return args[0];
            }
            throw RuntimeError("sparse requires a matrix argument");
        } else if (args.size() == 2) {
            // sparse(m, n) - create empty sparse matrix
            size_t m = static_cast<size_t>(toDouble(args[0]));
            size_t n = static_cast<size_t>(toDouble(args[1]));
            return std::make_shared<SparseMatrix>(m, n);
        }
        throw RuntimeError("sparse requires 1, 2, 3, or 5 arguments");
    }

    if (name == "speye") {
        // Create sparse identity matrix
        if (args.size() == 1) {
            size_t n = static_cast<size_t>(toDouble(args[0]));
            auto sparse = std::make_shared<SparseMatrix>(n, n);
            for (size_t i = 1; i <= n; ++i) {
                sparse->set(i, i, 1.0);
            }
            return sparse;
        } else if (args.size() == 2) {
            size_t m = static_cast<size_t>(toDouble(args[0]));
            size_t n = static_cast<size_t>(toDouble(args[1]));
            auto sparse = std::make_shared<SparseMatrix>(m, n);
            size_t min_dim = std::min(m, n);
            for (size_t i = 1; i <= min_dim; ++i) {
                sparse->set(i, i, 1.0);
            }
            return sparse;
        }
        throw RuntimeError("speye requires 1 or 2 arguments");
    }

    if (name == "spdiags") {
        // spdiags(B, d, m, n) - create sparse diagonal matrix
        if (args.size() == 4) {
            auto B = std::get<Matrix>(args[0]);
            auto d = std::get<Matrix>(args[1]);
            size_t m = static_cast<size_t>(toDouble(args[2]));
            size_t n = static_cast<size_t>(toDouble(args[3]));
            
            auto sparse = std::make_shared<SparseMatrix>(m, n);
            
            for (size_t k = 0; k < d.size(); ++k) {
                int diag = static_cast<int>(d(k));
                for (size_t i = 0; i < B.rows() && i < m && i + diag < n; ++i) {
                    size_t row = i + 1;
                    size_t col = i + diag + 1;
                    if (row >= 1 && row <= m && col >= 1 && col <= n) {
                        sparse->set(row, col, B(i, k));
                    }
                }
            }
            return sparse;
        }
        throw RuntimeError("spdiags requires 4 arguments");
    }

    if (name == "nnz") {
        // Count non-zero elements
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<SparseMatrix>>(args[0])) {
                return static_cast<double>(std::get<std::shared_ptr<SparseMatrix>>(args[0])->nnz());
            } else if (std::holds_alternative<Matrix>(args[0])) {
                const auto& mat = std::get<Matrix>(args[0]);
                size_t count = 0;
                for (size_t i = 0; i < mat.rows(); ++i) {
                    for (size_t j = 0; j < mat.cols(); ++j) {
                        if (mat(i, j) != 0.0) count++;
                    }
                }
                return static_cast<double>(count);
            }
            throw RuntimeError("nnz requires a matrix argument");
        }
        throw RuntimeError("nnz requires 1 argument");
    }

    if (name == "issparse") {
        // Check if value is sparse matrix
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<SparseMatrix>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("issparse requires 1 argument");
    }

    if (name == "full") {
        // Convert sparse to dense
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<SparseMatrix>>(args[0])) {
                return std::get<std::shared_ptr<SparseMatrix>>(args[0])->toDense();
            } else if (std::holds_alternative<Matrix>(args[0])) {
                return args[0];
            }
            throw RuntimeError("full requires a matrix argument");
        }
        throw RuntimeError("full requires 1 argument");
    }

    // Logical array functions
    if (name == "logical") {
        // Convert to logical array
        if (args.size() == 1) {
            if (std::holds_alternative<Matrix>(args[0])) {
                return LogicalArray::fromDense(std::get<Matrix>(args[0]));
            } else if (std::holds_alternative<std::shared_ptr<LogicalArray>>(args[0])) {
                return args[0];
            } else if (std::holds_alternative<double>(args[0])) {
                auto logical = std::make_shared<LogicalArray>(1);
                logical->set(1, std::get<double>(args[0]) != 0.0);
                return logical;
            } else if (std::holds_alternative<bool>(args[0])) {
                auto logical = std::make_shared<LogicalArray>(1);
                logical->set(1, std::get<bool>(args[0]));
                return logical;
            }
            throw RuntimeError("logical requires a numeric or boolean argument");
        }
        throw RuntimeError("logical requires 1 argument");
    }

    if (name == "islogical") {
        // Check if value is logical array
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<LogicalArray>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("islogical requires 1 argument");
    }

    if (name == "true") {
        // Create logical true array
        if (args.size() == 0) {
            auto logical = std::make_shared<LogicalArray>(1);
            logical->set(1, true);
            return logical;
        } else if (args.size() == 1) {
            size_t n = static_cast<size_t>(toDouble(args[0]));
            auto logical = std::make_shared<LogicalArray>(n);
            for (size_t i = 1; i <= n; ++i) {
                logical->set(i, true);
            }
            return logical;
        } else if (args.size() == 2) {
            size_t rows = static_cast<size_t>(toDouble(args[0]));
            size_t cols = static_cast<size_t>(toDouble(args[1]));
            auto logical = std::make_shared<LogicalArray>(rows, cols);
            for (size_t i = 1; i <= rows * cols; ++i) {
                logical->set(i, true);
            }
            return logical;
        }
        throw RuntimeError("true requires 0, 1, or 2 arguments");
    }

    if (name == "false") {
        // Create logical false array
        if (args.size() == 0) {
            auto logical = std::make_shared<LogicalArray>(1);
            logical->set(1, false);
            return logical;
        } else if (args.size() == 1) {
            size_t n = static_cast<size_t>(toDouble(args[0]));
            auto logical = std::make_shared<LogicalArray>(n);
            // Already initialized to false
            return logical;
        } else if (args.size() == 2) {
            size_t rows = static_cast<size_t>(toDouble(args[0]));
            size_t cols = static_cast<size_t>(toDouble(args[1]));
            auto logical = std::make_shared<LogicalArray>(rows, cols);
            // Already initialized to false
            return logical;
        }
        throw RuntimeError("false requires 0, 1, or 2 arguments");
    }

    // Char array functions
    if (name == "char") {
        // Convert to char array
        if (args.size() == 1) {
            if (std::holds_alternative<std::string>(args[0])) {
                return CharArray::fromString(std::get<std::string>(args[0]));
            } else if (std::holds_alternative<std::shared_ptr<CharArray>>(args[0])) {
                return args[0];
            } else if (std::holds_alternative<Matrix>(args[0])) {
                // Convert numeric codes to characters
                const auto& mat = std::get<Matrix>(args[0]);
                std::string str;
                for (size_t i = 0; i < mat.size(); ++i) {
                    str += static_cast<char>(static_cast<int>(mat(i)));
                }
                return CharArray::fromString(str);
            } else if (std::holds_alternative<double>(args[0])) {
                int code = static_cast<int>(std::get<double>(args[0]));
                return CharArray::fromString(std::string(1, static_cast<char>(code)));
            }
            throw RuntimeError("char requires a string or numeric argument");
        } else if (args.size() > 1) {
            // Multiple strings - create char array with each as a row
            std::vector<std::string> strings;
            for (const auto& arg : args) {
                if (std::holds_alternative<std::string>(arg)) {
                    strings.push_back(std::get<std::string>(arg));
                } else {
                    throw RuntimeError("char with multiple arguments requires all string inputs");
                }
            }
            return CharArray::fromStrings(strings);
        }
        throw RuntimeError("char requires at least 1 argument");
    }

    if (name == "ischar") {
        // Check if value is char array
        if (args.size() == 1) {
            return std::holds_alternative<std::shared_ptr<CharArray>>(args[0]) ? 1.0 : 0.0;
        }
        throw RuntimeError("ischar requires 1 argument");
    }

    if (name == "isletter") {
        // Check which characters are letters
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<CharArray>>(args[0])) {
                auto charArray = std::get<std::shared_ptr<CharArray>>(args[0]);
                auto result = std::make_shared<LogicalArray>(charArray->rows(), charArray->cols());
                for (size_t i = 1; i <= charArray->size(); ++i) {
                    char c = charArray->get(i);
                    result->set(i, std::isalpha(static_cast<unsigned char>(c)) != 0);
                }
                return result;
            } else if (std::holds_alternative<std::string>(args[0])) {
                const std::string& str = std::get<std::string>(args[0]);
                auto result = std::make_shared<LogicalArray>(1, str.length());
                for (size_t i = 0; i < str.length(); ++i) {
                    result->set(i + 1, std::isalpha(static_cast<unsigned char>(str[i])) != 0);
                }
                return result;
            }
            throw RuntimeError("isletter requires a char array or string argument");
        }
        throw RuntimeError("isletter requires 1 argument");
    }

    if (name == "isspace") {
        // Check which characters are whitespace
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<CharArray>>(args[0])) {
                auto charArray = std::get<std::shared_ptr<CharArray>>(args[0]);
                auto result = std::make_shared<LogicalArray>(charArray->rows(), charArray->cols());
                for (size_t i = 1; i <= charArray->size(); ++i) {
                    char c = charArray->get(i);
                    result->set(i, std::isspace(static_cast<unsigned char>(c)) != 0);
                }
                return result;
            } else if (std::holds_alternative<std::string>(args[0])) {
                const std::string& str = std::get<std::string>(args[0]);
                auto result = std::make_shared<LogicalArray>(1, str.length());
                for (size_t i = 0; i < str.length(); ++i) {
                    result->set(i + 1, std::isspace(static_cast<unsigned char>(str[i])) != 0);
                }
                return result;
            }
            throw RuntimeError("isspace requires a char array or string argument");
        }
        throw RuntimeError("isspace requires 1 argument");
    }

    // Function handle functions
    if (name == "str2func") {
        // Convert string to function handle
        if (args.size() == 1) {
            std::string funcName = toString(args[0]);
            // Check if function exists
            if (functions_.find(funcName) == functions_.end() && !isBuiltinFunction(funcName)) {
                throw RuntimeError("Undefined function: " + funcName);
            }
            return std::make_shared<FuncHandle>(funcName);
        }
        throw RuntimeError("str2func requires 1 argument");
    }

    if (name == "func2str") {
        // Convert function handle to string
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<FuncHandle>>(args[0])) {
                return std::get<std::shared_ptr<FuncHandle>>(args[0])->toString();
            }
            throw RuntimeError("func2str requires a function handle argument");
        }
        throw RuntimeError("func2str requires 1 argument");
    }

    if (name == "isa") {
        // Check if value is of specified type
        if (args.size() == 2) {
            std::string typeName = toString(args[1]);
            if (typeName == "function_handle") {
                return std::holds_alternative<std::shared_ptr<FuncHandle>>(args[0]) ? 1.0 : 0.0;
            } else if (typeName == "char") {
                return std::holds_alternative<std::shared_ptr<CharArray>>(args[0]) ? 1.0 : 0.0;
            } else if (typeName == "logical") {
                return std::holds_alternative<std::shared_ptr<LogicalArray>>(args[0]) ? 1.0 : 0.0;
            } else if (typeName == "sparse") {
                return std::holds_alternative<std::shared_ptr<SparseMatrix>>(args[0]) ? 1.0 : 0.0;
            } else if (typeName == "string") {
                return std::holds_alternative<std::shared_ptr<StringArray>>(args[0]) ? 1.0 : 0.0;
            } else if (typeName == "cell") {
                return std::holds_alternative<std::shared_ptr<Cell>>(args[0]) ? 1.0 : 0.0;
            } else if (typeName == "struct") {
                return std::holds_alternative<std::shared_ptr<StructArray>>(args[0]) ? 1.0 : 0.0;
            } else if (typeName == "double" || typeName == "numeric") {
                return std::holds_alternative<double>(args[0]) || std::holds_alternative<Matrix>(args[0]) ? 1.0 : 0.0;
            } else if (typeName == "containers.Map" || typeName == "Map") {
                return std::holds_alternative<std::shared_ptr<Map>>(args[0]) ? 1.0 : 0.0;
            }
            return 0.0;
        }
        throw RuntimeError("isa requires 2 arguments");
    }

    // Map container functions
    if (name == "Map") {
        // Create a new Map
        // containers.Map() - empty map
        // containers.Map(keys, values) - map with initial data
        if (args.size() == 0) {
            return std::make_shared<Map>();
        } else if (args.size() == 2) {
            auto map = std::make_shared<Map>();
            // First arg should be keys (cell array of strings)
            // Second arg should be values (cell array)
            if (std::holds_alternative<std::shared_ptr<Cell>>(args[0]) &&
                std::holds_alternative<std::shared_ptr<Cell>>(args[1])) {
                auto keysCell = std::get<std::shared_ptr<Cell>>(args[0]);
                auto valuesCell = std::get<std::shared_ptr<Cell>>(args[1]);
                if (keysCell->size() != valuesCell->size()) {
                    throw RuntimeError("Keys and values must have the same size");
                }
                for (size_t i = 1; i <= keysCell->size(); ++i) {
                    std::string key = toString(keysCell->getElement(i));
                    map->set(key, valuesCell->getElement(i));
                }
            } else if (std::holds_alternative<Matrix>(args[0]) &&
                       std::holds_alternative<Matrix>(args[1])) {
                // Numeric keys - convert to string
                auto keysMat = std::get<Matrix>(args[0]);
                auto valuesMat = std::get<Matrix>(args[1]);
                if (keysMat.size() != valuesMat.size()) {
                    throw RuntimeError("Keys and values must have the same size");
                }
                for (size_t i = 0; i < keysMat.size(); ++i) {
                    std::string key = std::to_string(static_cast<int>(keysMat(i)));
                    map->set(key, valuesMat(i));
                }
            }
            return map;
        }
        throw RuntimeError("containers.Map requires 0 or 2 arguments");
    }

    if (name == "isKey") {
        // Check if key exists in map
        if (args.size() == 2) {
            if (std::holds_alternative<std::shared_ptr<Map>>(args[0])) {
                auto map = std::get<std::shared_ptr<Map>>(args[0]);
                std::string key = toString(args[1]);
                return map->isKey(key) ? 1.0 : 0.0;
            }
            throw RuntimeError("isKey requires a Map as first argument");
        }
        throw RuntimeError("isKey requires 2 arguments");
    }

    if (name == "keys") {
        // Get all keys from map
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<Map>>(args[0])) {
                auto map = std::get<std::shared_ptr<Map>>(args[0]);
                auto keys = map->keys();
                auto cell = std::make_shared<Cell>(keys.size());
                for (size_t i = 0; i < keys.size(); ++i) {
                    cell->setElement(i + 1, keys[i]);
                }
                return cell;
            }
            throw RuntimeError("keys requires a Map argument");
        }
        throw RuntimeError("keys requires 1 argument");
    }

    if (name == "values") {
        // Get all values from map
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<Map>>(args[0])) {
                auto map = std::get<std::shared_ptr<Map>>(args[0]);
                auto vals = map->values();
                auto cell = std::make_shared<Cell>(vals.size());
                for (size_t i = 0; i < vals.size(); ++i) {
                    cell->setElement(i + 1, vals[i]);
                }
                return cell;
            }
            throw RuntimeError("values requires a Map argument");
        }
        throw RuntimeError("values requires 1 argument");
    }

    if (name == "remove") {
        // Remove key from map
        if (args.size() == 2) {
            if (std::holds_alternative<std::shared_ptr<Map>>(args[0])) {
                auto map = std::get<std::shared_ptr<Map>>(args[0]);
                std::string key = toString(args[1]);
                map->remove(key);
                return std::monostate{};
            }
            throw RuntimeError("remove requires a Map as first argument");
        }
        throw RuntimeError("remove requires 2 arguments");
    }

    // File I/O functions
    if (name == "fopen") {
        // Open file: fopen(filename, mode)
        if (args.size() >= 1) {
            std::string filename = toString(args[0]);
            std::string mode = "r";
            if (args.size() >= 2) {
                mode = toString(args[1]);
            }
            auto fileHandle = std::make_shared<FileHandle>(filename, mode);
            if (!fileHandle->isOpen()) {
                throw RuntimeError("Cannot open file: " + filename);
            }
            return fileHandle;
        }
        throw RuntimeError("fopen requires at least 1 argument");
    }

    if (name == "fclose") {
        // Close file: fclose(fileID)
        if (args.size() == 1) {
            if (std::holds_alternative<std::shared_ptr<FileHandle>>(args[0])) {
                auto fileHandle = std::get<std::shared_ptr<FileHandle>>(args[0]);
                fileHandle->close();
                return 0.0;
            }
            throw RuntimeError("fclose requires a file handle");
        }
        throw RuntimeError("fclose requires 1 argument");
    }

    if (name == "fprintf") {
        // Format and write to file or stdout
        if (args.size() >= 1) {
            // Check if first argument is a file handle
            if (std::holds_alternative<std::shared_ptr<FileHandle>>(args[0])) {
                // fprintf(fileID, format, ...)
                if (args.size() < 2) {
                    throw RuntimeError("fprintf requires format string");
                }
                auto fileHandle = std::get<std::shared_ptr<FileHandle>>(args[0]);
                if (!fileHandle->isOpen()) {
                    throw RuntimeError("File is not open");
                }
                
                // Format the string manually (same logic as sprintf)
                std::string format = toString(args[1]);
                std::string result;
                size_t argIdx = 2;
                for (size_t i = 0; i < format.length(); ++i) {
                    if (format[i] == '%' && i + 1 < format.length()) {
                        char spec = format[i + 1];
                        if (spec == '%') {
                            result += '%';
                            i++;
                        } else if (argIdx < args.size()) {
                            std::ostringstream oss;
                            if (spec == 'd' || spec == 'i') {
                                oss << static_cast<int>(toDouble(args[argIdx]));
                            } else if (spec == 'f') {
                                oss << std::fixed << toDouble(args[argIdx]);
                            } else if (spec == 'e') {
                                oss << std::scientific << toDouble(args[argIdx]);
                            } else if (spec == 'g') {
                                oss << toDouble(args[argIdx]);
                            } else if (spec == 's') {
                                if (std::holds_alternative<std::string>(args[argIdx])) {
                                    oss << std::get<std::string>(args[argIdx]);
                                } else {
                                    oss << valueToString(args[argIdx]);
                                }
                            } else if (spec == 'c') {
                                if (std::holds_alternative<std::string>(args[argIdx]) && 
                                    std::get<std::string>(args[argIdx]).length() > 0) {
                                    oss << std::get<std::string>(args[argIdx])[0];
                                }
                            } else {
                                result += format[i];
                                continue;
                            }
                            result += oss.str();
                            argIdx++;
                            i++;
                        } else {
                            result += format[i];
                        }
                    } else {
                        result += format[i];
                    }
                }
                
                if (fileHandle->handle()) {
                    fprintf(fileHandle->handle(), "%s", result.c_str());
                }
                return static_cast<double>(result.length());
            } else {
                // fprintf(format, ...) - write to stdout
                // First argument should be the format string
                std::string format = toString(args[0]);
                std::string result;
                size_t argIdx = 1;
                for (size_t i = 0; i < format.length(); ++i) {
                    if (format[i] == '%' && i + 1 < format.length()) {
                        char spec = format[i + 1];
                        if (spec == '%') {
                            result += '%';
                            i++;
                        } else if (argIdx < args.size()) {
                            std::ostringstream oss;
                            if (spec == 'd' || spec == 'i') {
                                oss << static_cast<int>(toDouble(args[argIdx]));
                            } else if (spec == 'f') {
                                oss << std::fixed << toDouble(args[argIdx]);
                            } else if (spec == 'e') {
                                oss << std::scientific << toDouble(args[argIdx]);
                            } else if (spec == 'g') {
                                oss << toDouble(args[argIdx]);
                            } else if (spec == 's') {
                                if (std::holds_alternative<std::string>(args[argIdx])) {
                                    oss << std::get<std::string>(args[argIdx]);
                                } else {
                                    oss << valueToString(args[argIdx]);
                                }
                            } else if (spec == 'c') {
                                if (std::holds_alternative<std::string>(args[argIdx]) && 
                                    std::get<std::string>(args[argIdx]).length() > 0) {
                                    oss << std::get<std::string>(args[argIdx])[0];
                                }
                            } else {
                                result += format[i];
                                continue;
                            }
                            result += oss.str();
                            argIdx++;
                            i++;
                        } else {
                            result += format[i];
                        }
                    } else {
                        result += format[i];
                    }
                }
                std::cout << result;
                return static_cast<double>(result.length());
            }
        }
        throw RuntimeError("fprintf requires at least 1 argument");
    }

    if (name == "fscanf") {
        // Read formatted data from file
        if (args.size() >= 2) {
            if (std::holds_alternative<std::shared_ptr<FileHandle>>(args[0])) {
                auto fileHandle = std::get<std::shared_ptr<FileHandle>>(args[0]);
                if (!fileHandle->isOpen()) {
                    throw RuntimeError("File is not open");
                }
                
                std::string format = toString(args[1]);
                
                // Simple implementation: read a line and parse
                char buffer[1024];
                if (fgets(buffer, sizeof(buffer), fileHandle->handle())) {
                    std::string line(buffer);
                    // Remove newline
                    if (!line.empty() && line.back() == '\n') {
                        line.pop_back();
                    }
                    
                    // Try to parse as number if format suggests
                    if (format.find("%d") != std::string::npos || 
                        format.find("%f") != std::string::npos ||
                        format.find("%g") != std::string::npos ||
                        format.find("%e") != std::string::npos) {
                        try {
                            return std::stod(line);
                        } catch (...) {
                            return line;
                        }
                    }
                    return line;
                }
                return std::monostate{}; // EOF
            }
            throw RuntimeError("fscanf requires a file handle as first argument");
        }
        throw RuntimeError("fscanf requires at least 2 arguments");
    }

    if (name == "fread") {
        // Read binary data from file
        if (args.size() >= 2) {
            if (std::holds_alternative<std::shared_ptr<FileHandle>>(args[0])) {
                auto fileHandle = std::get<std::shared_ptr<FileHandle>>(args[0]);
                if (!fileHandle->isOpen()) {
                    throw RuntimeError("File is not open");
                }
                
                double count = toDouble(args[1]);
                std::vector<double> data;
                data.reserve(static_cast<size_t>(count));
                
                for (size_t i = 0; i < static_cast<size_t>(count); ++i) {
                    double val;
                    if (fread(&val, sizeof(double), 1, fileHandle->handle()) == 1) {
                        data.push_back(val);
                    } else {
                        break;
                    }
                }
                
                if (data.size() == 1) {
                    return data[0];
                }
                
                Matrix result(data.size(), 1);
                for (size_t i = 0; i < data.size(); ++i) {
                    result(i) = data[i];
                }
                return result;
            }
            throw RuntimeError("fread requires a file handle as first argument");
        }
        throw RuntimeError("fread requires at least 2 arguments");
    }

    if (name == "fwrite") {
        // Write binary data to file
        if (args.size() >= 2) {
            if (std::holds_alternative<std::shared_ptr<FileHandle>>(args[0])) {
                auto fileHandle = std::get<std::shared_ptr<FileHandle>>(args[0]);
                if (!fileHandle->isOpen()) {
                    throw RuntimeError("File is not open");
                }
                
                size_t count = 0;
                if (std::holds_alternative<Matrix>(args[1])) {
                    const Matrix& mat = std::get<Matrix>(args[1]);
                    for (size_t i = 0; i < mat.size(); ++i) {
                        double val = mat(i);
                        if (fwrite(&val, sizeof(double), 1, fileHandle->handle()) == 1) {
                            count++;
                        }
                    }
                } else if (std::holds_alternative<double>(args[1])) {
                    double val = std::get<double>(args[1]);
                    if (fwrite(&val, sizeof(double), 1, fileHandle->handle()) == 1) {
                        count++;
                    }
                } else {
                    std::string str = toString(args[1]);
                    count = fwrite(str.c_str(), 1, str.length(), fileHandle->handle());
                }
                
                return static_cast<double>(count);
            }
            throw RuntimeError("fwrite requires a file handle as first argument");
        }
        throw RuntimeError("fwrite requires at least 2 arguments");
    }

    if (name == "save") {
        // Save variables to file: save(filename, var1, var2, ...)
        if (args.size() >= 2) {
            std::string filename = toString(args[0]);
            FILE* file = fopen(filename.c_str(), "w");
            if (!file) {
                throw RuntimeError("Cannot create file: " + filename);
            }
            
            fprintf(file, "CNLAB_MAT v1.0\n");
            
            for (size_t i = 1; i < args.size(); ++i) {
                // Get variable name from caller context - simplified approach
                // In real implementation, we'd need variable names
                std::string varName = "var" + std::to_string(i);
                
                fprintf(file, "VAR %s ", varName.c_str());
                
                if (std::holds_alternative<double>(args[i])) {
                    fprintf(file, "double %g\n", std::get<double>(args[i]));
                } else if (std::holds_alternative<Matrix>(args[i])) {
                    const Matrix& mat = std::get<Matrix>(args[i]);
                    fprintf(file, "matrix %zu %zu", mat.rows(), mat.cols());
                    for (size_t r = 0; r < mat.rows(); ++r) {
                        for (size_t c = 0; c < mat.cols(); ++c) {
                            fprintf(file, " %g", mat(r, c));
                        }
                    }
                    fprintf(file, "\n");
                } else if (std::holds_alternative<std::string>(args[i])) {
                    fprintf(file, "string \"%s\"\n", std::get<std::string>(args[i]).c_str());
                } else {
                    fprintf(file, "unknown\n");
                }
            }
            
            fprintf(file, "END\n");
            fclose(file);
            return 0.0;
        }
        throw RuntimeError("save requires at least 2 arguments");
    }

    if (name == "load") {
        // Load variables from file: load(filename)
        if (args.size() == 1) {
            std::string filename = toString(args[0]);
            FILE* file = fopen(filename.c_str(), "r");
            if (!file) {
                throw RuntimeError("Cannot open file: " + filename);
            }
            
            char line[4096];
            if (!fgets(line, sizeof(line), file) || strncmp(line, "CNLAB_MAT", 9) != 0) {
                fclose(file);
                throw RuntimeError("Invalid file format");
            }
            
            // For simplicity, return a struct with loaded variables
            auto result = std::make_shared<StructArray>();
            
            while (fgets(line, sizeof(line), file)) {
                if (strncmp(line, "END", 3) == 0) break;
                
                char varName[256];
                char type[256];
                if (sscanf(line, "VAR %s %s", varName, type) == 2) {
                    if (strcmp(type, "double") == 0) {
                        double val;
                        sscanf(line, "VAR %*s %*s %lf", &val);
                        result->setField(varName, val);
                    } else if (strcmp(type, "string") == 0) {
                        char strVal[1024];
                        sscanf(line, "VAR %*s %*s \"%[^\"]\"", strVal);
                        result->setField(varName, std::string(strVal));
                    }
                }
            }
            
            fclose(file);
            return result;
        }
        throw RuntimeError("load requires 1 argument");
    }

    if (name == "fscanf") {
        if (args.size() >= 2) {
            if (!std::holds_alternative<std::shared_ptr<FileHandle>>(args[0])) {
                throw RuntimeError("fscanf requires a file handle as first argument");
            }
            auto fileHandle = std::get<std::shared_ptr<FileHandle>>(args[0]);
            if (!fileHandle->isOpen()) {
                throw RuntimeError("File is not open");
            }
            
            std::string format = toString(args[1]);
            std::vector<double> values;
            
            char buffer[4096];
            while (fgets(buffer, sizeof(buffer), fileHandle->handle())) {
                std::string line(buffer);
                std::istringstream iss(line);
                
                size_t i = 0;
                while (i < format.length()) {
                    if (format[i] == '%' && i + 1 < format.length()) {
                        char spec = format[i + 1];
                        double val;
                        std::string strVal;
                        
                        if (spec == 'd' || spec == 'f' || spec == 'e' || spec == 'g') {
                            if (iss >> val) {
                                values.push_back(val);
                            }
                        } else if (spec == 's') {
                            if (iss >> strVal) {
                            }
                        }
                        i += 2;
                    } else {
                        i++;
                    }
                }
            }
            
            if (values.empty()) {
                return Matrix();
            }
            
            Matrix result(values.size(), 1);
            for (size_t i = 0; i < values.size(); ++i) {
                result(i, 0) = values[i];
            }
            return result;
        }
        throw RuntimeError("fscanf requires at least 2 arguments");
    }

    if (name == "sscanf") {
        if (args.size() >= 2) {
            std::string str = toString(args[0]);
            std::string format = toString(args[1]);
            std::vector<double> values;
            
            std::istringstream iss(str);
            
            size_t i = 0;
            while (i < format.length()) {
                if (format[i] == '%' && i + 1 < format.length()) {
                    char spec = format[i + 1];
                    double val;
                    std::string strVal;
                    
                    if (spec == 'd' || spec == 'f' || spec == 'e' || spec == 'g') {
                        if (iss >> val) {
                            values.push_back(val);
                        }
                    } else if (spec == 's') {
                        if (iss >> strVal) {
                        }
                    }
                    i += 2;
                } else {
                    i++;
                }
            }
            
            if (values.empty()) {
                return Matrix();
            }
            
            Matrix result(values.size(), 1);
            for (size_t i = 0; i < values.size(); ++i) {
                result(i, 0) = values[i];
            }
            return result;
        }
        throw RuntimeError("sscanf requires at least 2 arguments");
    }

    if (name == "csvread") {
        if (args.size() >= 1) {
            std::string filename = toString(args[0]);
            size_t startRow = 0;
            size_t startCol = 0;
            if (args.size() >= 2) startRow = static_cast<size_t>(toDouble(args[1]));
            if (args.size() >= 3) startCol = static_cast<size_t>(toDouble(args[2]));
            return FileIOManager::csvread(filename, startRow, startCol);
        }
        throw RuntimeError("csvread requires at least 1 argument");
    }

    if (name == "csvwrite") {
        if (args.size() >= 2) {
            std::string filename = toString(args[0]);
            if (!std::holds_alternative<Matrix>(args[1])) {
                throw RuntimeError("csvwrite requires a matrix as second argument");
            }
            FileIOManager::csvwrite(filename, std::get<Matrix>(args[1]));
            return 0.0;
        }
        throw RuntimeError("csvwrite requires 2 arguments");
    }

    if (name == "dlmread") {
        if (args.size() >= 2) {
            std::string filename = toString(args[0]);
            std::string delimiter = toString(args[1]);
            size_t startRow = 0;
            size_t startCol = 0;
            if (args.size() >= 3) startRow = static_cast<size_t>(toDouble(args[2]));
            if (args.size() >= 4) startCol = static_cast<size_t>(toDouble(args[3]));
            return FileIOManager::dlmread(filename, delimiter, startRow, startCol);
        }
        throw RuntimeError("dlmread requires at least 2 arguments");
    }

    if (name == "dlmwrite") {
        if (args.size() >= 2) {
            std::string filename = toString(args[0]);
            if (!std::holds_alternative<Matrix>(args[1])) {
                throw RuntimeError("dlmwrite requires a matrix as second argument");
            }
            std::string delimiter = ",";
            std::string precision = "%.6f";
            if (args.size() >= 3) delimiter = toString(args[2]);
            if (args.size() >= 4) precision = toString(args[3]);
            FileIOManager::dlmwrite(filename, std::get<Matrix>(args[1]), delimiter, precision);
            return 0.0;
        }
        throw RuntimeError("dlmwrite requires at least 2 arguments");
    }

    if (name == "readmatrix") {
        if (args.size() >= 1) {
            std::string filename = toString(args[0]);
            std::string delimiter = "auto";
            if (args.size() >= 2) delimiter = toString(args[1]);
            return FileIOManager::readmatrix(filename, delimiter);
        }
        throw RuntimeError("readmatrix requires at least 1 argument");
    }

    if (name == "writematrix") {
        if (args.size() >= 2) {
            if (!std::holds_alternative<Matrix>(args[0])) {
                throw RuntimeError("writematrix requires a matrix as first argument");
            }
            std::string filename = toString(args[1]);
            std::string delimiter = ",";
            if (args.size() >= 3) delimiter = toString(args[2]);
            FileIOManager::writematrix(std::get<Matrix>(args[0]), filename, delimiter);
            return 0.0;
        }
        throw RuntimeError("writematrix requires at least 2 arguments");
    }

    if (name == "textscan") {
        if (args.size() >= 2) {
            if (!std::holds_alternative<std::shared_ptr<FileHandle>>(args[0])) {
                throw RuntimeError("textscan requires a file handle as first argument");
            }
            auto fileHandle = std::get<std::shared_ptr<FileHandle>>(args[0]);
            if (!fileHandle->isOpen()) {
                throw RuntimeError("File is not open");
            }
            std::string format = toString(args[1]);
            return FileIOManager::textscan(fileHandle->handle(), format);
        }
        throw RuntimeError("textscan requires at least 2 arguments");
    }

    if (name == "dir") {
        std::string path = ".";
        if (args.size() >= 1) {
            path = toString(args[0]);
        }
        
        auto files = FileIOManager::dir(path);
        auto result = std::make_shared<StructArray>();
        
        for (size_t i = 0; i < files.size(); ++i) {
            auto elem = std::make_shared<StructArray>();
            elem->setField("name", files[i].name);
            elem->setField("folder", files[i].folder);
            elem->setField("date", files[i].date);
            elem->setField("bytes", files[i].bytes);
            elem->setField("isdir", files[i].isdir);
            result->setElement(i + 1, elem);
        }
        
        return result;
    }

    if (name == "mkdir") {
        if (args.size() >= 1) {
            std::string path = toString(args[0]);
            return FileIOManager::mkdir(path) ? 1.0 : 0.0;
        }
        throw RuntimeError("mkdir requires 1 argument");
    }

    if (name == "rmdir") {
        if (args.size() >= 1) {
            std::string path = toString(args[0]);
            return FileIOManager::rmdir(path) ? 1.0 : 0.0;
        }
        throw RuntimeError("rmdir requires 1 argument");
    }

    if (name == "pwd") {
        return FileIOManager::pwd();
    }

    if (name == "cd") {
        if (args.size() >= 1) {
            std::string path = toString(args[0]);
            return FileIOManager::cd(path) ? 1.0 : 0.0;
        }
        throw RuntimeError("cd requires 1 argument");
    }

    if (name == "fullfile") {
        if (args.size() >= 1) {
            std::vector<std::string> parts;
            for (const auto& arg : args) {
                parts.push_back(toString(arg));
            }
            return FileIOManager::fullfile(parts);
        }
        throw RuntimeError("fullfile requires at least 1 argument");
    }

    if (name == "fileparts") {
        if (args.size() >= 1) {
            std::string filename = toString(args[0]);
            auto info = FileIOManager::fileparts(filename);
            
            auto result = std::make_shared<StructArray>();
            result->setField("path", info.path);
            result->setField("name", info.name);
            result->setField("ext", info.ext);
            result->setField("filename", info.filename);
            
            return result;
        }
        throw RuntimeError("fileparts requires 1 argument");
    }

    if (name == "filesep") {
        return FileIOManager::filesep();
    }

    if (name == "exist" || name == "exists") {
        if (args.size() >= 1) {
            std::string path = toString(args[0]);
            return FileIOManager::exists(path) ? 1.0 : 0.0;
        }
        throw RuntimeError("exist requires 1 argument");
    }

    if (name == "isfile") {
        if (args.size() >= 1) {
            std::string path = toString(args[0]);
            return FileIOManager::isfile(path) ? 1.0 : 0.0;
        }
        throw RuntimeError("isfile requires 1 argument");
    }

    if (name == "isfolder" || name == "isdir") {
        if (args.size() >= 1) {
            std::string path = toString(args[0]);
            return FileIOManager::isdir(path) ? 1.0 : 0.0;
        }
        throw RuntimeError("isfolder requires 1 argument");
    }

    if (name == "deletefile" || name == "rmfile") {
        if (args.size() >= 1) {
            std::string filename = toString(args[0]);
            return FileIOManager::deletefile(filename) ? 1.0 : 0.0;
        }
        throw RuntimeError("deletefile requires 1 argument");
    }

    if (name == "copyfile") {
        if (args.size() >= 2) {
            std::string source = toString(args[0]);
            std::string destination = toString(args[1]);
            return FileIOManager::copyfile(source, destination) ? 1.0 : 0.0;
        }
        throw RuntimeError("copyfile requires 2 arguments");
    }

    if (name == "movefile") {
        if (args.size() >= 2) {
            std::string source = toString(args[0]);
            std::string destination = toString(args[1]);
            return FileIOManager::movefile(source, destination) ? 1.0 : 0.0;
        }
        throw RuntimeError("movefile requires 2 arguments");
    }

    throw RuntimeError("Unknown function: " + name);
}

bool Evaluator::isBuiltinFunction(const std::string& name) {
    // List of all builtin functions
    static const std::unordered_set<std::string> builtins = {
        "zeros", "ones", "eye", "rand", "randi", "randn", "randperm",
        "linspace", "logspace", "meshgrid", "magic", "hilb", "pascal", "vander",
        "size", "length", "numel", "isempty", "disp", "print", "printf", "sprintf", "fprintf",
        "struct", "isstruct", "fieldnames", "cell", "iscell", "table", "istable",
        "height", "width", "addrow", "removerow", "getrow",
        "categorical", "iscategorical", "categories",
        "datetime", "isdatetime",
        "years", "days", "hours", "minutes", "seconds", "isduration",
        "ndarray", "isndarray", "ndims", "squeeze", "permute",
        "regexp", "regexpi", "regexprep", "regexptranslate", "regex", "isregex", "regexpattern",
        "string", "isstring", "strjoin", "strsplit", "strlength", "upper", "lower", "contains", "startsWith", "endsWith",
        "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sec", "csc", "cot", "asec", "acsc", "acot",
        "sinh", "cosh", "tanh", "asinh", "acosh", "atanh", "sech", "csch", "coth",
        "exp", "log", "log10", "log2", "expm1", "log1p", "sqrt", "abs", "pow", "hypot", "nextpow2",
        "floor", "ceil", "round", "fix", "sign",
        "sum", "mean", "max", "min", "prod", "std", "var", "cumsum", "cumprod", "diff",
        "factorial", "nchoosek", "gcd", "lcm", "isprime", "primes",
        "gamma", "gammaln", "erf", "erfc",
        "diag", "trace", "tril", "triu", "det", "inv", "transpose", "rank", "norm", "cond",
        "lu", "qr", "svd", "eig", "chol", "solve",
        "sort", "find", "unique", "reshape", "repmat", "flipud", "fliplr", "rot90",
        "horzcat", "vertcat", "blkdiag", "kron",
        "strcat", "strlength", "strcmp", "strcmpi", "strfind", "strrep", "strtrim", "str2num", "num2str",
        "upper", "lower", "split", "startsWith", "endsWith", "contains", "join",
        "extractBefore", "extractAfter", "extractBetween", "replace", "reverse",
        "real", "imag", "abs", "angle", "conj", "complex",
        "mod", "rem",
        "sparse", "speye", "spdiags", "nnz", "issparse", "full",
        "logical", "islogical", "true", "false",
        "char", "ischar", "isletter", "isspace",
        "str2func", "func2str", "isa",
        "Map", "isKey", "keys", "values", "remove",
        "fopen", "fclose", "fprintf", "fscanf", "fread", "fwrite", "save", "load",
        "csvread", "csvwrite", "dlmread", "dlmwrite", "readmatrix", "writematrix", "textscan",
        "dir", "mkdir", "rmdir", "pwd", "cd", "fullfile", "fileparts", "filesep",
        "exist", "exists", "isfile", "isfolder", "isdir", "deletefile", "rmfile", "copyfile", "movefile",
        "clear",
        "figure", "plot", "scatter", "bar", "histogram", "hist",
        "title", "xlabel", "ylabel", "legend", "grid", "hold",
        "xlim", "ylim", "axis", "clf", "subplot", "show", "close"
    };
    return builtins.find(name) != builtins.end();
}

Value Evaluator::add(const Value& left, const Value& right) {
    bool leftIsMatrix = std::holds_alternative<Matrix>(left);
    bool rightIsMatrix = std::holds_alternative<Matrix>(right);
    bool leftIsComplex = std::holds_alternative<Complex>(left);
    bool rightIsComplex = std::holds_alternative<Complex>(right);
    bool leftIsDateTime = std::holds_alternative<std::shared_ptr<DateTime>>(left);
    bool rightIsDateTime = std::holds_alternative<std::shared_ptr<DateTime>>(right);
    bool leftIsDuration = std::holds_alternative<std::shared_ptr<Duration>>(left);
    bool rightIsDuration = std::holds_alternative<std::shared_ptr<Duration>>(right);
    bool leftIsString = std::holds_alternative<std::string>(left);
    bool rightIsString = std::holds_alternative<std::string>(right);
    bool leftIsStringArray = std::holds_alternative<std::shared_ptr<StringArray>>(left);
    bool rightIsStringArray = std::holds_alternative<std::shared_ptr<StringArray>>(right);
    bool leftIsSparse = std::holds_alternative<std::shared_ptr<SparseMatrix>>(left);
    bool rightIsSparse = std::holds_alternative<std::shared_ptr<SparseMatrix>>(right);

    // Handle string concatenation
    if (leftIsString && rightIsString) {
        return std::get<std::string>(left) + std::get<std::string>(right);
    }

    // Handle string + number (convert number to string)
    if (leftIsString && std::holds_alternative<double>(right)) {
        return std::get<std::string>(left) + std::to_string(std::get<double>(right));
    }
    if (std::holds_alternative<double>(left) && rightIsString) {
        return std::to_string(std::get<double>(left)) + std::get<std::string>(right);
    }

    // Handle StringArray concatenation
    if (leftIsStringArray && rightIsStringArray) {
        auto arr1 = std::get<std::shared_ptr<StringArray>>(left);
        auto arr2 = std::get<std::shared_ptr<StringArray>>(right);
        std::vector<std::string> result = arr1->strings();
        for (const auto& s : arr2->strings()) {
            result.push_back(s);
        }
        return std::make_shared<StringArray>(result);
    }

    // Handle DateTime + Duration
    if (leftIsDateTime && rightIsDuration) {
        auto dt = std::get<std::shared_ptr<DateTime>>(left);
        auto dur = std::get<std::shared_ptr<Duration>>(right);
        return std::make_shared<DateTime>(*dt + *dur);
    }

    // Handle Duration + DateTime
    if (leftIsDuration && rightIsDateTime) {
        auto dur = std::get<std::shared_ptr<Duration>>(left);
        auto dt = std::get<std::shared_ptr<DateTime>>(right);
        return std::make_shared<DateTime>(*dt + *dur);
    }

    // Handle Duration + Duration
    if (leftIsDuration && rightIsDuration) {
        auto dur1 = std::get<std::shared_ptr<Duration>>(left);
        auto dur2 = std::get<std::shared_ptr<Duration>>(right);
        return std::make_shared<Duration>(*dur1 + *dur2);
    }

    // Handle sparse + sparse
    if (leftIsSparse && rightIsSparse) {
        return std::get<std::shared_ptr<SparseMatrix>>(left)->add(*std::get<std::shared_ptr<SparseMatrix>>(right));
    }

    // Handle sparse + dense -> dense
    if (leftIsSparse && rightIsMatrix) {
        return std::get<std::shared_ptr<SparseMatrix>>(left)->toDense() + std::get<Matrix>(right);
    }
    if (leftIsMatrix && rightIsSparse) {
        return std::get<Matrix>(left) + std::get<std::shared_ptr<SparseMatrix>>(right)->toDense();
    }

    if (leftIsMatrix && rightIsMatrix) {
        return std::get<Matrix>(left) + std::get<Matrix>(right);
    } else if (leftIsMatrix) {
        return std::get<Matrix>(left) + toDouble(right);
    } else if (rightIsMatrix) {
        return toDouble(left) + std::get<Matrix>(right);
    }

    // Handle complex numbers
    if (leftIsComplex || rightIsComplex) {
        Complex c1 = leftIsComplex ? std::get<Complex>(left) : Complex(toDouble(left), 0.0);
        Complex c2 = rightIsComplex ? std::get<Complex>(right) : Complex(toDouble(right), 0.0);
        return c1 + c2;
    }

    return toDouble(left) + toDouble(right);
}

Value Evaluator::subtract(const Value& left, const Value& right) {
    bool leftIsMatrix = std::holds_alternative<Matrix>(left);
    bool rightIsMatrix = std::holds_alternative<Matrix>(right);
    bool leftIsComplex = std::holds_alternative<Complex>(left);
    bool rightIsComplex = std::holds_alternative<Complex>(right);
    bool leftIsDateTime = std::holds_alternative<std::shared_ptr<DateTime>>(left);
    bool rightIsDateTime = std::holds_alternative<std::shared_ptr<DateTime>>(right);
    bool leftIsDuration = std::holds_alternative<std::shared_ptr<Duration>>(left);
    bool rightIsDuration = std::holds_alternative<std::shared_ptr<Duration>>(right);
    bool leftIsSparse = std::holds_alternative<std::shared_ptr<SparseMatrix>>(left);
    bool rightIsSparse = std::holds_alternative<std::shared_ptr<SparseMatrix>>(right);

    // Handle DateTime - Duration
    if (leftIsDateTime && rightIsDuration) {
        auto dt = std::get<std::shared_ptr<DateTime>>(left);
        auto dur = std::get<std::shared_ptr<Duration>>(right);
        return std::make_shared<DateTime>(*dt - *dur);
    }

    // Handle DateTime - DateTime = Duration
    if (leftIsDateTime && rightIsDateTime) {
        auto dt1 = std::get<std::shared_ptr<DateTime>>(left);
        auto dt2 = std::get<std::shared_ptr<DateTime>>(right);
        return std::make_shared<Duration>(*dt1 - *dt2);
    }

    // Handle Duration - Duration
    if (leftIsDuration && rightIsDuration) {
        auto dur1 = std::get<std::shared_ptr<Duration>>(left);
        auto dur2 = std::get<std::shared_ptr<Duration>>(right);
        return std::make_shared<Duration>(*dur1 - *dur2);
    }

    // Handle sparse - sparse
    if (leftIsSparse && rightIsSparse) {
        return std::get<std::shared_ptr<SparseMatrix>>(left)->subtract(*std::get<std::shared_ptr<SparseMatrix>>(right));
    }

    // Handle sparse - dense -> dense
    if (leftIsSparse && rightIsMatrix) {
        return std::get<std::shared_ptr<SparseMatrix>>(left)->toDense() - std::get<Matrix>(right);
    }
    if (leftIsMatrix && rightIsSparse) {
        return std::get<Matrix>(left) - std::get<std::shared_ptr<SparseMatrix>>(right)->toDense();
    }

    if (leftIsMatrix && rightIsMatrix) {
        return std::get<Matrix>(left) - std::get<Matrix>(right);
    } else if (leftIsMatrix) {
        return std::get<Matrix>(left) - toDouble(right);
    } else if (rightIsMatrix) {
        return toDouble(left) - std::get<Matrix>(right);
    }

    if (leftIsComplex || rightIsComplex) {
        Complex c1 = leftIsComplex ? std::get<Complex>(left) : Complex(toDouble(left), 0.0);
        Complex c2 = rightIsComplex ? std::get<Complex>(right) : Complex(toDouble(right), 0.0);
        return c1 - c2;
    }

    return toDouble(left) - toDouble(right);
}

Value Evaluator::multiply(const Value& left, const Value& right) {
    bool leftIsMatrix = std::holds_alternative<Matrix>(left);
    bool rightIsMatrix = std::holds_alternative<Matrix>(right);
    bool leftIsComplex = std::holds_alternative<Complex>(left);
    bool rightIsComplex = std::holds_alternative<Complex>(right);
    bool leftIsSparse = std::holds_alternative<std::shared_ptr<SparseMatrix>>(left);
    bool rightIsSparse = std::holds_alternative<std::shared_ptr<SparseMatrix>>(right);

    // Handle sparse * scalar
    if (leftIsSparse && !rightIsMatrix && !rightIsSparse) {
        return std::get<std::shared_ptr<SparseMatrix>>(left)->multiply(toDouble(right));
    }

    // Handle scalar * sparse
    if (rightIsSparse && !leftIsMatrix && !leftIsSparse) {
        return std::get<std::shared_ptr<SparseMatrix>>(right)->multiply(toDouble(left));
    }

    // Handle sparse * dense matrix
    if (leftIsSparse && rightIsMatrix) {
        return std::get<std::shared_ptr<SparseMatrix>>(left)->multiply(std::get<Matrix>(right));
    }

    // Handle dense matrix * sparse -> convert to dense first
    if (leftIsMatrix && rightIsSparse) {
        return std::get<Matrix>(left) * std::get<std::shared_ptr<SparseMatrix>>(right)->toDense();
    }

    if (leftIsMatrix && rightIsMatrix) {
        return std::get<Matrix>(left) * std::get<Matrix>(right);
    } else if (leftIsMatrix) {
        return std::get<Matrix>(left) * toDouble(right);
    } else if (rightIsMatrix) {
        return toDouble(left) * std::get<Matrix>(right);
    }

    if (leftIsComplex || rightIsComplex) {
        Complex c1 = leftIsComplex ? std::get<Complex>(left) : Complex(toDouble(left), 0.0);
        Complex c2 = rightIsComplex ? std::get<Complex>(right) : Complex(toDouble(right), 0.0);
        return c1 * c2;
    }

    return toDouble(left) * toDouble(right);
}

Value Evaluator::divide(const Value& left, const Value& right) {
    bool leftIsMatrix = std::holds_alternative<Matrix>(left);
    bool leftIsComplex = std::holds_alternative<Complex>(left);
    bool rightIsComplex = std::holds_alternative<Complex>(right);
    
    // Check for division by zero
    if (!rightIsComplex && toDouble(right) == 0.0) {
        throw RuntimeError("Division by zero");
    }
    
    if (leftIsMatrix) {
        return toMatrix(left) / toDouble(right);
    }
    
    if (leftIsComplex || rightIsComplex) {
        Complex c1 = leftIsComplex ? std::get<Complex>(left) : Complex(toDouble(left), 0.0);
        Complex c2 = rightIsComplex ? std::get<Complex>(right) : Complex(toDouble(right), 0.0);
        return c1 / c2;
    }
    
    return toDouble(left) / toDouble(right);
}

Value Evaluator::modulo(const Value& left, const Value& right) {
    if (std::holds_alternative<Matrix>(left) || std::holds_alternative<Matrix>(right)) {
        if (std::holds_alternative<Matrix>(left) && std::holds_alternative<double>(right)) {
            return toMatrix(left) % toDouble(right);
        } else if (std::holds_alternative<double>(left) && std::holds_alternative<Matrix>(right)) {
            // Not supported yet, fallback to scalar
            throw RuntimeError("Modulo with matrix divisor not supported");
        } else {
            return toMatrix(left) % toMatrix(right);
        }
    }
    return std::fmod(toDouble(left), toDouble(right));
}

Value Evaluator::power(const Value& left, const Value& right) {
    bool leftIsMatrix = std::holds_alternative<Matrix>(left);
    bool leftIsComplex = std::holds_alternative<Complex>(left);
    bool rightIsComplex = std::holds_alternative<Complex>(right);
    
    if (leftIsMatrix) {
        return toMatrix(left).pow(toDouble(right));
    }
    
    if (leftIsComplex || rightIsComplex) {
        Complex c1 = leftIsComplex ? std::get<Complex>(left) : Complex(toDouble(left), 0.0);
        Complex c2 = rightIsComplex ? std::get<Complex>(right) : Complex(toDouble(right), 0.0);
        return std::pow(c1, c2);
    }
    
    return std::pow(toDouble(left), toDouble(right));
}

Value Evaluator::elementWiseAnd(const Value& left, const Value& right) {
    if (std::holds_alternative<Matrix>(left) || std::holds_alternative<Matrix>(right)) {
        Matrix m1 = toMatrix(left);
        Matrix m2 = toMatrix(right);
        if (m1.rows() != m2.rows() || m1.cols() != m2.cols()) {
            throw RuntimeError("Matrix dimensions must agree for element-wise AND");
        }
        Matrix result(m1.rows(), m1.cols());
        for (size_t i = 0; i < m1.rows(); ++i) {
            for (size_t j = 0; j < m1.cols(); ++j) {
                result(i, j) = (m1(i, j) != 0 && m2(i, j) != 0) ? 1.0 : 0.0;
            }
        }
        return result;
    }
    return (toBool(left) && toBool(right)) ? 1.0 : 0.0;
}

Value Evaluator::elementWiseOr(const Value& left, const Value& right) {
    if (std::holds_alternative<Matrix>(left) || std::holds_alternative<Matrix>(right)) {
        Matrix m1 = toMatrix(left);
        Matrix m2 = toMatrix(right);
        if (m1.rows() != m2.rows() || m1.cols() != m2.cols()) {
            throw RuntimeError("Matrix dimensions must agree for element-wise OR");
        }
        Matrix result(m1.rows(), m1.cols());
        for (size_t i = 0; i < m1.rows(); ++i) {
            for (size_t j = 0; j < m1.cols(); ++j) {
                result(i, j) = (m1(i, j) != 0 || m2(i, j) != 0) ? 1.0 : 0.0;
            }
        }
        return result;
    }
    return (toBool(left) || toBool(right)) ? 1.0 : 0.0;
}

Value Evaluator::compare(const Value& left, const Value& right, const std::string& op) {
    bool leftIsMatrix = std::holds_alternative<Matrix>(left);
    bool rightIsMatrix = std::holds_alternative<Matrix>(right);
    
    if (leftIsMatrix && rightIsMatrix) {
        Matrix m1 = std::get<Matrix>(left);
        Matrix m2 = std::get<Matrix>(right);
        
        if (m1.rows() != m2.rows() || m1.cols() != m2.cols()) {
            throw RuntimeError("Matrix dimensions must agree for comparison");
        }
        
        Matrix result(m1.rows(), m1.cols());
        for (size_t i = 0; i < m1.rows(); ++i) {
            for (size_t j = 0; j < m1.cols(); ++j) {
                double a = m1(i, j);
                double b = m2(i, j);
                bool val = false;
                if (op == "==") val = (a == b);
                else if (op == "~=") val = (a != b);
                else if (op == "<") val = (a < b);
                else if (op == ">") val = (a > b);
                else if (op == "<=") val = (a <= b);
                else if (op == ">=") val = (a >= b);
                result(i, j) = val ? 1.0 : 0.0;
            }
        }
        return result;
    } else if (leftIsMatrix) {
        Matrix m = std::get<Matrix>(left);
        double b = toDouble(right);
        Matrix result(m.rows(), m.cols());
        for (size_t i = 0; i < m.rows(); ++i) {
            for (size_t j = 0; j < m.cols(); ++j) {
                double a = m(i, j);
                bool val = false;
                if (op == "==") val = (a == b);
                else if (op == "~=") val = (a != b);
                else if (op == "<") val = (a < b);
                else if (op == ">") val = (a > b);
                else if (op == "<=") val = (a <= b);
                else if (op == ">=") val = (a >= b);
                result(i, j) = val ? 1.0 : 0.0;
            }
        }
        return result;
    } else if (rightIsMatrix) {
        double a = toDouble(left);
        Matrix m = std::get<Matrix>(right);
        Matrix result(m.rows(), m.cols());
        for (size_t i = 0; i < m.rows(); ++i) {
            for (size_t j = 0; j < m.cols(); ++j) {
                double b = m(i, j);
                bool val = false;
                if (op == "==") val = (a == b);
                else if (op == "~=") val = (a != b);
                else if (op == "<") val = (a < b);
                else if (op == ">") val = (a > b);
                else if (op == "<=") val = (a <= b);
                else if (op == ">=") val = (a >= b);
                result(i, j) = val ? 1.0 : 0.0;
            }
        }
        return result;
    }
    
    double a = toDouble(left);
    double b = toDouble(right);
    bool result = false;
    if (op == "==") result = (a == b);
    else if (op == "~=") result = (a != b);
    else if (op == "<") result = (a < b);
    else if (op == ">") result = (a > b);
    else if (op == "<=") result = (a <= b);
    else if (op == ">=") result = (a >= b);
    return result ? 1.0 : 0.0;
}

Value Evaluator::elementWiseMul(const Value& left, const Value& right) {
    bool leftIsMatrix = std::holds_alternative<Matrix>(left);
    bool rightIsMatrix = std::holds_alternative<Matrix>(right);
    
    if (leftIsMatrix && rightIsMatrix) {
        Matrix m1 = std::get<Matrix>(left);
        Matrix m2 = std::get<Matrix>(right);
        
        if (m1.rows() != m2.rows() || m1.cols() != m2.cols()) {
            throw RuntimeError("Matrix dimensions must agree for element-wise multiplication");
        }
        
        Matrix result(m1.rows(), m1.cols());
        for (size_t i = 0; i < m1.rows(); ++i) {
            for (size_t j = 0; j < m1.cols(); ++j) {
                result(i, j) = m1(i, j) * m2(i, j);
            }
        }
        return result;
    } else if (leftIsMatrix) {
        Matrix m = std::get<Matrix>(left);
        double s = toDouble(right);
        return m * s;
    } else if (rightIsMatrix) {
        Matrix m = std::get<Matrix>(right);
        double s = toDouble(left);
        return m * s;
    }
    
    return toDouble(left) * toDouble(right);
}

Value Evaluator::elementWiseDiv(const Value& left, const Value& right) {
    bool leftIsMatrix = std::holds_alternative<Matrix>(left);
    bool rightIsMatrix = std::holds_alternative<Matrix>(right);
    
    if (leftIsMatrix && rightIsMatrix) {
        Matrix m1 = std::get<Matrix>(left);
        Matrix m2 = std::get<Matrix>(right);
        
        if (m1.rows() != m2.rows() || m1.cols() != m2.cols()) {
            throw RuntimeError("Matrix dimensions must agree for element-wise division");
        }
        
        Matrix result(m1.rows(), m1.cols());
        for (size_t i = 0; i < m1.rows(); ++i) {
            for (size_t j = 0; j < m1.cols(); ++j) {
                if (m2(i, j) == 0) {
                    throw RuntimeError("Division by zero in element-wise division");
                }
                result(i, j) = m1(i, j) / m2(i, j);
            }
        }
        return result;
    } else if (leftIsMatrix) {
        Matrix m = std::get<Matrix>(left);
        double s = toDouble(right);
        if (s == 0) {
            throw RuntimeError("Division by zero");
        }
        return m / s;
    } else if (rightIsMatrix) {
        double s = toDouble(left);
        Matrix m = std::get<Matrix>(right);
        Matrix result(m.rows(), m.cols());
        for (size_t i = 0; i < m.rows(); ++i) {
            for (size_t j = 0; j < m.cols(); ++j) {
                if (m(i, j) == 0) {
                    throw RuntimeError("Division by zero in element-wise division");
                }
                result(i, j) = s / m(i, j);
            }
        }
        return result;
    }
    
    double divisor = toDouble(right);
    if (divisor == 0) {
        throw RuntimeError("Division by zero");
    }
    return toDouble(left) / divisor;
}

Value Evaluator::elementWisePow(const Value& left, const Value& right) {
    bool leftIsMatrix = std::holds_alternative<Matrix>(left);
    bool rightIsMatrix = std::holds_alternative<Matrix>(right);
    
    if (leftIsMatrix && rightIsMatrix) {
        Matrix m1 = std::get<Matrix>(left);
        Matrix m2 = std::get<Matrix>(right);
        
        if (m1.rows() != m2.rows() || m1.cols() != m2.cols()) {
            throw RuntimeError("Matrix dimensions must agree for element-wise power");
        }
        
        Matrix result(m1.rows(), m1.cols());
        for (size_t i = 0; i < m1.rows(); ++i) {
            for (size_t j = 0; j < m1.cols(); ++j) {
                result(i, j) = std::pow(m1(i, j), m2(i, j));
            }
        }
        return result;
    } else if (leftIsMatrix) {
        Matrix m = std::get<Matrix>(left);
        double exp = toDouble(right);
        Matrix result(m.rows(), m.cols());
        for (size_t i = 0; i < m.rows(); ++i) {
            for (size_t j = 0; j < m.cols(); ++j) {
                result(i, j) = std::pow(m(i, j), exp);
            }
        }
        return result;
    } else if (rightIsMatrix) {
        double base = toDouble(left);
        Matrix m = std::get<Matrix>(right);
        Matrix result(m.rows(), m.cols());
        for (size_t i = 0; i < m.rows(); ++i) {
            for (size_t j = 0; j < m.cols(); ++j) {
                result(i, j) = std::pow(base, m(i, j));
            }
        }
        return result;
    }
    
    return std::pow(toDouble(left), toDouble(right));
}

Value Evaluator::leftDivide(const Value& left, const Value& right) {
    // A \ b solves Ax = b, equivalent to solve(A, b)
    Matrix A = toMatrix(left);

    // Handle scalar right operand: A \ s is equivalent to A \ [s; s; ...]
    if (std::holds_alternative<double>(right)) {
        double s = std::get<double>(right);
        Matrix b(A.rows(), 1);
        for (size_t i = 0; i < A.rows(); ++i) {
            b(i, 0) = s;
        }
        return solve_linear(A, b);
    }

    Matrix b = toMatrix(right);
    return solve_linear(A, b);
}

}
