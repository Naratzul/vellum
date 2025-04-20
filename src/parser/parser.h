#pragma once

#include <memory>
#include <vector>

#include "ast/decl/declaration.h"
#include "lexer/token.h"

namespace vellum {

namespace ast {
class Expression;
class Statement;
}  // namespace ast

class CompilerErrorHandler;
class Lexer;

struct ParserResult {
  std::vector<std::unique_ptr<ast::Declaration>> declarations;
};

class Parser {
 public:
  Parser(std::unique_ptr<Lexer> lexer,
         std::shared_ptr<CompilerErrorHandler> errorHandler);

  ParserResult parse();

 private:
  std::unique_ptr<Lexer> lexer;
  std::shared_ptr<CompilerErrorHandler> errorHandler;

  Token previous;
  Token current;

  void advance();

  std::unique_ptr<ast::Declaration> declaration();

  bool match(TokenType type);
  bool match(std::initializer_list<TokenType> types);

  bool check(TokenType type) const;
  void consume(TokenType type, std::string_view message);

  std::unique_ptr<ast::Declaration> scriptDeclaration();
  std::unique_ptr<ast::Declaration> variableDeclaration();
  std::unique_ptr<ast::Declaration> functionDeclaration();

  std::unique_ptr<ast::Statement> statement();
  std::unique_ptr<ast::Statement> expressionStatement();
  std::unique_ptr<ast::Expression> expression();
  std::unique_ptr<ast::Expression> callExpression();
};
}  // namespace vellum