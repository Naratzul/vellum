#include "statement.h"

#include "statement_visitor.h"

namespace vellum {
namespace ast {

namespace {
bool statementsEqual(const Vec<Unique<Statement>>& lhs,
                     const Vec<Unique<Statement>>& rhs) {
  if (lhs.size() != rhs.size()) return false;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (!lhs[i]->equals(*rhs[i])) return false;
  }
  return true;
}

bool matchArmsEqual(const Vec<MatchArm>& lhs, const Vec<MatchArm>& rhs) {
  if (lhs.size() != rhs.size()) return false;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (lhs[i].patterns.size() != rhs[i].patterns.size()) {
      return false;
    }
    for (std::size_t j = 0; j < lhs[i].patterns.size(); ++j) {
      if (!lhs[i].patterns[j]->equals(*rhs[i].patterns[j])) {
        return false;
      }
    }
    if (!lhs[i].body->equals(*rhs[i].body)) {
      return false;
    }
  }
  return true;
}
}  // namespace

void ExpressionStatement::accept(StatementVisitor& visitor) {
  visitor.visitExpressionStatement(*this);
}

bool ExpressionStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const ExpressionStatement&>(other_);
  return getExpression()->equals(*other.getExpression());
}

void ReturnStatement::accept(StatementVisitor& visitor) {
  visitor.visitReturnStatement(*this);
}

bool ReturnStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const ReturnStatement&>(other_);
  const auto& expr = getExpression();
  const auto& otherExpr = other.getExpression();
  if (!expr && !otherExpr) return true;
  if (!expr || !otherExpr) return false;
  return expr->equals(*otherExpr);
}

bool operator==(const Statement& lhs, const Statement& rhs) {
  return typeid(lhs) == typeid(rhs) && lhs.equals(rhs);
}

bool operator!=(const Statement& lhs, const Statement& rhs) {
  return !(lhs == rhs);
}

void IfStatement::accept(StatementVisitor& visitor) {
  visitor.visitIfStatement(*this);
}

bool IfStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const IfStatement&>(other_);
  if (!getCondition()->equals(*other.getCondition())) {
    return false;
  }

  if (!thenBlock->equals(*other.thenBlock)) {
    return false;
  }

  const bool hasElse = static_cast<bool>(elseBlock);
  const bool otherHasElse = static_cast<bool>(other.elseBlock);
  if (hasElse != otherHasElse) {
    return false;
  }
  return !hasElse || elseBlock->equals(*other.elseBlock);
}

void LocalVariableStatement::accept(StatementVisitor& visitor) {
  visitor.visitLocalVariableStatement(*this);
}

bool LocalVariableStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const LocalVariableStatement&>(other_);
  if (getName() != other.getName()) {
    return false;
  }

  if (getType() != other.getType()) {
    return false;
  }

  const bool hasInit = getInitializer() != nullptr;
  if (hasInit != (other.getInitializer() != nullptr)) {
    return false;
  }

  return !hasInit || getInitializer()->equals(*other.getInitializer());
}

void WhileStatement::accept(StatementVisitor& visitor) {
  visitor.visitWhileStatement(*this);
}

bool WhileStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const WhileStatement&>(other_);
  if (!getCondition()->equals(*other.getCondition())) {
    return false;
  }
  return getBody()->equals(*other.getBody());
}

void BreakStatement::accept(StatementVisitor& visitor) {
  visitor.visitBreakStatement(*this);
}

bool BreakStatement::equals(const Statement& other_) const {
  return typeid(other_) == typeid(BreakStatement);
}

void ContinueStatement::accept(StatementVisitor& visitor) {
  visitor.visitContinueStatement(*this);
}

bool ContinueStatement::equals(const Statement& other_) const {
  return typeid(other_) == typeid(ContinueStatement);
}

void ForStatement::accept(StatementVisitor& visitor) {
  visitor.visitForStatement(*this);
}

bool ForStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const ForStatement&>(other_);
  if (!variableName->equals(*other.variableName) ||
      !array->equals(*other.array)) {
    return false;
  }
  return getBody()->equals(*other.getBody());
}

void BlockStatement::accept(StatementVisitor& visitor) {
  visitor.visitBlockStatement(*this);
}

bool BlockStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const BlockStatement&>(other_);
  return statementsEqual(statements, other.statements);
}

void MatchStatement::accept(StatementVisitor& visitor) {
  visitor.visitMatchStatement(*this);
}

bool MatchStatement::equals(const Statement& other_) const {
  auto& other = static_cast<const MatchStatement&>(other_);

  if (!scrutinee->equals(*other.scrutinee)) {
    return false;
  }

  const bool hasElse = elseBody != nullptr;
  if (hasElse != (other.elseBody != nullptr)) {
    return false;
  }

  if (hasElse && !elseBody->equals(*other.elseBody)) {
    return false;
  }

  return matchArmsEqual(arms, other.arms) && matchToken == other.matchToken &&
         promoteType == other.promoteType;
}
}  // namespace ast
}  // namespace vellum