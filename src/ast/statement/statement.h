#pragma once
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "ast/expression/expression.h"
#include "common/types.h"
#include "lexer/token.h"

namespace vellum {

namespace ast {
using common::Opt;
using common::Unique;
using common::Vec;

class StatementVisitor;

class Statement {
 public:
  virtual ~Statement() = default;
  virtual void accept(StatementVisitor& visitor) = 0;
  virtual bool equals(const Statement& other) const = 0;
};

bool operator==(const Statement& lhs, const Statement& rhs);
bool operator!=(const Statement& lhs, const Statement& rhs);

class ExpressionStatement : public Statement {
 public:
  explicit ExpressionStatement(Unique<Expression> expression)
      : expression(std::move(expression)) {}

  const Unique<Expression>& getExpression() const { return expression; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Unique<Expression> expression;
};

class ReturnStatement : public Statement {
 public:
  explicit ReturnStatement(Unique<Expression> expression)
      : expression(std::move(expression)) {}

  const Unique<Expression>& getExpression() const { return expression; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Unique<Expression> expression;
};

class IfStatement : public Statement {
 public:
  IfStatement(Unique<Expression> condition, Vec<Unique<Statement>> then_block,
              Opt<Vec<Unique<Statement>>> else_block = std::nullopt)
      : condition(std::move(condition)),
        then_block(std::move(then_block)),
        else_block(std::move(else_block)) {}

  const Unique<Expression>& getCondition() const { return condition; }
  const Vec<Unique<Statement>>& getThenBlock() const { return then_block; }
  const Opt<Vec<Unique<Statement>>>& getElseBlock() const { return else_block; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Unique<Expression> condition;
  Vec<Unique<Statement>> then_block;
  Opt<Vec<Unique<Statement>>> else_block;
};

class LocalVariableStatement : public Statement {
 public:
  LocalVariableStatement(VellumIdentifier name, Opt<VellumType> type,
                         Unique<Expression> initializer,
                         Token nameLocation = Token(),
                         Opt<Token> typeLocation = std::nullopt)
      : name(name),
        type(type),
        initializer(std::move(initializer)),
        nameLocation(nameLocation),
        typeLocation(typeLocation) {}

  VellumIdentifier getName() const { return name; }

  Opt<VellumType> getType() const { return type; }
  void setType(VellumType type_) { type = type_; }

  const Unique<Expression>& getInitializer() const { return initializer; }
  Token getNameLocation() const { return nameLocation; }
  Opt<Token> getTypeLocation() const { return typeLocation; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  VellumIdentifier name;
  Opt<VellumType> type;
  Unique<Expression> initializer;
  Token nameLocation;
  Opt<Token> typeLocation;
};

class WhileStatement : public Statement {
 public:
  using Body = Vec<Unique<Statement>>;

  WhileStatement(Unique<Expression> condition, Body body)
      : condition(std::move(condition)), body(std::move(body)) {}

  const Unique<Expression>& getCondition() const { return condition; }

  const Body& getBody() const { return body; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Unique<Expression> condition;
  Body body;
};

class BreakStatement : public Statement {
 public:
  explicit BreakStatement(Token breakToken) : breakToken(breakToken) {}

  const Token& getLocation() const { return breakToken; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Token breakToken;
};

class ContinueStatement : public Statement {
 public:
  explicit ContinueStatement(Token continueToken)
      : continueToken(continueToken) {}

  const Token& getLocation() const { return continueToken; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Token continueToken;
};

class ForStatement : public Statement {
 public:
  using Body = Vec<Unique<Statement>>;

  ForStatement(Unique<Expression> variableName, Unique<Expression> array,
               Body body, Token variableNameLocation = {},
               Token arrayLocation = {})
      : variableName(std::move(variableName)),
        array(std::move(array)),
        body(std::move(body)),
        variableNameLocation(variableNameLocation),
        arrayLocation(arrayLocation) {}

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

  const Unique<Expression>& getVariableName() const { return variableName; }

  Unique<Expression>& getVariableName() { return variableName; }

  const Unique<Expression>& getArray() const { return array; }

  const Body& getBody() const { return body; }

  Body& getBody() { return body; }

  Token getVariableNameLocation() const { return variableNameLocation; }

  Token getArrayLocation() const { return arrayLocation; }

  void setCounterMangledName(std::string_view name) {
    counterMangledName = name;
  }

  const std::string& getCounterMangledName() const {
    return counterMangledName;
  }

 private:
  Unique<Expression> variableName;
  Unique<Expression> array;
  Body body;
  Token variableNameLocation;
  Token arrayLocation;
  std::string counterMangledName;
};

}  // namespace ast
}  // namespace vellum