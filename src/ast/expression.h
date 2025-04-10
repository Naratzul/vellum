#pragma once

#include <vector>

namespace vellum {
namespace ast {

class Expression {
 public:
  virtual std::vector<uint8_t> compile() = 0;
};

class Statement {};
}  // namespace ast
}  // namespace vellum