#pragma once

#include <memory>
#include <vector>

#include "ast/expression.h"
#include "ast/statement.h"
#include "lexer/token.h"

namespace vellum {

class CompilerErrorHandler;
class Lexer;

struct ParserError {
  std::string message;
};

struct ParserResult {
  std::vector<std::unique_ptr<ast::Statement>> statements;
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

  std::unique_ptr<ast::Statement> statement();

  bool match(TokenType type);
  bool match(std::initializer_list<TokenType> types);

  bool check(TokenType type) const;
  void consume(TokenType type, std::string_view message);

  std::unique_ptr<ast::Statement> script();
  std::unique_ptr<ast::Statement> variableDeclaration();
  std::unique_ptr<ast::Statement> expressionStatement();

  std::unique_ptr<ast::Expression> expression();
};
}  // namespace vellum