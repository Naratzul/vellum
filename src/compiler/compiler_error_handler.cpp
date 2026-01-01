#include "compiler_error_handler.h"

#include <algorithm>
#include <format>
#include <functional>
#include <iostream>
#include <sstream>

#include "common/os.h"

namespace vellum {
void CompilerErrorHandler::errorAt(const Token& token,
                                   CompilerErrorKind errorKind,
                                   std::string_view message) {
  if (isPanicMode()) {
    return;
  }

  enablePanicMode();
  hadError_ = true;

  errors.push_back(DiagnosticMessage{.type = DiagnosticMessageType::Error,
                                     .errorKind = errorKind,
                                     .token = token,
                                     .message = std::string(message)});
  printError(token, message);
}

bool CompilerErrorHandler::hasError(CompilerErrorKind kind) {
  return std::ranges::any_of(
      errors, [kind](const auto& error) { return error.errorKind == kind; });
}

void CompilerErrorHandler::printError(const Token& token,
                                      std::string_view message) {
  std::ostringstream stream;
  stream << std::format("[line {}, position {}] Error",
                        token.location.start.line + 1,
                        token.location.start.position);

  if (token.type == TokenType::END_OF_FILE) {
    stream << " at end";
  } else if (token.type != TokenType::ERROR) {
    stream << std::format(" at {}", token.lexeme);
  }

  stream << std::format(": {}", message);
  std::cerr << stream.str() << std::endl;

  if (common::isDebuggerPresent()) {
    common::debugBreak();
  }
}

}  // namespace vellum