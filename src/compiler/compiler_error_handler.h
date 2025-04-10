#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "lexer/token.h"

namespace vellum {
class CompilerErrorHandler {
 public:
  void errorAt(const Token& token, std::string_view message);
  void printErrors() const;

  bool hadError() const { return !errors.empty(); }

 private:
  struct ErrorMessage {
    Token token;
    std::string message;
  };

  std::vector<ErrorMessage> errors;

  void printError(const ErrorMessage& error) const;
};
}  // namespace vellum