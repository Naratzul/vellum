#include "statement.h"

#include "statement_visitor.h"

namespace vellum {
namespace ast {
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
  return getExpression()->equals(*other.getExpression());
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

  const auto& other_then = other.getThenBlock();
  if (then_block.size() != other_then.size()) {
    return false;
  }
  for (size_t i = 0; i < then_block.size(); ++i) {
    if (!then_block[i]->equals(*other_then[i])) {
      return false;
    }
  }

  if (else_block.has_value() != other.getElseBlock().has_value()) {
    return false;
  }
  if (else_block) {
    const auto& other_else = other.getElseBlock().value();
    if (else_block->size() != other_else.size()) {
      return false;
    }
    for (size_t i = 0; i < else_block->size(); ++i) {
      if (!(*else_block)[i]->equals(*other_else[i])) {
        return false;
      }
    }
  }
  return true;
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
  if (getBody().size() != other.getBody().size()) {
    return false;
  }
  for (std::size_t i = 0; i < getBody().size(); ++i) {
    if (!getBody()[i]->equals(*other.getBody()[i])) {
      return false;
    }
  }
  return true;
}

void BreakStatement::accept(StatementVisitor& visitor) {
  visitor.visitBreakStatement(*this);
}

bool BreakStatement::equals(const Statement& other_) const {
  (void)other_;
  return typeid(other_) == typeid(BreakStatement);
}

void ContinueStatement::accept(StatementVisitor& visitor) {
  visitor.visitContinueStatement(*this);
}

bool ContinueStatement::equals(const Statement& other_) const {
  (void)other_;
  return typeid(other_) == typeid(ContinueStatement);
}
}  // namespace ast
}  // namespace vellum