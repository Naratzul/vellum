#include "parser.h"

#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "lexer/lexer.h"

namespace vellum {

Parser::Parser(std::unique_ptr<Lexer> lexer,
               std::shared_ptr<CompilerErrorHandler> errorHandler)
    : lexer(std::move(lexer)), errorHandler(errorHandler) {}

ParserResult Parser::parse() {
  advance();

  ParserResult result;

  while (!match(TokenType::END_OF_FILE)) {
    if (auto decl = declaration()) {
      result.declarations.push_back(std::move(decl));
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

std::unique_ptr<ast::Declaration> Parser::declaration() {
  if (match(TokenType::SCRIPT)) {
    return scriptDeclaration();
  } else if (match(TokenType::VAR)) {
    return variableDeclaration();
  } else if (match(TokenType::FUN)) {
    return functionDeclaration(FunctionType::Function);
  } else if (match(TokenType::EVENT)) {
    return functionDeclaration(FunctionType::Event);
  }

  errorHandler->errorAt(current, "Unexpected declaraion.");

  return nullptr;
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

std::unique_ptr<ast::Declaration> Parser::scriptDeclaration() {
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

  return std::make_unique<ast::ScriptDeclaration>(scriptName, parentScriptName);
}

std::unique_ptr<ast::Declaration> Parser::variableDeclaration() {
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

  return std::make_unique<ast::GlobalVariableDeclaration>(
      name, typeName, std::move(initializer));
}

std::unique_ptr<ast::Declaration> Parser::functionDeclaration(
    FunctionType functionType) {
  const std::string functionTypeName =
      functionType == FunctionType::Function ? "function" : "event";

  consume(TokenType::IDENTIFIER, "Expect a {} name.", functionTypeName);
  const std::string_view name = previous.lexeme;

  consume(TokenType::LEFT_PAREN, "Expect '(' after {} name.", functionTypeName);

  std::vector<ast::FunctionParameter> parameters;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      consume(TokenType::IDENTIFIER, "Expect a parameter name.");
      std::string_view paramName = previous.lexeme;

      consume(TokenType::COLON, "Expect ':' after parameter name.");
      consume(TokenType::IDENTIFIER, "Expect a parameter type.");
      std::string_view paramType = previous.lexeme;

      parameters.emplace_back(paramName, paramType);
    } while (match(TokenType::COMMA));
  }

  consume(TokenType::RIGHT_PAREN, "Expect ')' after {} declaration.",
          functionTypeName);

  std::optional<std::string_view> returnTypeName;
  if (functionType == FunctionType::Function && match(TokenType::ARROW)) {
    consume(TokenType::IDENTIFIER,
            "Expect a function return type name after '->'.");
    returnTypeName = previous.lexeme;
  }

  consume(TokenType::LEFT_BRACE, "Expect '{{' before {} body.",
          functionTypeName);

  std::vector<std::unique_ptr<ast::Statement>> body;

  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
    body.push_back(statement());
  }

  consume(TokenType::RIGHT_BRACE, "Expect '}}' after {} body.",
          functionTypeName);

  return std::make_unique<ast::FunctionDeclaration>(
      name, parameters, returnTypeName, std::move(body));
}

std::unique_ptr<ast::Statement> Parser::statement() {
  return expressionStatement();
}

std::unique_ptr<ast::Statement> Parser::expressionStatement() {
  return std::make_unique<ast::ExpressionStatement>(expression());
}

std::unique_ptr<ast::Expression> Parser::expression() {
  if (match({TokenType::INT, TokenType::FLOAT, TokenType::FALSE,
             TokenType::TRUE, TokenType::STRING, TokenType::NIL})) {
    return std::make_unique<ast::LiteralExpression>(previous.value);
  }

  if (match(TokenType::IDENTIFIER)) {
    return callExpression();
  }

  errorHandler->errorAt(current, "Unsupported expression.");

  return std::unique_ptr<ast::Expression>();
}

std::unique_ptr<ast::Expression> Parser::callExpression() {
  std::string_view functionName = previous.lexeme;
  std::optional<std::string_view> moduleName;

  if (match(TokenType::DOT)) {
    moduleName = functionName;

    consume(TokenType::IDENTIFIER, "Expect a function name after '.'");
    functionName = previous.lexeme;
  }

  std::vector<std::unique_ptr<ast::Expression>> arguments;

  consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      arguments.push_back(expression());
    } while (match(TokenType::COMMA));
  }

  consume(TokenType::RIGHT_PAREN, "Expect ')' after function call.");

  return std::make_unique<ast::CallExpression>(functionName, moduleName,
                                               std::move(arguments));
}

}  // namespace vellum