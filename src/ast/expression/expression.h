#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "vellum/vellum_value.h"

namespace vellum {
namespace ast {

class ExpressionVisitor;

class Expression {
 public:
  virtual ~Expression() = default;

  virtual VellumValue produceValue() const = 0;
  virtual void accept(ExpressionVisitor& visitor) = 0;
  virtual bool equals(const Expression& other) const = 0;
};

bool operator==(const Expression& lhs, const Expression& rhs);
bool operator!=(const Expression& lhs, const Expression& rhs);

class LiteralExpression : public Expression {
 public:
  explicit LiteralExpression(VellumValue value) : value(value) {}

  VellumValue produceValue() const override { return value; }

  void accept(ExpressionVisitor& visitor) override;
  bool equals(const Expression& other) const override;

 private:
  VellumValue value;
};

class CallExpression : public Expression {
 public:
  CallExpression(std::string_view functionName,
                 std::optional<std::string_view> moduleName,
                 std::vector<std::unique_ptr<Expression>> arguments)
      : functionName(functionName),
        moduleName(moduleName),
        arguments(std::move(arguments)) {}

  std::string_view getFunctionName() const { return functionName; }
  std::optional<std::string_view> getModuleName() const { return moduleName; }
  const std::vector<std::unique_ptr<Expression>>& getArguments() const {
    return arguments;
  }

  VellumValue produceValue() const override { return VellumValue(); }

  void accept(ExpressionVisitor& visitor) override;
  bool equals(const Expression& other) const override;

 private:
  std::string_view functionName;
  std::optional<std::string_view> moduleName;
  std::vector<std::unique_ptr<Expression>> arguments;
};

}  // namespace ast
}  // namespace vellum