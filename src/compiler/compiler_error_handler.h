#pragma once

#include <format>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "common/types.h"
#include "lexer/token.h"

namespace vellum {
using common::Vec;

enum class DiagnosticMessageType { Warning, Error };

enum class CompilerErrorKind {
  UnexpectedToken,
  ExpectTopLevelDeclaration,
  ExpectExpression,
  ExpectDeclaration,
  ExpectLiteralValue,
  TypeAnnotationMissing,
  ArrayLengthMissing,
  ExpectVarName,
  ExpectTypeName,
  ExpectFunctionName,
  ExpectLeftBrace,
  ExpectRightBrace,
  ExpectLeftParen,
  ExpectRightParen,
  ExpectRightBracket,
  ExpectParamName,
  ExpectColon,
  ExpectReturnType,

  PropertySetAlreadyDefined,
  PropertyGetAlreadyDefined,

  ExpectCastTargetType,
  ArrayTypeMissing,
  ArraySemicolonMissing,
  ArrayLengthAsIntExpected,
  ArrayLengthMustBeProvided,

  FilenameMismatch,
  ReturnTypeMismatch,
  VariableTypeMismatch,
  UndefinedIdentifier,
  UndefinedFunction,
  UndefinedProperty,
  UndefinedVariable,
  FunctionArgumentsCountMismatch,
  FunctionArgumentTypeMismatch,
  PropertyNotAccessible,
  NotAssignable,
  AssignTypeMismatch,

  LogicalOperationTypeMismatch,
  ArithmeticOperationTypeMismatch,
  UnaryOperatorTypeMismatch,
  UnsupportedBinaryOperator,

  ArrayLengthTypeMismatch,
  ArrayLengthMustBePositive,
  ArrayLengthMaximumExceeded
};

enum class CompilerWarningKind {};

struct DiagnosticMessage {
  DiagnosticMessageType type;
  CompilerErrorKind errorKind;
  Token token;
  std::string message;
};

class CompilerErrorHandler {
 private:
  template <typename... Args>
  static std::string formatMessage(std::string_view fmt, const Args&... args) {
    if constexpr (sizeof...(Args) == 0) {
      return std::string(fmt);
    } else {
      // Create format_args and use vformat with string_view
      auto format_args = std::make_format_args(args...);
      return std::vformat(fmt, format_args);
    }
  }

 public:
  template <typename... Args>
  void errorAt(const Token& token, CompilerErrorKind errorKind,
               std::string_view fmt, const Args&... args) {
    if (isPanicMode()) {
      return;
    }

    enablePanicMode();
    hadError_ = true;

    std::string message = formatMessage(fmt, args...);

    errors.push_back(DiagnosticMessage{.type = DiagnosticMessageType::Error,
                                       .token = token,
                                       .errorKind = errorKind,
                                       .message = message});
    printError(token, message);
  }

  template <typename... Args>
  void errorAt(const Token& token, std::string_view fmt, const Args&... args) {
    errorAt(token, CompilerErrorKind::UnexpectedToken, fmt, args...);
  }

  bool hadError() const { return hadError_; }
  bool hasError(CompilerErrorKind kind);

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