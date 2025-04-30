#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "lexer/token.h"

namespace vellum {
class CompilerErrorHandler {
 public:
  void errorAt(const Token& token, std::string_view message);

  bool hadError() const { return hadError_; }

  bool isPanicMode() const { return panicMode_; }
  void enablePanicMode() { panicMode_ = true; }
  void disablePanicMode() { panicMode_ = false; }

 private:
  bool hadError_ = false;
  bool panicMode_ = false;

  void printError(const Token& token, std::string_view message);
};
}  // namespace vellum