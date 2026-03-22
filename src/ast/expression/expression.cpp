#include "expression.h"

#include "expression_visitor.h"

namespace vellum {
namespace ast {

bool LiteralExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const LiteralExpression&>(other_);
  return literal == other.literal;
}

pex::PexValue LiteralExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool IdentifierExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const IdentifierExpression&>(other_);
  return identifier == other.identifier;
}

void IdentifierExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitIdentifierExpression(*this);
}

pex::PexValue IdentifierExpression::compile(
    ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

void CallExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitCallExpression(*this);
}

pex::PexValue CallExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool CallExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const CallExpression&>(other_);
  if (!getCallee()->equals(*other.getCallee())) {
    return false;
  }

  if (getArguments().size() != other.getArguments().size()) {
    return false;
  }

  for (size_t i = 0; i < getArguments().size(); i++) {
    if (!getArguments()[i]->equals(*other.getArguments()[i])) {
      return false;
    }
  }

  return true;
}

bool PropertyGetExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const PropertyGetExpression&>(other_);
  return getObject()->equals(*other.getObject()) &&
         getProperty() == other.getProperty();
}

void PropertyGetExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitPropertyGetExpression(*this);
}

pex::PexValue PropertyGetExpression::compile(
    ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool operator==(const Expression& lhs, const Expression& rhs) {
  return typeid(lhs) == typeid(rhs) && lhs.equals(rhs);
}

bool operator!=(const Expression& lhs, const Expression& rhs) {
  return !(lhs == rhs);
}

bool AssignExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const AssignExpression&>(other_);
  return getName() == other.getName() && getOperator() == other.getOperator() &&
         getType() == other.getType() && getValue()->equals(*other.getValue());
}

void AssignExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitAssignExpression(*this);
}

pex::PexValue AssignExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

LiteralExpression& Expression::asLiteral() {
  return static_cast<LiteralExpression&>(*this);
}

UnaryExpression& Expression::asUnary() {
  return static_cast<UnaryExpression&>(*this);
}

IdentifierExpression& Expression::asIdentifier() {
  return static_cast<IdentifierExpression&>(*this);
}

ArrayIndexExpression& Expression::asArrayIndex() {
  return static_cast<ArrayIndexExpression&>(*this);
}

PropertyGetExpression& Expression::asPropertyGet() {
  return static_cast<PropertyGetExpression&>(*this);
}

SelfExpression& Expression::asSelfExpression() {
  return static_cast<SelfExpression&>(*this);
}

SuperExpression& Expression::asSuperExpression() {
  return static_cast<SuperExpression&>(*this);
}

bool SelfExpression::equals(const Expression& other_) const {
  return other_.isSelfExpression();
}

void SelfExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitSelfExpression(*this);
}

pex::PexValue SelfExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool SuperExpression::equals(const Expression& other_) const {
  return other_.isSuperExpression();
}

void SuperExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitSuperExpression(*this);
}

pex::PexValue SuperExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool PropertySetExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const PropertySetExpression&>(other_);
  return getObject()->equals(*other.getObject()) &&
         getProperty() == other.getProperty() &&
         getOperator() == other.getOperator() &&
         getValue()->equals(*other.getValue());
}

void PropertySetExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitPropertySetExpression(*this);
}

pex::PexValue PropertySetExpression::compile(
    ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool BinaryExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const BinaryExpression&>(other_);
  return op == other.op && left->equals(*other.left) &&
         right->equals(*other.right);
}

void BinaryExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitBinaryExpression(*this);
}

pex::PexValue BinaryExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool UnaryExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const UnaryExpression&>(other_);
  return op == other.op && operand->equals(*other.operand);
}

void UnaryExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitUnaryExpression(*this);
}

pex::PexValue UnaryExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool CastExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const CastExpression&>(other_);
  return getTargetType() == other.getTargetType() &&
         getExpression()->equals(*other.getExpression());
}

void CastExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitCastExpression(*this);
}

pex::PexValue CastExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool NewArrayExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const NewArrayExpression&>(other_);
  return subtype == other.subtype && length == other.length;
}

void NewArrayExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitNewArrayExpression(*this);
}

pex::PexValue NewArrayExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool ArrayIndexExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const ArrayIndexExpression&>(other_);
  return getArray()->equals(*other.getArray()) &&
         getIndex()->equals(*other.getIndex());
}

void ArrayIndexExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitArrayIndexExpression(*this);
}

pex::PexValue ArrayIndexExpression::compile(
    ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool ArrayIndexSetExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const ArrayIndexSetExpression&>(other_);
  return getArray()->equals(*other.getArray()) &&
         getIndex()->equals(*other.getIndex()) &&
         getOperator() == other.getOperator() &&
         getValue()->equals(*other.getValue());
}

void ArrayIndexSetExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitArrayIndexSetExpression(*this);
}

pex::PexValue ArrayIndexSetExpression::compile(
    ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

bool TernaryExpression::equals(const Expression& other_) const {
  auto& other = static_cast<const TernaryExpression&>(other_);
  return condition->equals(*other.condition) && left->equals(*other.left) &&
         right->equals(*other.right);
}

void TernaryExpression::accept(ExpressionVisitor& visitor) {
  visitor.visitTernaryExpression(*this);
}

pex::PexValue TernaryExpression::compile(ExpressionCompiler& compiler) const {
  return compiler.compile(*this);
}

}  // namespace ast
}  // namespace vellum