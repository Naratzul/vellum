#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "lexer/token.h"

namespace vellum {
using common::Vec;

enum class DiagnosticMessageType { Warning, Error };

struct DiagnosticMessage {
  DiagnosticMessageType type;
  Token token;
  std::string message;
};

class CompilerErrorHandler {
 public:
  void errorAt(const Token& token, std::string_view message);

  bool hadError() const { return hadError_; }

  void setCanEnterPanicMode(bool value) { canEnterPanicMode = value; }

  bool isPanicMode() const { return canEnterPanicMode && panicMode_; }
  void enablePanicMode() { panicMode_ = true; }
  void disablePanicMode() { panicMode_ = false; }

  const Vec<DiagnosticMessage>& getErrors() const { return errors; }

 private:
  bool hadError_ = false;
  bool panicMode_ = false;
  bool canEnterPanicMode = true;

  Vec<DiagnosticMessage> errors;

  void printError(const Token& token, std::string_view message);
};
}  // namespace vellum