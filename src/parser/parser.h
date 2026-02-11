#pragma once

#include <memory>

#include "ast/decl/declaration.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "lexer/ilexer.h"

namespace vellum {
using common::Opt;
using common::Shared;
using common::Unique;
using common::Vec;

namespace ast {
class Expression;
class Statement;
}  // namespace ast

struct ParserResult {
  Vec<Unique<ast::Declaration>> declarations;
};

enum class FunctionType { Function, Event, Getter, Setter };

class ParseException : public std::runtime_error {
 public:
  ParseException(Token token, const std::string& message)
      : std::runtime_error(message), token(token) {}

  Token getToken() const { return token; }
  std::string getMessage() const { return what(); }

 private:
  Token token;
};

class Parser {
 public:
  Parser(Unique<ILexer> lexer, Shared<CompilerErrorHandler> errorHandler);

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
  Unique<ast::Declaration> stateDeclaration(bool isAuto);

  Unique<ast::Declaration> scriptMemberDeclaration();

  bool match(TokenType type);
  bool match(std::initializer_list<TokenType> types);

  bool check(TokenType type) const;

  template <typename... Args>
  void consume(TokenType type, CompilerErrorKind error,
               std::format_string<Args...> fmt, Args&&... args);

  Unique<ast::Declaration> variableDeclaration();
  Unique<ast::Declaration> functionDeclaration(FunctionType functionType,
                                               bool isStatic);
  Unique<ast::Declaration> propertyDeclaration();

  ast::FunctionBody functionBody(FunctionType type);

  Unique<ast::Statement> statement();
  Unique<ast::Statement> expressionStatement();
  Unique<ast::Statement> ifStatement();
  Unique<ast::Statement> returnStatement();
  Unique<ast::Statement> varStatement();
  Unique<ast::Statement> whileStatement();

  Unique<ast::Expression> expression();
  Unique<ast::Expression> assignExpression();
  Unique<ast::Expression> logicalOrExpression();
  Unique<ast::Expression> logicalAndExpression();
  Unique<ast::Expression> equalityExpression();
  Unique<ast::Expression> compareExpression();
  Unique<ast::Expression> termExpression();
  Unique<ast::Expression> factorExpression();
  Unique<ast::Expression> unaryExpression();
  Unique<ast::Expression> castExpression();
  Unique<ast::Expression> callOrGetExpression();
  Unique<ast::Expression> callExpression(Unique<ast::Expression> callee,
                                         const Token& location);
  Unique<ast::Expression> primaryExpression();
  Unique<ast::Expression> arrayExpression();

  void synchronizeTopDeclaration();
  void synchronizeDeclaration();
  void synchronizeStatement();
  void synchronizeToRightParen();
};
template <typename... Args>
inline void Parser::consume(TokenType type, CompilerErrorKind error,
                            std::format_string<Args...> fmt, Args&&... args) {
  if (check(type)) {
    advance();
    return;
  }
  throw ParseException(current, std::format(fmt, std::forward<Args>(args)...));
}
}  // namespace vellum