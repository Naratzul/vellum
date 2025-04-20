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
};

class ExpressionStatement : public Statement {
 public:
  explicit ExpressionStatement(std::unique_ptr<Expression> expression)
      : expression(std::move(expression)) {}

  const std::unique_ptr<Expression>& getExpression() const {
    return expression;
  }

  void accept(StatementVisitor& visitor) override;

 private:
  std::unique_ptr<Expression> expression;
};

}  // namespace ast
}  // namespace vellum