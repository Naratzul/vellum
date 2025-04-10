#include "compiler_error_handler.h"

#include <algorithm>
#include <format>
#include <functional>
#include <iostream>
#include <sstream>

namespace vellum {

void CompilerErrorHandler::errorAt(const Token& token,
                                   std::string_view message) {
  errors.emplace_back(token, std::string(message));
}

void CompilerErrorHandler::printErrors() const {
  std::ranges::for_each(
      errors, std::bind_front(&CompilerErrorHandler::printError, this));
}

void CompilerErrorHandler::printError(const ErrorMessage& error) const {
  std::ostringstream stream;
  stream << std::format("[line {}] Error", error.token.line);

  if (error.token.type == TokenType::ERROR) {
    return;
  }

  if (error.token.type == TokenType::END_OF_FILE) {
    stream << " at end";
  } else {
    stream << std::format(" at {}", error.token.lexeme);
  }

  stream << std::format(": {}", error.message);

  std::cerr << stream.str() << std::endl;
  ;
}

}  // namespace vellum