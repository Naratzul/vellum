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
  using Body = Vec<Unique<ast::Statement>>;

  WhileStatement(Unique<ast::Expression> condition, Body body)
      : condition(std::move(condition)), body(std::move(body)) {}

  const Unique<ast::Expression>& getCondition() const { return condition; }

  const Body& getBody() const { return body; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  Unique<ast::Expression> condition;
  Body body;
};

}  // namespace ast
}  // namespace vellum