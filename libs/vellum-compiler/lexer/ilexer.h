#pragma once

#include "token.h"

namespace vellum {

class ILexer {
 public:
  virtual Token scanToken() = 0;
  virtual ~ILexer() = default;
};

}  // namespace vellum