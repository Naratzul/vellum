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
  virtual VellumType getType() const = 0;

  virtual void accept(ExpressionVisitor& visitor) {}
  virtual pex::PexValue compile(ExpressionCompiler& compiler) const = 0;

  virtual bool isLiteralExpression() const { return false; }
  virtual bool isIdentifierExpression() const { return false; }
  virtual bool isPropertyGetExpression() const { return false; }

  IdentifierExpression& asIdentifier();
  PropertyGetExpression& asPropertyGet();
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
      : identifier(identifier), type(VellumType::unresolved()) {}

  VellumIdentifier getIdentifier() const { return identifier; }
  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override { return VellumValue(identifier); }
  VellumType getType() const override { return type; }

  void setType(VellumType type_) { type = type_; }

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  bool isIdentifierExpression() const override { return true; }

 private:
  VellumIdentifier identifier;
  VellumType type;
};

class CallExpression : public Expression {
 public:
  CallExpression(std::unique_ptr<Expression> callee,
                 std::vector<std::unique_ptr<Expression>> arguments)
      : callee(std::move(callee)),
        arguments(std::move(arguments)),
        type(VellumType::none()) {}

  const std::unique_ptr<Expression>& getCallee() const { return callee; }

  const std::vector<std::unique_ptr<Expression>>& getArguments() const {
    return arguments;
  }

  VellumType getType() const override { return type; }
  void setType(VellumType type_) { type = type_; }

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
  VellumType type;
};

class AssignExpression : public Expression {
 public:
  AssignExpression(VellumIdentifier name, std::unique_ptr<Expression> value)
      : name(name), value(std::move(value)), type(VellumType::none()) {}

  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override { return value->produceValue(); }
  VellumType getType() const override { return type; }
  void setType(VellumType type_) { type = type_; }

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  VellumIdentifier getName() const { return name; }
  const std::unique_ptr<Expression>& getValue() const { return value; }

 private:
  VellumIdentifier name;
  std::unique_ptr<Expression> value;
  VellumType type;
};

class PropertyGetExpression : public Expression {
 public:
  PropertyGetExpression(std::unique_ptr<Expression> object,
                        VellumIdentifier property)
      : object(std::move(object)),
        property(property),
        propertyType(VellumType::unresolved()) {}

  const std::unique_ptr<Expression>& getObject() const { return object; }
  std::unique_ptr<Expression> getObject() { return std::move(object); }

  VellumIdentifier getProperty() const { return property; }

  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override;

  VellumType getType() const override { return propertyType; }

  void setType(VellumType typeName) { propertyType = typeName; }

  virtual void accept(ExpressionVisitor& visitor);
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  virtual bool isPropertyGetExpression() const { return true; }

 private:
  std::unique_ptr<Expression> object;
  VellumIdentifier property;
  VellumType propertyType;
};

class PropertySetExpression : public Expression {
 public:
  PropertySetExpression(std::unique_ptr<Expression> object,
                        VellumIdentifier property,
                        std::unique_ptr<Expression> value)
      : object(std::move(object)),
        property(property),
        value(std::move(value)),
        propertyType(VellumType::unresolved()) {}

  const std::unique_ptr<Expression>& getObject() const { return object; }
  VellumIdentifier getProperty() const { return property; }
  const std::unique_ptr<Expression>& getValue() const { return value; }

  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override;

  VellumType getType() const override { return propertyType; }

  void setType(VellumType typeName) { propertyType = typeName; }

  virtual void accept(ExpressionVisitor& visitor);
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  virtual bool isPropertyGetExpression() const { return true; }

 private:
  std::unique_ptr<Expression> object;
  VellumIdentifier property;
  std::unique_ptr<Expression> value;
  VellumType propertyType;
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
      : op(op),
        left(std::move(left)),
        right(std::move(right)),
        type(VellumType::unresolved()) {}

  Operator getOperator() const { return op; }
  const std::unique_ptr<Expression>& getLeft() const { return left; }
  const std::unique_ptr<Expression>& getRight() const { return right; }

  bool equals(const Expression& other) const override;

  VellumValue produceValue() const override;
  VellumType getType() const override { return type; }
  void setType(VellumType type_) { type = type_; }

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

 private:
  Operator op;
  std::unique_ptr<Expression> left;
  std::unique_ptr<Expression> right;
  VellumType type;
};

}  // namespace ast
}  // namespace vellum