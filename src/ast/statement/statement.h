#pragma once
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "ast/expression/expression.h"

namespace vellum {

namespace ast {

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
  explicit ExpressionStatement(std::unique_ptr<Expression> expression)
      : expression(std::move(expression)) {}

  const std::unique_ptr<Expression>& getExpression() const {
    return expression;
  }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  std::unique_ptr<Expression> expression;
};

class ReturnStatement : public Statement {
 public:
  explicit ReturnStatement(std::unique_ptr<Expression> expression)
      : expression(std::move(expression)) {}

  const std::unique_ptr<Expression>& getExpression() const {
    return expression;
  }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  std::unique_ptr<Expression> expression;
};

class IfStatement : public Statement {
 public:
  IfStatement(std::unique_ptr<Expression> condition,
              std::vector<std::unique_ptr<Statement>> then_block,
              std::optional<std::vector<std::unique_ptr<Statement>>>
                  else_block = std::nullopt)
      : condition(std::move(condition)),
        then_block(std::move(then_block)),
        else_block(std::move(else_block)) {}

  const std::unique_ptr<Expression>& getCondition() const { return condition; }
  const std::vector<std::unique_ptr<Statement>>& getThenBlock() const {
    return then_block;
  }
  const std::optional<std::vector<std::unique_ptr<Statement>>>& getElseBlock()
      const {
    return else_block;
  }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  std::unique_ptr<Expression> condition;
  std::vector<std::unique_ptr<Statement>> then_block;
  std::optional<std::vector<std::unique_ptr<Statement>>> else_block;
};

class LocalVariableStatement : public Statement {
 public:
  LocalVariableStatement(VellumIdentifier name, std::optional<VellumType> type,
                         std::unique_ptr<Expression> initializer)
      : name(name), type(type), initializer(std::move(initializer)) {}

  VellumIdentifier getName() const { return name; }

  std::optional<VellumType> getType() const { return type; }
  void setType(VellumType type_) { type = type_; }

  const std::unique_ptr<Expression>& getInitializer() const {
    return initializer;
  }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  VellumIdentifier name;
  std::optional<VellumType> type;
  std::unique_ptr<Expression> initializer;
};

class WhileStatement : public Statement {
 public:
  using Body = std::vector<std::unique_ptr<ast::Statement>>;

  WhileStatement(std::unique_ptr<ast::Expression> condition, Body body)
      : condition(std::move(condition)), body(std::move(body)) {}

  const std::unique_ptr<ast::Expression>& getCondition() const {
    return condition;
  }

  const Body& getBody() const { return body; }

  void accept(StatementVisitor& visitor) override;
  bool equals(const Statement& other) const override;

 private:
  std::unique_ptr<ast::Expression> condition;
  Body body;
};

}  // namespace ast
}  // namespace vellum