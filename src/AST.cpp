#include "AST.hpp"

namespace cnlab {

void NumberLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ComplexLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void StringLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CharArrayLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BooleanLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Identifier::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void MatrixLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void RangeExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BinaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void UnaryExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CallExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ColonExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void EndExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IndexExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AnonymousFunction::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FunctionHandle::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CellLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void StringArrayLiteral::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CellIndexExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FieldAccessExpression::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void AssignmentStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void MultiAssignmentStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IndexAssignmentStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void CellIndexAssignmentStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FieldAssignmentStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ExpressionStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void IfStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void WhileStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ForStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ReturnStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void BreakStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void ContinueStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void FunctionDeclaration::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void SwitchStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void TryCatchStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void GlobalStatement::accept(ASTVisitor& visitor) { visitor.visit(*this); }
void Program::accept(ASTVisitor& visitor) { visitor.visit(*this); }

}
