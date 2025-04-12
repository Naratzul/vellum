#pragma once

#include <vector>

#include "vellum/vellum_value.h"

namespace vellum {
namespace ast {

class Expression {
 public:
  virtual ~Expression() = default;

  virtual VellumValue produceValue() const = 0;
};

class LiteralExpression : public Expression {
 public:
  explicit LiteralExpression(VellumValue value) : value(value) {}

  VellumValue produceValue() const override { return value; }

 private:
  VellumValue value;
};

}  // namespace ast
}  // namespace vellum