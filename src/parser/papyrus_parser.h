#pragma once

#include <memory>

#include "ast/decl/declaration.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "lexer/ilexer.h"
#include "parser/parser.h"

namespace vellum {
using common::Opt;
using common::Shared;
using common::Unique;
using common::Vec;

namespace ast {
class Expression;
class Statement;
}  // namespace ast

class PapyrusParser {
 public:
  PapyrusParser(Unique<ILexer> lexer, Shared<CompilerErrorHandler> errorHandler);

  ParserResult parse();

 private:
  Unique<ILexer> lexer;
  Shared<CompilerErrorHandler> errorHandler;

  Token previous;
  Token current;

  void advance();

  Unique<ast::Declaration> topDeclaration();
  Unique<ast::Declaration> importDeclaration();
  Unique<ast::Declaration> scriptDeclaration();

  bool match(TokenType type);
  bool check(TokenType type) const;

  template <typename... Args>
  void consume(TokenType type, CompilerErrorKind error,
               std::format_string<Args...> fmt, Args&&... args);

  Unique<ast::Declaration> functionDeclarationWithReturnType(VellumType returnType);
  Unique<ast::Declaration> eventDeclaration();
  Unique<ast::Declaration> propertyDeclaration();
  Unique<ast::Declaration> propertyDeclarationWithType(VellumType type);

  Vec<ast::FunctionParameter> parseParameters();
  VellumType parseType();
  void skipUntilEndFunction();
  void skipUntilEndEvent();
  void skipUnitlEndProperty();
  void skipToEndOfStatement();
  void skipBlock();

  void synchronize();
};

template <typename... Args>
inline void PapyrusParser::consume(TokenType type, CompilerErrorKind error,
                                   std::format_string<Args...> fmt,
                                   Args&&... args) {
  if (check(type)) {
    advance();
    return;
  }
  errorHandler->errorAt(current, error, fmt, std::forward<Args>(args)...);
}

}  // namespace vellum
