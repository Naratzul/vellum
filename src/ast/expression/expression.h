#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <vector>

#include "common/types.h"
#include "lexer/token.h"
#include "pex/pex_value.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_value.h"

namespace vellum {
namespace ast {
using common::Opt;
using common::Unique;
using common::Vec;

class ExpressionVisitor;
class ExpressionCompiler;

class LiteralExpression;
class IdentifierExpression;
class PropertyGetExpression;

class Expression {
 public:
  explicit Expression(Token location) : location(location) {}
  virtual ~Expression() = default;

  virtual bool equals(const Expression& other) const = 0;

  virtual VellumType getType() const { return type; }
  virtual void setType(VellumType type_) { type = type_; }

  Token getLocation() const { return location; }

  virtual void accept(ExpressionVisitor& visitor) { (void)visitor; }
  virtual pex::PexValue compile(ExpressionCompiler& compiler) const = 0;

  virtual bool isLiteralExpression() const { return false; }
  virtual bool isIdentifierExpression() const { return false; }
  virtual bool isPropertyGetExpression() const { return false; }

  LiteralExpression& asLiteral();
  IdentifierExpression& asIdentifier();
  PropertyGetExpression& asPropertyGet();

 protected:
  VellumType type{VellumType::unresolved()};
  Token location;
};

bool operator==(const Expression& lhs, const Expression& rhs);
bool operator!=(const Expression& lhs, const Expression& rhs);

class LiteralExpression : public Expression {
 public:
  explicit LiteralExpression(VellumLiteral literal, Token location = Token{})
      : Expression(location), literal(literal) {}

  VellumLiteral getLiteral() const { return literal; }
  bool equals(const Expression& other) const override;

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
  IdentifierExpression(VellumIdentifier identifier, Token location = Token{})
      : Expression(location), identifier(identifier) {}

  VellumIdentifier getIdentifier() const { return identifier; }

  void setIdentifierType(VellumValueType type) { identifierType = type; }
  VellumValueType getIdentifierType() const { return identifierType; }

  bool equals(const Expression& other) const override;

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  bool isIdentifierExpression() const override { return true; }

 private:
  VellumIdentifier identifier;
  VellumValueType identifierType{VellumValueType::Identifier};
};

class CallExpression : public Expression {
 public:
  CallExpression(Unique<Expression> callee, Vec<Unique<Expression>> arguments,
                 Token location = Token{})
      : Expression(location),
        callee(std::move(callee)),
        arguments(std::move(arguments)) {}

  const Unique<Expression>& getCallee() const { return callee; }

  const Vec<Unique<Expression>>& getArguments() const { return arguments; }

  void setFunctionCall(VellumFunctionCall function_) { function = function_; }
  Opt<VellumFunctionCall> getFunctionCall() const { return function; }

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;
  bool equals(const Expression& other) const override;

 private:
  Unique<Expression> callee;
  Vec<Unique<Expression>> arguments;
  Opt<VellumFunctionCall> function;
};

class AssignExpression : public Expression {
 public:
  AssignExpression(VellumIdentifier name, Unique<Expression> value,
                   Token location = Token{})
      : Expression(location), name(name), value(std::move(value)) {}

  bool equals(const Expression& other) const override;

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  VellumIdentifier getName() const { return name; }
  const Unique<Expression>& getValue() const { return value; }

 private:
  VellumIdentifier name;
  Unique<Expression> value;
};

class PropertyGetExpression : public Expression {
 public:
  PropertyGetExpression(Unique<Expression> object, VellumIdentifier property,
                        Token location = Token{})
      : Expression(location), object(std::move(object)), property(property) {}

  const Unique<Expression>& getObject() const { return object; }
  Unique<Expression> releaseObject() { return std::move(object); }

  VellumIdentifier getProperty() const { return property; }

  void setIdentifierType(VellumValueType type) { identifierType = type; }
  VellumValueType getIdentifierType() const { return identifierType; }

  bool equals(const Expression& other) const override;

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  bool isPropertyGetExpression() const override { return true; }

 private:
  Unique<Expression> object;
  VellumIdentifier property;
  VellumValueType identifierType{VellumValueType::Identifier};
};

class PropertySetExpression : public Expression {
 public:
  PropertySetExpression(Unique<Expression> object, VellumIdentifier property,
                        Unique<Expression> value, Token location = Token{})
      : Expression(location),
        object(std::move(object)),
        property(property),
        value(std::move(value)) {}

  const Unique<Expression>& getObject() const { return object; }
  VellumIdentifier getProperty() const { return property; }
  const Unique<Expression>& getValue() const { return value; }

  bool equals(const Expression& other) const override;

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  bool isPropertyGetExpression() const override { return true; }

 private:
  Unique<Expression> object;
  VellumIdentifier property;
  Unique<Expression> value;
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

  BinaryExpression(Operator op, Unique<Expression> left, Unique<Expression> right,
                   Token location = Token{})
      : Expression(location),
        op(op),
        left(std::move(left)),
        right(std::move(right)) {}

  Operator getOperator() const { return op; }
  const Unique<Expression>& getLeft() const { return left; }
  const Unique<Expression>& getRight() const { return right; }

  bool equals(const Expression& other) const override;

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

 private:
  Operator op;
  Unique<Expression> left;
  Unique<Expression> right;
};

class UnaryExpression : public Expression {
 public:
  enum class Operator { Negate, Not };

  UnaryExpression(Operator op, Unique<Expression> operand,
                  Token location = Token{})
      : Expression(location), op(op), operand(std::move(operand)) {}

  Operator getOperator() const { return op; }
  const Unique<Expression>& getOperand() const { return operand; }

  bool equals(const Expression& other) const override;
  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

 private:
  Operator op;
  Unique<Expression> operand;
};

class GroupingExpression : public Expression {
 public:
  GroupingExpression(Unique<Expression> expr, Token location = Token{})
      : Expression(location), expr(std::move(expr)) {}

  const Unique<Expression>& getExpression() const { return expr; }

  bool equals(const Expression& other) const override {
    return expr->equals(other);
  }

  VellumType getType() const override { return expr->getType(); }

  void accept(ExpressionVisitor& visitor) override {
    return expr->accept(visitor);
  }

  pex::PexValue compile(ExpressionCompiler& compiler) const override {
    return expr->compile(compiler);
  }

 private:
  Unique<Expression> expr;
};

class CastExpression : public Expression {
 public:
  CastExpression(Unique<Expression> expr, VellumType targetType,
                 Token location = Token{})
      : Expression(location), expr(std::move(expr)), targetType(targetType) {}

  const Unique<Expression>& getExpression() const { return expr; }

  VellumType getTargetType() const { return targetType; }
  void setTargetType(VellumType type) { targetType = type; }

  VellumType getType() const override { return targetType; }

  bool equals(const Expression& other) const override;

  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

 private:
  Unique<Expression> expr;
  VellumType targetType;
};

class NewArrayExpression : public Expression {
 public:
  NewArrayExpression(Opt<VellumType> subtype, VellumLiteral length, Token location)
      : Expression(location), subtype(subtype), length(length) {}

  bool equals(const Expression& other) const override;
  void accept(ExpressionVisitor& visitor) override;
  pex::PexValue compile(ExpressionCompiler& compiler) const override;

  const Opt<VellumType>& getSubtype() const { return subtype; }
  void setSubtype(VellumType type) { subtype = type; }

  const VellumLiteral& getLength() const { return length; }

 private:
  Opt<VellumType> subtype;
  VellumLiteral length;
};
}  // namespace ast
}  // namespace vellum