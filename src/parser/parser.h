#pragma once

#include <format>
#include <memory>
#include <vector>

#include "ast/decl/declaration.h"
#include "compiler/compiler_error_handler.h"
#include "lexer/ilexer.h"

namespace vellum {

namespace ast {
class Expression;
class Statement;
}  // namespace ast

class Resolver;

struct ParserResult {
  std::shared_ptr<Resolver> resolver;
  std::vector<std::unique_ptr<ast::Declaration>> declarations;
};

enum class FunctionType { Function, Event, Getter, Setter };

class Parser {
 public:
  Parser(std::unique_ptr<ILexer> lexer,
         std::shared_ptr<CompilerErrorHandler> errorHandler);

  ParserResult parse();

 private:
  std::unique_ptr<ILexer> lexer;
  std::shared_ptr<CompilerErrorHandler> errorHandler;
  std::shared_ptr<Resolver> resolver;

  Token previous;
  Token current;

  void advance();

  std::unique_ptr<ast::Declaration> declaration();

  bool match(TokenType type);
  bool match(std::initializer_list<TokenType> types);

  bool check(TokenType type) const;

  template <typename... Args>
  void consume(TokenType type, std::format_string<Args...> fmt, Args&&... args);

  std::unique_ptr<ast::Declaration> scriptDeclaration();
  std::unique_ptr<ast::Declaration> variableDeclaration();
  std::unique_ptr<ast::Declaration> functionDeclaration(
      FunctionType functionType);
  std::unique_ptr<ast::Declaration> propertyDeclaration();

  ast::FunctionBody functionBody(FunctionType type);

  std::unique_ptr<ast::Statement> statement();
  std::unique_ptr<ast::Statement> expressionStatement();
  std::unique_ptr<ast::Statement> ifStatement();
  std::unique_ptr<ast::Statement> returnStatement();
  std::unique_ptr<ast::Statement> varStatement();
  std::unique_ptr<ast::Statement> whileStatement();

  std::unique_ptr<ast::Expression> expression();
  std::unique_ptr<ast::Expression> assignExpression();
  std::unique_ptr<ast::Expression> logicalOrExpression();
  std::unique_ptr<ast::Expression> logicalAndExpression();
  std::unique_ptr<ast::Expression> equalityExpression();
  std::unique_ptr<ast::Expression> compareExpression();
  std::unique_ptr<ast::Expression> termExpression();
  std::unique_ptr<ast::Expression> factorExpression();
  std::unique_ptr<ast::Expression> unaryExpression();
  std::unique_ptr<ast::Expression> castExpression();
  std::unique_ptr<ast::Expression> callOrGetExpression();
  std::unique_ptr<ast::Expression> callExpression(
      std::unique_ptr<ast::Expression> callee, const Token& location);
  std::unique_ptr<ast::Expression> primaryExpression();

  void synchronizeDeclaration();
  void synchronizeStatement();
};
template <typename... Args>
inline void Parser::consume(TokenType type, std::format_string<Args...> fmt,
                            Args&&... args) {
  if (check(type)) {
    advance();
    return;
  }
  errorHandler->errorAt(current, std::format(fmt, std::forward<Args>(args)...));
}
}  // namespace vellum