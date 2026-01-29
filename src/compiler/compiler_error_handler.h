#pragma once

#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "common/types.h"
#include "lexer/token.h"

namespace vellum {
using common::Vec;

enum class DiagnosticMessageType { Warning, Error };

enum class CompilerErrorKind {
  UnexpectedToken,
  ExpectTopLevelDeclaration,
  MultipleScriptsDefinition,
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
  InvalidArgumentType,
  PropertyNotAccessible,
  NotAssignable,
  AssignTypeMismatch,

  LogicalOperationTypeMismatch,
  ArithmeticOperationTypeMismatch,
  UnaryOperatorTypeMismatch,
  UnsupportedBinaryOperator,

  ArrayLengthTypeMismatch,
  ArrayLengthMustBePositive,
  ArrayLengthMaximumExceeded,

  CannotImportLiteralType,
  CannotExtendFromLiteralType
};

enum class CompilerWarningKind {};

struct DiagnosticMessage {
  DiagnosticMessageType type;
  CompilerErrorKind errorKind;
  Token token;
  std::string message;
};

class CompilerErrorHandler {
 public:
  void errorAt(const Token& token, CompilerErrorKind errorKind,
               std::string_view message);

  void errorAt(const Token& token, std::string_view message) {
    errorAt(token, CompilerErrorKind::UnexpectedToken, message);
  }

  template <typename... Args>
  void errorAt(const Token& token, std::format_string<Args...> fmt,
               Args&&... args) {
    errorAt(token, CompilerErrorKind::UnexpectedToken, fmt,
            std::forward<Args>(args)...);
  }

  template <typename... Args>
  void errorAt(const Token& token, CompilerErrorKind error,
               std::format_string<Args...> fmt, Args&&... args) {
    errorAt(token, error, std::format(fmt, std::forward<Args>(args)...));
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