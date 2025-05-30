#include "parser.h"

#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "compiler/resolver.h"
#include "vellum/vellum_value.h"

namespace vellum {

static std::string_view getFunctionTypeName(FunctionType type) {
  switch (type) {
    case FunctionType::Event:
      return "event";
    case FunctionType::Function:
      return "function";
    case FunctionType::Getter:
      return "get";
    case FunctionType::Setter:
      return "set";
    default:
      assert(false && "Unknown function type");
  }
}

Parser::Parser(std::unique_ptr<ILexer> lexer,
               std::shared_ptr<CompilerErrorHandler> errorHandler)
    : lexer(std::move(lexer)), errorHandler(errorHandler) {}

ParserResult Parser::parse() {
  advance();

  ParserResult result;

  while (!check(TokenType::END_OF_FILE)) {
    if (auto decl = declaration()) {
      result.declarations.push_back(std::move(decl));
    }
    if (errorHandler->isPanicMode()) {
      synchronize();
    }
  }

  result.resolver = resolver;

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
  } else if (match(TokenType::IDENTIFIER)) {
    return propertyDeclaration();
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

  resolver =
      std::make_shared<Resolver>(VellumObject(VellumIdentifier(scriptName)));

  return std::make_unique<ast::ScriptDeclaration>(scriptName, parentScriptName);
}

std::unique_ptr<ast::Declaration> Parser::variableDeclaration() {
  consume(TokenType::IDENTIFIER, "Expect a variable name.");
  const std::string_view name = previous.lexeme;

  std::optional<VellumType> typeName;
  if (match(TokenType::COLON)) {
    consume(TokenType::IDENTIFIER, "Expect a type name.");
    typeName = VellumType::unresolved(previous.lexeme);
  }

  std::unique_ptr<ast::Expression> initializer;
  if (match(TokenType::EQUAL)) {
    initializer = expression();
    if (initializer && !initializer->isLiteralExpression()) {
      errorHandler->errorAt(
          current,
          "Script variables can only be initialized with literal value.");
      return nullptr;
    }
  }

  if (!typeName && !initializer) {
    errorHandler->errorAt(current, "Type annotation is missing.");
    return nullptr;
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

      parameters.emplace_back(paramName, VellumType::unresolved(paramType));
    } while (match(TokenType::COMMA));
  }

  consume(TokenType::RIGHT_PAREN, "Expect ')' after {} declaration.",
          functionTypeName);

  auto returnTypeName = VellumType::none();
  if (functionType == FunctionType::Function && match(TokenType::ARROW)) {
    consume(TokenType::IDENTIFIER,
            "Expect a function return type name after '->'.");
    returnTypeName = VellumType::unresolved(previous.lexeme);
  }

  return std::make_unique<ast::FunctionDeclaration>(
      name, parameters, returnTypeName, functionBody(functionType));
}

std::unique_ptr<ast::Declaration> Parser::propertyDeclaration() {
  const std::string_view name = previous.lexeme;
  consume(TokenType::COLON, "Expect ':' after property name.");

  consume(TokenType::IDENTIFIER, "Expect a property type name.");
  const auto typeName = VellumType::unresolved(previous.lexeme);

  consume(TokenType::LEFT_BRACE, "Expect '{{' after property type.");

  std::optional<ast::FunctionBody> getAccessor;
  std::optional<ast::FunctionBody> setAccessor;

  for (int i = 0; i < 2; ++i) {
    if (match(TokenType::GET)) {
      if (getAccessor.has_value()) {
        errorHandler->errorAt(
            previous,
            std::format("Get accessor for property '{}' already declared.",
                        name));
        continue;
      }
      getAccessor = ast::FunctionBody();
      if (check(TokenType::LEFT_BRACE)) {
        getAccessor = functionBody(FunctionType::Getter);
      }
    } else if (match(TokenType::SET)) {
      if (setAccessor.has_value()) {
        errorHandler->errorAt(
            previous,
            std::format("Set accessor for property '{}' already declared.",
                        name));
        continue;
      }
      setAccessor = ast::FunctionBody();
      if (check(TokenType::LEFT_BRACE)) {
        setAccessor = functionBody(FunctionType::Setter);
      }
    }
  }

  consume(TokenType::RIGHT_BRACE, "Expect '}}' after property type.");

  std::string_view documentationString = "";
  return std::make_unique<ast::PropertyDeclaration>(
      name, typeName, documentationString, std::move(getAccessor),
      std::move(setAccessor), VellumValue());  // TODO: fix default value
}

ast::FunctionBody Parser::functionBody(FunctionType type) {
  const std::string_view functionTypeName = getFunctionTypeName(type);
  consume(TokenType::LEFT_BRACE, "Expect '{{' before {} body.",
          functionTypeName);

  ast::FunctionBody body;

  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
    body.push_back(statement());
  }

  consume(TokenType::RIGHT_BRACE, "Expect '}}' after {} body.",
          functionTypeName);

  return body;
}

std::unique_ptr<ast::Statement> Parser::statement() {
  if (match(TokenType::RETURN)) {
    return std::make_unique<ast::ReturnStatement>(expression());
  }

  return expressionStatement();
}

std::unique_ptr<ast::Statement> Parser::expressionStatement() {
  return std::make_unique<ast::ExpressionStatement>(expression());
}

std::unique_ptr<ast::Expression> Parser::expression() {
  return assignExpression();
}

std::unique_ptr<ast::Expression> Parser::assignExpression() {
  auto expr = callOrGetExpression();

  if (match(TokenType::EQUAL)) {
    if (expr->isIdentifierExpression()) {
      return std::make_unique<ast::AssignExpression>(
          expr->asIdentifier().getIdentifier(), expression());
    } else if (expr->isPropertyGetExpression()) {
    } else {
      errorHandler->errorAt(previous, "Invalid assignment target.");
    }
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::callOrGetExpression() {
  auto expr = primaryExpression();

  while (true) {
    if (match(TokenType::LEFT_PAREN)) {
      expr = callExpression(std::move(expr));
    } else if (match(TokenType::DOT)) {
      consume(TokenType::IDENTIFIER, "Expect a property name after '.'");
      expr = std::make_unique<ast::GetExpression>(
          std::move(expr), VellumIdentifier(previous.lexeme));
    } else {
      break;
    }
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::callExpression(
    std::unique_ptr<ast::Expression> callee) {
  std::vector<std::unique_ptr<ast::Expression>> arguments;

  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      arguments.push_back(expression());
    } while (match(TokenType::COMMA));
  }

  consume(TokenType::RIGHT_PAREN, "Expect ')' after function call.");

  return std::make_unique<ast::CallExpression>(std::move(callee),
                                               std::move(arguments));
}

std::unique_ptr<ast::Expression> Parser::primaryExpression() {
  if (match({TokenType::INT, TokenType::FLOAT, TokenType::FALSE,
             TokenType::TRUE, TokenType::STRING, TokenType::NIL})) {
    return std::make_unique<ast::LiteralExpression>(*previous.value);
  }

  if (match(TokenType::IDENTIFIER)) {
    return std::make_unique<ast::IdentifierExpression>(
        VellumIdentifier(previous.lexeme));
  }

  errorHandler->errorAt(current, "Unexpected expression.");
  return nullptr;
}

void Parser::synchronize() {
  errorHandler->disablePanicMode();
  while (!check(TokenType::END_OF_FILE)) {
    switch (current.type) {
      case TokenType::FUN:
      case TokenType::IDENTIFIER:
      case TokenType::VAR:
      case TokenType::SCRIPT:
      case TokenType::IF:
      case TokenType::FOR:
      case TokenType::WHILE:
      case TokenType::RETURN:
        return;
      default:
        break;
    }
    advance();
  }
}

}  // namespace vellum