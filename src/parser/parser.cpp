#include "parser.h"

#include "compiler/compiler_error_handler.h"
#include "lexer/lexer.h"

namespace vellum {

Parser::Parser(std::unique_ptr<Lexer> lexer,
               std::shared_ptr<CompilerErrorHandler> errorHandler)
    : lexer(std::move(lexer)), errorHandler(errorHandler) {}

ParserResult Parser::parse() {
  advance();

  ParserResult result;

  while (!isEndOfFile()) {
    if (auto stmt = statement()) {
      result.statements.push_back(std::move(stmt));
    }
  }

  return result;
}

void Parser::advance() {
  previous = current;
  for (;;) {
    current = lexer->scanToken();
    if (current.type != TokenType::ERROR) {
      break;
    }
    errorHandler->errorAt(current, current.lexeme);
  }
}

bool Parser::isEndOfFile() const {
  return current.type == TokenType::END_OF_FILE;
}

std::unique_ptr<ast::Statement> Parser::statement() {
  if (match(TokenType::SCRIPT)) {
    return script();
  }
  return nullptr;
}

bool Parser::match(TokenType type) {
  if (!check(type)) {
    return false;
  }

  advance();

  return true;
}

bool Parser::check(TokenType type) const { return current.type == type; }

std::unique_ptr<ast::Statement> Parser::script() {
  if (!match(TokenType::IDENTIFIER)) {
    errorHandler->errorAt(current, "Expect a script's name after 'script'.");
  }

  std::string_view scriptName = current.lexeme;

  // TODO: check that scriptName == filename

  std::optional<std::string_view> parentScriptName;
  if (match(TokenType::COLON)) {
    if (!match(TokenType::IDENTIFIER)) {
      errorHandler->errorAt(current, "Expect a parent script's name after ':'.");
    }
    parentScriptName = current.lexeme;
  }

  return std::make_unique<ast::ScriptStatement>(scriptName, parentScriptName);
}

}  // namespace vellum