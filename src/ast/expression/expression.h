#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <vector>

#include "pex/pex_value.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_value.h"

namespace vellum {
namespace ast {

class ExpressionVisitor;
class ExpressionCompiler;

class IdentifierExpression;
class PropertyGetExpression;

class Expression {
 public:
  virtual ~Expression() = default;
  virtual bool equals(const Expression& other) const = 0;

  virtual VellumValue produceValue() const = 0;
  virtual VellumType getType() const { return type; }
  virtual void setType(VellumType type_) { type = type_; }

  virtual void accept(ExpressionVisitor& visitor) { (void)visitor; }
  virtual pex::PexValue compile(ExpressionCompiler& compiler) const = 0;

  virtual bool isLiteralExpression() const { return false; }
  virtual bool isIdentifierExpression() const { return false; }
  virtual bool isPropertyGetExpression() const { return false; }

  IdentifierExpression& asIdentifier();
  PropertyGetExpression& asPropertyGet();

 protected:
  VellumType type{VellumType::unresolved()};
};

bool operator==(const Expression& lhs, const Expression& rhs);
bool operator!=(const Expression& lhs, const Expression& rhs);

class LiteralExpression : public Expression {
 public:
  explicit LiteralExpression(VellumLiteral literal) : literal(literal) {}

  VellumLiteral getLiteral() const { return literal; }
  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override { return VellumValue(literal); }
  VellumType getType() const override {
    return VellumType::literal(literal.getType());
  }

  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  bool isLiteralExpression() const override { return true; }

 private:
  VellumLiteral literal;
};

class IdentifierExpression : public Expression {
 public:
  explicit IdentifierExpression(VellumIdentifier identifier)
      : identifier(identifier) {}

  VellumIdentifier getIdentifier() const { return identifier; }

  void setIdentifierType(VellumValueType type) { identifierType = type; }
  VellumValueType getIdentifierType() const { return identifierType; }

  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override { return VellumValue(identifier); }

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  bool isIdentifierExpression() const override { return true; }

 private:
  VellumIdentifier identifier;
  VellumValueType identifierType{VellumValueType::None};
};

class CallExpression : public Expression {
 public:
  CallExpression(std::unique_ptr<Expression> callee,
                 std::vector<std::unique_ptr<Expression>> arguments)
      : callee(std::move(callee)), arguments(std::move(arguments)) {}

  const std::unique_ptr<Expression>& getCallee() const { return callee; }

  const std::vector<std::unique_ptr<Expression>>& getArguments() const {
    return arguments;
  }

  VellumValue produceValue() const override { return function.value(); }

  void setFunctionCall(VellumFunctionCall function_) { function = function_; }
  std::optional<VellumFunctionCall> getFunctionCall() const { return function; }

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;
  bool equals(const Expression& other) const override;

 private:
  std::unique_ptr<Expression> callee;
  std::vector<std::unique_ptr<Expression>> arguments;
  std::optional<VellumFunctionCall> function;
};

class AssignExpression : public Expression {
 public:
  AssignExpression(VellumIdentifier name, std::unique_ptr<Expression> value)
      : name(name), value(std::move(value)) {}

  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override { return value->produceValue(); }

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  VellumIdentifier getName() const { return name; }
  const std::unique_ptr<Expression>& getValue() const { return value; }

 private:
  VellumIdentifier name;
  std::unique_ptr<Expression> value;
};

class PropertyGetExpression : public Expression {
 public:
  PropertyGetExpression(std::unique_ptr<Expression> object,
                        VellumIdentifier property)
      : object(std::move(object)), property(property) {}

  const std::unique_ptr<Expression>& getObject() const { return object; }
  std::unique_ptr<Expression> getObject() { return std::move(object); }

  VellumIdentifier getProperty() const { return property; }

  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override;

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  bool isPropertyGetExpression() const override { return true; }

 private:
  std::unique_ptr<Expression> object;
  VellumIdentifier property;
};

class PropertySetExpression : public Expression {
 public:
  PropertySetExpression(std::unique_ptr<Expression> object,
                        VellumIdentifier property,
                        std::unique_ptr<Expression> value)
      : object(std::move(object)),
        property(property),
        value(std::move(value)) {}

  const std::unique_ptr<Expression>& getObject() const { return object; }
  VellumIdentifier getProperty() const { return property; }
  const std::unique_ptr<Expression>& getValue() const { return value; }

  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override;

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  bool isPropertyGetExpression() const override { return true; }

 private:
  std::unique_ptr<Expression> object;
  VellumIdentifier property;
  std::unique_ptr<Expression> value;
};

class BinaryExpression : public Expression {
 public:
  enum class Operator {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Equal,
    NotEqual,
    LessThan,
    LessThanEqual,
    GreaterThan,
    GreaterThanEqual,
    And,
    Or
  };

  BinaryExpression(Operator op, std::unique_ptr<Expression> left,
                   std::unique_ptr<Expression> right)
      : op(op), left(std::move(left)), right(std::move(right)) {}

  Operator getOperator() const { return op; }
  const std::unique_ptr<Expression>& getLeft() const { return left; }
  const std::unique_ptr<Expression>& getRight() const { return right; }

  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override;

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

 private:
  Operator op;
  std::unique_ptr<Expression> left;
  std::unique_ptr<Expression> right;
};

class UnaryExpression : public Expression {
 public:
  enum class Operator { Negate, Not };

  UnaryExpression(Operator op, std::unique_ptr<Expression> operand)
      : op(op), operand(std::move(operand)) {}

  Operator getOperator() const { return op; }
  const std::unique_ptr<Expression>& getOperand() const { return operand; }

  bool equals(const Expression& other) const override;
  VellumValue produceValue() const override;
  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

 private:
  Operator op;
  std::unique_ptr<Expression> operand;
};

class GroupingExpression : public Expression {
 public:
  explicit GroupingExpression(std::unique_ptr<Expression> expr)
      : expr(std::move(expr)) {}

  const std::unique_ptr<Expression>& getExpression() const { return expr; }

  bool equals(const Expression& other) const override {
    return expr->equals(other);
  }

  VellumValue produceValue() const override { return expr->produceValue(); }
  VellumType getType() const override { return expr->getType(); }

  void accept(ExpressionVisitor& visitor) override {
    return expr->accept(visitor);
  }

  pex::PexValue compile(ExpressionCompiler& compiler) const override {
    return expr->compile(compiler);
  }

 private:
  std::unique_ptr<Expression> expr;
};

}  // namespace ast
}  // namespace vellum