#include "parser.h"

#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "compiler/resolver.h"
#include "vellum/vellum_value.h"

namespace vellum {

class ParseException : public std::runtime_error {
 public:
  ParseException(Token token, const std::string& message)
      : std::runtime_error(message), token(token) {}

  Token getToken() const { return token; }
  std::string getMessage() const { return what(); }

 private:
  Token token;
};

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
    try {
      result.declarations.push_back(declaration());
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getMessage());
      synchronizeDeclaration();
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
  std::unique_ptr<ast::Declaration> decl;

  if (match(TokenType::SCRIPT)) {
    decl = scriptDeclaration();
  } else if (match(TokenType::VAR)) {
    decl = variableDeclaration();
  } else if (match(TokenType::FUN)) {
    decl = functionDeclaration(FunctionType::Function);
  } else if (match(TokenType::EVENT)) {
    decl = functionDeclaration(FunctionType::Event);
  } else if (match(TokenType::IDENTIFIER)) {
    decl = propertyDeclaration();
  } else {
    throw ParseException(current, "Expect a declaraion.");
  }

  return decl;
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

  if (!check(TokenType::END_OF_FILE) &&
      previous.location.start.line == current.location.start.line) {
    errorHandler->errorAt(current, "Unexpected token after script declaration");
  }

  resolver = std::make_shared<Resolver>(
      VellumObject(VellumIdentifier(scriptName)), errorHandler);

  return std::make_unique<ast::ScriptDeclaration>(scriptName, parentScriptName);
}

std::unique_ptr<ast::Declaration> Parser::variableDeclaration() {
  consume(TokenType::IDENTIFIER, "Expect a variable name.");
  const std::string_view name = previous.lexeme;

  bool isArray = false;
  std::optional<VellumType> typeName;
  if (match(TokenType::COLON)) {
    isArray = match(TokenType::LEFT_BRACK);

    consume(TokenType::IDENTIFIER, "Expect a type name.");

    auto subtype = VellumType::unresolved(previous.lexeme);

    typeName = isArray ? VellumType::array(subtype) : subtype;

    if (isArray) {
      consume(TokenType::RIGHT_BRACK, "Expect ']' after array type.");
    }
  }

  std::unique_ptr<ast::Expression> initializer;
  if (!isArray && match(TokenType::EQUAL)) {
    initializer = expression();
    if (!initializer->isLiteralExpression()) {
      throw ParseException(
          current,
          "Script variables can only be initialized with literal value.");
    }
  }

  if (!typeName && !initializer) {
    throw ParseException(previous, "Type annotation is missing.");
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
    try {
      body.push_back(std::move(statement()));
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getMessage());
      synchronizeStatement();
    }
  }

  consume(TokenType::RIGHT_BRACE, "Expect '}}' after {} body.",
          functionTypeName);

  return body;
}

std::unique_ptr<ast::Statement> Parser::statement() {
  std::unique_ptr<ast::Statement> stmt;

  if (match(TokenType::IF)) {
    stmt = ifStatement();
  } else if (match(TokenType::RETURN)) {
    stmt = returnStatement();
  } else if (match(TokenType::VAR)) {
    stmt = varStatement();
  } else if (match(TokenType::WHILE)) {
    stmt = whileStatement();
  } else {
    stmt = expressionStatement();
  }

  return stmt;
}

std::unique_ptr<ast::Statement> Parser::ifStatement() {
  auto condition = expression();

  consume(TokenType::LEFT_BRACE, "Expected '{{' after if condition.");
  std::vector<std::unique_ptr<ast::Statement>> then_branch;
  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
    try {
      then_branch.push_back(std::move(statement()));
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getMessage());
      synchronizeStatement();
    }
  }
  consume(TokenType::RIGHT_BRACE, "Expected '}}' after if block.");

  std::optional<std::vector<std::unique_ptr<ast::Statement>>> else_branch;
  if (match(TokenType::ELSE)) {
    std::vector<std::unique_ptr<ast::Statement>> else_statements;
    consume(TokenType::LEFT_BRACE, "Expected '{{' after 'else'.");
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
      try {
        else_statements.push_back(std::move(statement()));
      } catch (const ParseException& e) {
        errorHandler->errorAt(e.getToken(), e.getMessage());
        synchronizeStatement();
      }
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}}' after else block.");
    else_branch = std::move(else_statements);
  }

  return std::make_unique<ast::IfStatement>(
      std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::unique_ptr<ast::Statement> Parser::returnStatement() {
  return std::make_unique<ast::ReturnStatement>(expression());
}

std::unique_ptr<ast::Statement> Parser::varStatement() {
  consume(TokenType::IDENTIFIER, "Expect a variable name.");
  const VellumIdentifier name(previous.lexeme);

  std::optional<VellumType> type;
  if (match(TokenType::COLON)) {
    consume(TokenType::IDENTIFIER, "Expect a type name.");
    type = VellumType::unresolved(previous.lexeme);
  }

  std::unique_ptr<ast::Expression> initializer;
  if (match(TokenType::EQUAL)) {
    initializer = expression();
  }

  if (!type && !initializer) {
    throw ParseException(previous, "Type annotation is missing.");
  }

  return std::make_unique<ast::LocalVariableStatement>(name, type,
                                                       std::move(initializer));
}

std::unique_ptr<ast::Statement> Parser::whileStatement() {
  auto condition = expression();
  consume(TokenType::LEFT_BRACE, "Expect '{{' after condition.");

  ast::WhileStatement::Body body;
  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
    try {
      body.push_back(std::move(statement()));
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getMessage());
      synchronizeStatement();
    }
  }

  consume(TokenType::RIGHT_BRACE, "Expect '}}' after while loop.");

  return std::make_unique<ast::WhileStatement>(std::move(condition),
                                               std::move(body));
}

std::unique_ptr<ast::Statement> Parser::expressionStatement() {
  return std::make_unique<ast::ExpressionStatement>(expression());
}

std::unique_ptr<ast::Expression> Parser::expression() {
  return assignExpression();
}

std::unique_ptr<ast::Expression> Parser::assignExpression() {
  auto expr = logicalOrExpression();

  if (match(TokenType::EQUAL)) {
    if (expr->isIdentifierExpression()) {
      return std::make_unique<ast::AssignExpression>(
          expr->asIdentifier().getIdentifier(), assignExpression(), previous);
    } else if (expr->isPropertyGetExpression()) {
      ast::PropertyGetExpression& getExpr = expr->asPropertyGet();
      return std::make_unique<ast::PropertySetExpression>(
          getExpr.releaseObject(), getExpr.getProperty(), assignExpression(),
          previous);
    } else {
      errorHandler->errorAt(previous, "Invalid assignment target.");
    }
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::logicalOrExpression() {
  auto expr = logicalAndExpression();

  while (match(TokenType::OR)) {
    expr = std::make_unique<ast::BinaryExpression>(
        ast::BinaryExpression::Operator::Or, std::move(expr),
        logicalAndExpression(), previous);
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::logicalAndExpression() {
  auto expr = equalityExpression();

  while (match(TokenType::AND)) {
    expr = std::make_unique<ast::BinaryExpression>(
        ast::BinaryExpression::Operator::And, std::move(expr),
        equalityExpression(), previous);
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::equalityExpression() {
  auto expr = compareExpression();

  while (match({TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL})) {
    const Token op = previous;
    expr = std::make_unique<ast::BinaryExpression>(
        op.type == TokenType::EQUAL_EQUAL
            ? ast::BinaryExpression::Operator::Equal
            : ast::BinaryExpression::Operator::NotEqual,
        std::move(expr), compareExpression(), op);
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::compareExpression() {
  auto expr = termExpression();

  while (match({TokenType::LESS, TokenType::LESS_EQUAL, TokenType::GREATER,
                TokenType::GREATER_EQUAL})) {
    const Token op = previous;
    auto binOp = ast::BinaryExpression::Operator::LessThan;
    if (op.type == TokenType::LESS_EQUAL) {
      binOp = ast::BinaryExpression::Operator::LessThanEqual;
    } else if (op.type == TokenType::GREATER) {
      binOp = ast::BinaryExpression::Operator::GreaterThan;
    } else if (op.type == TokenType::GREATER_EQUAL) {
      binOp = ast::BinaryExpression::Operator::GreaterThanEqual;
    }
    expr = std::make_unique<ast::BinaryExpression>(binOp, std::move(expr),
                                                   termExpression(), op);
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::termExpression() {
  auto expr = factorExpression();

  while (match({TokenType::MINUS, TokenType::PLUS})) {
    const Token op = previous;
    expr = std::make_unique<ast::BinaryExpression>(
        op.type == TokenType::MINUS ? ast::BinaryExpression::Operator::Subtract
                                    : ast::BinaryExpression::Operator::Add,
        std::move(expr), factorExpression(), op);
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::factorExpression() {
  auto expr = unaryExpression();

  while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
    const Token op = previous;
    ast::BinaryExpression::Operator binOp;
    switch (op.type) {
      case TokenType::STAR:
        binOp = ast::BinaryExpression::Operator::Multiply;
        break;
      case TokenType::SLASH:
        binOp = ast::BinaryExpression::Operator::Divide;
        break;
      case TokenType::PERCENT:
        binOp = ast::BinaryExpression::Operator::Modulo;
        break;
      default:
        assert(false && "Unexpected operator type");
    }
    expr = std::make_unique<ast::BinaryExpression>(binOp, std::move(expr),
                                                   factorExpression(), op);
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::unaryExpression() {
  if (match({TokenType::MINUS, TokenType::NOT})) {
    const Token op = previous;
    return std::make_unique<ast::UnaryExpression>(
        op.type == TokenType::MINUS ? ast::UnaryExpression::Operator::Negate
                                    : ast::UnaryExpression::Operator::Not,
        op.type == TokenType::MINUS ? unaryExpression() : logicalOrExpression(),
        op);
  }
  return castExpression();
}

std::unique_ptr<ast::Expression> Parser::castExpression() {
  auto expr = callOrGetExpression();

  if (match(TokenType::AS)) {
    const Token op = previous;
    consume(TokenType::IDENTIFIER, "Expect a target type for cast.");
    return std::make_unique<ast::CastExpression>(
        std::move(expr), VellumType::unresolved(previous.lexeme), op);
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::callOrGetExpression() {
  auto expr = primaryExpression();

  while (true) {
    if (match(TokenType::LEFT_PAREN)) {
      const Token op = previous;
      expr = callExpression(std::move(expr), op);
    } else if (match(TokenType::DOT)) {
      const Token op = previous;
      consume(TokenType::IDENTIFIER, "Expect a property name after '.'");
      expr = std::make_unique<ast::PropertyGetExpression>(
          std::move(expr), VellumIdentifier(previous.lexeme), op);
    } else {
      break;
    }
  }

  return expr;
}

std::unique_ptr<ast::Expression> Parser::callExpression(
    std::unique_ptr<ast::Expression> callee, const Token& location) {
  std::vector<std::unique_ptr<ast::Expression>> arguments;

  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      arguments.push_back(expression());
    } while (match(TokenType::COMMA));
  }

  consume(TokenType::RIGHT_PAREN, "Expect ')' after function call.");

  return std::make_unique<ast::CallExpression>(std::move(callee),
                                               std::move(arguments), location);
}

std::unique_ptr<ast::Expression> Parser::primaryExpression() {
  if (match({TokenType::INT, TokenType::FLOAT, TokenType::FALSE,
             TokenType::TRUE, TokenType::STRING, TokenType::NIL})) {
    return std::make_unique<ast::LiteralExpression>(*previous.value, previous);
  }

  if (match(TokenType::IDENTIFIER)) {
    return std::make_unique<ast::IdentifierExpression>(
        VellumIdentifier(previous.lexeme), previous);
  }

  if (match(TokenType::LEFT_PAREN)) {
    const Token op = previous;
    auto expr = expression();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
    return std::make_unique<ast::GroupingExpression>(std::move(expr), op);
  }

  if (match(TokenType::LEFT_BRACK)) {
    return arrayExpression();
  }

  throw ParseException(current, "Expect an expression.");
}

std::unique_ptr<ast::Expression> Parser::arrayExpression() {
  std::optional<VellumType> subtype;

  if (match(TokenType::RIGHT_BRACK)) {
    return std::make_unique<ast::NewArrayExpression>(subtype, VellumLiteral(0), previous);
  }

  consume(TokenType::IDENTIFIER, "Expect array elements type.");
  subtype = VellumType::unresolved(previous.lexeme);

  consume(TokenType::SEMICOLON, "Expect ';' after array elements type.");

  consume(TokenType::INT, "Expect array length as Int value.");
  if (!previous.value.has_value()) {
    throw ParseException(previous, "Array length must be provided.");
  }

  const VellumLiteral length = previous.value.value();

  consume(TokenType::RIGHT_BRACK, "Expect ']' after array.");

  return std::make_unique<ast::NewArrayExpression>(subtype, length, previous);
}

void Parser::synchronizeDeclaration() {
  errorHandler->disablePanicMode();
  while (!check(TokenType::END_OF_FILE)) {
    switch (current.type) {
      case TokenType::FUN:
      case TokenType::EVENT:
      case TokenType::IDENTIFIER:
      case TokenType::VAR:
      case TokenType::SCRIPT:
        return;
      default:
        break;
    }
    advance();
  }
}

void Parser::synchronizeStatement() {
  errorHandler->disablePanicMode();
  while (!check(TokenType::END_OF_FILE)) {
    switch (current.type) {
      case TokenType::VAR:
      case TokenType::IF:
      case TokenType::FOR:
      case TokenType::WHILE:
      case TokenType::RETURN:
      case TokenType::RIGHT_BRACE:
        return;
      default:
        break;
    }
    advance();
  }
}

}  // namespace vellum