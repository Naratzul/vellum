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

  while (!match(TokenType::END_OF_FILE)) {
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

std::unique_ptr<ast::Statement> Parser::statement() {
  if (match(TokenType::SCRIPT)) {
    return script();
  } else if (match(TokenType::VAR)) {
    return variableDeclaration();
  }
  return expressionStatement();
}

bool Parser::match(TokenType type) {
  if (!check(type)) {
    return false;
  }

  advance();

  return true;
}

bool Parser::match(std::initializer_list<TokenType> types) {
  for (auto type : types) {
    if (check(type)) {
      advance();
      return true;
    }
  }
  return false;
}


bool Parser::check(TokenType type) const { return current.type == type; }

void Parser::consume(TokenType type, std::string_view message) {
  if (check(type)) {
    advance();
    return;
  }
  errorHandler->errorAt(current, message);
}

std::unique_ptr<ast::Statement> Parser::script() {
  if (!match(TokenType::IDENTIFIER)) {
    errorHandler->errorAt(current, "Expect a script's name after 'script'.");
  }

  std::string_view scriptName = previous.lexeme;

  // TODO: check that scriptName == filename

  std::optional<std::string_view> parentScriptName;
  if (match(TokenType::COLON)) {
    if (!match(TokenType::IDENTIFIER)) {
      errorHandler->errorAt(current,
                            "Expect a parent script's name after ':'.");
    }
    parentScriptName = previous.lexeme;
  }

  if (!check(TokenType::END_OF_FILE) && previous.line == current.line) {
    errorHandler->errorAt(current, "Unexpected token after script declaration");
  }

  return std::make_unique<ast::ScriptStatement>(scriptName, parentScriptName);
}

std::unique_ptr<ast::Statement> Parser::variableDeclaration() {
  consume(TokenType::IDENTIFIER, "Expect a variable name.");
  const std::string_view name = previous.lexeme;

  std::optional<std::string_view> typeName;
  if (match(TokenType::COLON)) {
    consume(TokenType::IDENTIFIER, "Expect a type name.");
    typeName = previous.lexeme;
  }

  std::unique_ptr<ast::Expression> initializer;
  if (match(TokenType::EQUAL)) {
    initializer = expression();
  }

  return std::make_unique<ast::VariableDeclaration>(name, typeName,
                                                    std::move(initializer));
}

std::unique_ptr<ast::Statement> Parser::expressionStatement() {
  advance();
  return nullptr;
}

std::unique_ptr<ast::Expression> Parser::expression() {
  if (match({TokenType::INT, TokenType::FLOAT, TokenType::FALSE,
             TokenType::TRUE, TokenType::STRING, TokenType::NIL})) {
    return std::make_unique<ast::LiteralExpression>(previous.value);
  }

  errorHandler->errorAt(current, "Unsupported expression.");

  return std::unique_ptr<ast::Expression>();
}

}  // namespace vellum