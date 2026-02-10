#include "parser.h"

#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "common/types.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::makeUnique;
using common::Opt;
using common::Shared;
using common::Unique;
using common::Vec;

namespace {
class ParseException : public std::runtime_error {
 public:
  ParseException(Token token, const std::string& message)
      : std::runtime_error(message), token(token) {}

  Token getToken() const { return token; }
  std::string getMessage() const { return what(); }

 private:
  Token token;
};
}  // namespace

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

Parser::Parser(Unique<ILexer> lexer, Shared<CompilerErrorHandler> errorHandler)
    : lexer(std::move(lexer)), errorHandler(errorHandler) {}

ParserResult Parser::parse() {
  advance();

  ParserResult result;

  while (!check(TokenType::END_OF_FILE)) {
    try {
      result.declarations.push_back(topDeclaration());
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getMessage());
      synchronizeTopDeclaration();
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

Unique<ast::Declaration> Parser::topDeclaration() {
  if (match(TokenType::IMPORT)) {
    return importDeclaration();
  } else if (match(TokenType::SCRIPT)) {
    return scriptDeclaration();
  } else if (match(TokenType::AUTO)) {
    consume(TokenType::STATE, CompilerErrorKind::ExpectDeclaration,
            "Expect 'state' after 'auto'.");
    return stateDeclaration(true);
  } else if (match(TokenType::STATE)) {
    return stateDeclaration(false);
  }

  throw ParseException(
      previous, "Expect a top-level declaration: script, state or import.");
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

Unique<ast::Declaration> Parser::scriptDeclaration() {
  if (!match(TokenType::IDENTIFIER)) {
    errorHandler->errorAt(current, "Expect a script's name after 'script'.");
  }

  Token scriptNameLocation = previous;
  auto scriptName = VellumType::identifier(previous.lexeme);

  auto parentScriptName = VellumType::none();
  Opt<Token> parentScriptNameLocation;
  if (match(TokenType::COLON)) {
    if (!match(TokenType::IDENTIFIER)) {
      errorHandler->errorAt(current,
                            "Expect a parent script's name after ':'.");
    }
    parentScriptName = VellumType::identifier(previous.lexeme);
    parentScriptNameLocation = previous;
  }

  Vec<Unique<ast::Declaration>> scriptMembers;

  if (match(TokenType::LEFT_BRACE)) {
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
      try {
        scriptMembers.push_back(scriptMemberDeclaration());
      } catch (const ParseException& e) {
        errorHandler->errorAt(e.getToken(), e.getMessage());
        synchronizeDeclaration();
      }
    }

    consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
            "Expect '}}' after script declaration.");
  } else if (!check(TokenType::END_OF_FILE) &&
             previous.location.start.line == current.location.start.line) {
    throw ParseException(current, "Unexpected token after script declaration.");
  }

  return makeUnique<ast::ScriptDeclaration>(
      scriptName, scriptNameLocation, parentScriptName,
      parentScriptNameLocation, std::move(scriptMembers));
}

Unique<ast::Declaration> Parser::importDeclaration() {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
          "Import name expected.");
  std::string_view importName = previous.lexeme;
  return makeUnique<ast::ImportDeclaration>(importName, previous);
}

Unique<ast::Declaration> Parser::stateDeclaration(bool isAuto) {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
          "Expect a state name after 'state'.");
  std::string_view stateName = previous.lexeme;
  Token stateNameLocation = previous;

  Vec<Unique<ast::Declaration>> stateMembers;

  if (match(TokenType::LEFT_BRACE)) {
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
      try {
        stateMembers.push_back(scriptMemberDeclaration());
      } catch (const ParseException& e) {
        errorHandler->errorAt(e.getToken(), e.getMessage());
        synchronizeDeclaration();
      }
    }

    consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
            "Expect '}}' after state declaration.");
  } else if (!check(TokenType::END_OF_FILE) &&
             previous.location.start.line == current.location.start.line) {
    throw ParseException(current, "Unexpected token after state declaration.");
  }

  return makeUnique<ast::StateDeclaration>(stateName, stateNameLocation, isAuto,
                                           std::move(stateMembers));
}

Unique<ast::Declaration> Parser::scriptMemberDeclaration() {
  if (match(TokenType::VAR)) {
    return variableDeclaration();
  } else if (match(TokenType::FUN)) {
    return functionDeclaration(FunctionType::Function, false);
  } else if (match(TokenType::EVENT)) {
    return functionDeclaration(FunctionType::Event, false);
  } else if (match(TokenType::IDENTIFIER)) {
    return propertyDeclaration();
  } else if (match(TokenType::STATIC)) {
    consume(TokenType::FUN, CompilerErrorKind::ExpectDeclaration,
            "Expect function declaration after 'static'.");
    return functionDeclaration(FunctionType::Function, true);
  }

  throw ParseException(current,
                       "Expect a declaraion: var, property or fun/event.");
}

Unique<ast::Declaration> Parser::variableDeclaration() {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectVarName,
          "Expect a variable name.");
  const std::string_view name = previous.lexeme;
  Token nameLocation = previous;

  bool isArray = false;
  Opt<VellumType> typeName;
  Opt<Token> typeLocation = std::nullopt;
  if (match(TokenType::COLON)) {
    isArray = match(TokenType::LEFT_BRACK);

    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectTypeName,
            "Expect a type name.");

    auto subtype = VellumType::unresolved(previous.lexeme);
    typeLocation = previous;

    typeName = isArray ? VellumType::array(subtype) : subtype;

    if (isArray) {
      consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
              "Expect ']' after array type.");
    }
  }

  Unique<ast::Expression> initializer;
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

  return makeUnique<ast::GlobalVariableDeclaration>(
      name, typeName, std::move(initializer), nameLocation, typeLocation);
}

Unique<ast::Declaration> Parser::functionDeclaration(FunctionType functionType,
                                                     bool isStatic) {
  const std::string functionTypeName =
      functionType == FunctionType::Function ? "function" : "event";

  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectFunctionName,
          "Expect a {} name.", functionTypeName);
  const std::string_view name = previous.lexeme;
  Token nameLocation = previous;

  consume(TokenType::LEFT_PAREN, CompilerErrorKind::ExpectLeftParen,
          "Expect '(' after {} name.", functionTypeName);

  Vec<ast::FunctionParameter> parameters;
  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectParamName,
              "Expect a parameter name.");
      std::string_view paramName = previous.lexeme;
      Token paramNameLocation = previous;

      consume(TokenType::COLON, CompilerErrorKind::ExpectColon,
              "Expect ':' after parameter name.");
      consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectTypeName,
              "Expect a parameter type.");
      std::string_view paramType = previous.lexeme;
      Token paramTypeLocation = previous;

      parameters.emplace_back(paramName, VellumType::unresolved(paramType),
                              paramNameLocation, paramTypeLocation);
    } while (match(TokenType::COMMA));
  }

  consume(TokenType::RIGHT_PAREN, CompilerErrorKind::ExpectRightParen,
          "Expect ')' after {} declaration.", functionTypeName);

  Opt<Token> returnTypeLocation = std::nullopt;
  auto returnTypeName = VellumType::none();
  if (functionType == FunctionType::Function && match(TokenType::ARROW)) {
    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectReturnType,
            "Expect a function return type name after '->'.");
    returnTypeName = VellumType::unresolved(previous.lexeme);
    returnTypeLocation = previous;
  }

  return makeUnique<ast::FunctionDeclaration>(
      name, parameters, returnTypeName, functionBody(functionType), isStatic,
      nameLocation, returnTypeLocation);
}

Unique<ast::Declaration> Parser::propertyDeclaration() {
  const std::string_view name = previous.lexeme;
  Token nameLocation = previous;
  consume(TokenType::COLON, CompilerErrorKind::ExpectColon,
          "Expect ':' after property name.");

  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectTypeName,
          "Expect a property type name.");
  const auto typeName = VellumType::unresolved(previous.lexeme);
  Token typeLocation = previous;

  consume(TokenType::LEFT_BRACE, CompilerErrorKind::ExpectLeftBrace,
          "Expect '{{' after property type.");

  Opt<ast::FunctionBody> getAccessor;
  Opt<ast::FunctionBody> setAccessor;

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

  consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
          "Expect '}}' after property type.");

  std::string_view documentationString = "";
  return makeUnique<ast::PropertyDeclaration>(
      name, typeName, documentationString, std::move(getAccessor),
      std::move(setAccessor), std::nullopt, nameLocation,
      typeLocation);  // TODO: fix default value
}

ast::FunctionBody Parser::functionBody(FunctionType type) {
  const std::string_view functionTypeName = getFunctionTypeName(type);
  consume(TokenType::LEFT_BRACE, CompilerErrorKind::ExpectLeftBrace,
          "Expect '{{' before {} body.", functionTypeName);

  ast::FunctionBody body;

  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
    try {
      body.push_back(statement());
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getMessage());
      synchronizeStatement();
    }
  }

  consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
          "Expect '}}' after {} body.", functionTypeName);

  return body;
}

Unique<ast::Statement> Parser::statement() {
  Unique<ast::Statement> stmt;

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

Unique<ast::Statement> Parser::ifStatement() {
  auto condition = expression();

  consume(TokenType::LEFT_BRACE, CompilerErrorKind::ExpectLeftBrace,
          "Expected '{{' after if condition.");
  Vec<Unique<ast::Statement>> then_branch;
  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
    try {
      then_branch.push_back(statement());
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getMessage());
      synchronizeStatement();
    }
  }
  consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
          "Expected '}}' after if block.");

  Opt<Vec<Unique<ast::Statement>>> else_branch;
  if (match(TokenType::ELSE)) {
    Vec<Unique<ast::Statement>> else_statements;
    consume(TokenType::LEFT_BRACE, CompilerErrorKind::ExpectLeftBrace,
            "Expected '{{' after 'else'.");
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
      try {
        else_statements.push_back(statement());
      } catch (const ParseException& e) {
        errorHandler->errorAt(e.getToken(), e.getMessage());
        synchronizeStatement();
      }
    }
    consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
            "Expected '}}' after else block.");
    else_branch = std::move(else_statements);
  }

  return makeUnique<ast::IfStatement>(
      std::move(condition), std::move(then_branch), std::move(else_branch));
}

Unique<ast::Statement> Parser::returnStatement() {
  return makeUnique<ast::ReturnStatement>(expression());
}

Unique<ast::Statement> Parser::varStatement() {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectVarName,
          "Expect a variable name.");
  const VellumIdentifier name(previous.lexeme);
  Token nameLocation = previous;

  Opt<VellumType> type;
  Opt<Token> typeLocation = std::nullopt;
  if (match(TokenType::COLON)) {
    bool isArray = match(TokenType::LEFT_BRACK);
    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectTypeName,
            "Expect a type name.");

    auto subtype = VellumType::unresolved(previous.lexeme);
    typeLocation = previous;
    type = isArray ? VellumType::array(subtype) : subtype;

    if (isArray) {
      consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
              "Expect ']' after array type.");
    }
  }

  Unique<ast::Expression> initializer;
  if (match(TokenType::EQUAL)) {
    initializer = expression();
  }

  if (!type && !initializer) {
    throw ParseException(previous, "Type annotation is missing.");
  }

  return makeUnique<ast::LocalVariableStatement>(
      name, type, std::move(initializer), nameLocation, typeLocation);
}

Unique<ast::Statement> Parser::whileStatement() {
  auto condition = expression();
  consume(TokenType::LEFT_BRACE, CompilerErrorKind::ExpectLeftBrace,
          "Expect '{{' after condition.");

  ast::WhileStatement::Body body;
  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
    try {
      body.push_back(statement());
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getMessage());
      synchronizeStatement();
    }
  }

  consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
          "Expect '}}' after while loop.");

  return makeUnique<ast::WhileStatement>(std::move(condition), std::move(body));
}

Unique<ast::Statement> Parser::expressionStatement() {
  return makeUnique<ast::ExpressionStatement>(expression());
}

Unique<ast::Expression> Parser::expression() { return assignExpression(); }

Unique<ast::Expression> Parser::assignExpression() {
  auto expr = logicalOrExpression();

  if (match(TokenType::EQUAL)) {
    if (expr->isIdentifierExpression()) {
      return makeUnique<ast::AssignExpression>(
          expr->asIdentifier().getIdentifier(), assignExpression(), previous);
    } else if (expr->isPropertyGetExpression()) {
      ast::PropertyGetExpression& getExpr = expr->asPropertyGet();
      return makeUnique<ast::PropertySetExpression>(
          getExpr.releaseObject(), getExpr.getProperty(), assignExpression(),
          previous);
    } else if (expr->isArrayIndexExpression()) {
      ast::ArrayIndexExpression& indexExpr = expr->asArrayIndex();
      return makeUnique<ast::ArrayIndexSetExpression>(
          indexExpr.releaseArray(), indexExpr.releaseIndex(),
          assignExpression(), previous);
    } else {
      errorHandler->errorAt(previous, "Invalid assignment target.");
    }
  }

  return expr;
}

Unique<ast::Expression> Parser::logicalOrExpression() {
  auto expr = logicalAndExpression();

  while (match(TokenType::OR)) {
    expr = makeUnique<ast::BinaryExpression>(
        ast::BinaryExpression::Operator::Or, std::move(expr),
        logicalAndExpression(), previous);
  }

  return expr;
}

Unique<ast::Expression> Parser::logicalAndExpression() {
  auto expr = equalityExpression();

  while (match(TokenType::AND)) {
    expr = makeUnique<ast::BinaryExpression>(
        ast::BinaryExpression::Operator::And, std::move(expr),
        equalityExpression(), previous);
  }

  return expr;
}

Unique<ast::Expression> Parser::equalityExpression() {
  auto expr = compareExpression();

  while (match({TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL})) {
    const Token op = previous;
    expr = makeUnique<ast::BinaryExpression>(
        op.type == TokenType::EQUAL_EQUAL
            ? ast::BinaryExpression::Operator::Equal
            : ast::BinaryExpression::Operator::NotEqual,
        std::move(expr), compareExpression(), op);
  }

  return expr;
}

Unique<ast::Expression> Parser::compareExpression() {
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
    expr = makeUnique<ast::BinaryExpression>(binOp, std::move(expr),
                                             termExpression(), op);
  }

  return expr;
}

Unique<ast::Expression> Parser::termExpression() {
  auto expr = factorExpression();

  while (match({TokenType::MINUS, TokenType::PLUS})) {
    const Token op = previous;
    expr = makeUnique<ast::BinaryExpression>(
        op.type == TokenType::MINUS ? ast::BinaryExpression::Operator::Subtract
                                    : ast::BinaryExpression::Operator::Add,
        std::move(expr), factorExpression(), op);
  }

  return expr;
}

Unique<ast::Expression> Parser::factorExpression() {
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
    expr = makeUnique<ast::BinaryExpression>(binOp, std::move(expr),
                                             factorExpression(), op);
  }

  return expr;
}

Unique<ast::Expression> Parser::unaryExpression() {
  if (match({TokenType::MINUS, TokenType::NOT})) {
    const Token op = previous;
    return makeUnique<ast::UnaryExpression>(
        op.type == TokenType::MINUS ? ast::UnaryExpression::Operator::Negate
                                    : ast::UnaryExpression::Operator::Not,
        op.type == TokenType::MINUS ? unaryExpression() : logicalOrExpression(),
        op);
  }
  return castExpression();
}

Unique<ast::Expression> Parser::castExpression() {
  auto expr = callOrGetExpression();

  if (match(TokenType::AS)) {
    const Token op = previous;
    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectCastTargetType,
            "Expect a target type for cast.");
    return makeUnique<ast::CastExpression>(
        std::move(expr), VellumType::unresolved(previous.lexeme), op);
  }

  return expr;
}

Unique<ast::Expression> Parser::callOrGetExpression() {
  auto expr = primaryExpression();

  while (true) {
    if (match(TokenType::LEFT_BRACK)) {
      const Token location = previous;
      auto index = expression();
      consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
              "Expect ']' after array index.");
      expr = makeUnique<ast::ArrayIndexExpression>(std::move(expr),
                                                   std::move(index), location);
    } else if (match(TokenType::LEFT_PAREN)) {
      expr = callExpression(std::move(expr), previous);
    } else if (match(TokenType::DOT)) {
      consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectVarName,
              "Expect a property name after '.'");
      expr = makeUnique<ast::PropertyGetExpression>(
          std::move(expr), VellumIdentifier(previous.lexeme), previous);
    } else {
      break;
    }
  }

  return expr;
}

Unique<ast::Expression> Parser::callExpression(Unique<ast::Expression> callee,
                                               const Token& location) {
  Vec<Unique<ast::Expression>> arguments;

  if (!check(TokenType::RIGHT_PAREN)) {
    do {
      arguments.push_back(expression());
    } while (match(TokenType::COMMA));
  }

  consume(TokenType::RIGHT_PAREN, CompilerErrorKind::ExpectRightParen,
          "Expect ')' after function call.");

  return makeUnique<ast::CallExpression>(std::move(callee),
                                         std::move(arguments), location);
}

Unique<ast::Expression> Parser::primaryExpression() {
  if (match({TokenType::INT, TokenType::FLOAT, TokenType::FALSE,
             TokenType::TRUE, TokenType::STRING, TokenType::NONE})) {
    return makeUnique<ast::LiteralExpression>(*previous.value, previous);
  }

  if (match(TokenType::SUPER)) {
    return makeUnique<ast::SuperExpression>(previous);
  }

  if (match(TokenType::IDENTIFIER)) {
    return makeUnique<ast::IdentifierExpression>(
        VellumIdentifier(previous.lexeme), previous);
  }

  if (match(TokenType::LEFT_PAREN)) {
    const Token op = previous;
    auto expr = expression();
    consume(TokenType::RIGHT_PAREN, CompilerErrorKind::ExpectRightParen,
            "Expect ')' after expression.");
    return makeUnique<ast::GroupingExpression>(std::move(expr), op);
  }

  if (match(TokenType::LEFT_BRACK)) {
    return arrayExpression();
  }

  throw ParseException(current, "Expect an expression.");
}

Unique<ast::Expression> Parser::arrayExpression() {
  Opt<VellumType> subtype;

  if (match(TokenType::RIGHT_BRACK)) {
    return makeUnique<ast::NewArrayExpression>(subtype, VellumLiteral(0),
                                               previous);
  }

  consume(TokenType::IDENTIFIER, CompilerErrorKind::ArrayTypeMissing,
          "Expect array elements type.");
  subtype = VellumType::unresolved(previous.lexeme);

  consume(TokenType::SEMICOLON, CompilerErrorKind::ArraySemicolonMissing,
          "Expect ';' after array elements type.");

  consume(TokenType::INT, CompilerErrorKind::ArrayLengthAsIntExpected,
          "Expect array length as Int value.");
  if (!previous.value.has_value()) {
    throw ParseException(previous, "Array length must be provided.");
  }

  const VellumLiteral length = previous.value.value();

  consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
          "Expect ']' after array.");

  return makeUnique<ast::NewArrayExpression>(subtype, length, previous);
}

void Parser::synchronizeTopDeclaration() {
  errorHandler->disablePanicMode();
  while (!check(TokenType::END_OF_FILE)) {
    switch (current.type) {
      case TokenType::SCRIPT:
        return;
      default:
        break;
    }
    advance();
  }
}

void Parser::synchronizeDeclaration() {
  errorHandler->disablePanicMode();
  while (!check(TokenType::END_OF_FILE)) {
    switch (current.type) {
      case TokenType::FUN:
      case TokenType::EVENT:
      case TokenType::IDENTIFIER:
      case TokenType::VAR:
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