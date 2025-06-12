#include "compiler_error_handler.h"

#include <algorithm>
#include <format>
#include <functional>
#include <iostream>
#include <sstream>

#include "common/os.h"

namespace vellum {

void CompilerErrorHandler::errorAt(const Token& token,
                                   std::string_view message) {
  printError(token, message);
}

void CompilerErrorHandler::printError(const Token& token,
                                      std::string_view message) {
  if (isPanicMode()) {
    return;
  }

  enablePanicMode();
  hadError_ = true;

  std::ostringstream stream;
  stream << std::format("[line {}] Error", token.line);

  if (token.type == TokenType::END_OF_FILE) {
    stream << " at end";
  } else if (token.type != TokenType::ERROR) {
    stream << std::format(" at {}", token.lexeme);
  }

  stream << std::format(": {}", message);
  std::cerr << stream.str() << std::endl;

  common::debugBreak();
}

}  // namespace vellum