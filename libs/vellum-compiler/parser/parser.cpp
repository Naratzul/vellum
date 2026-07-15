#include "parser.h"

#include <optional>
#include <string_view>

#include "ast/expression/expression.h"
#include "ast/statement/statement.h"
#include "common/types.h"
#include "lexer/token.h"
#include "vellum/vellum_literal.h"
#include "vellum/vellum_type.h"
#include "vellum/vellum_value.h"

namespace vellum {
using common::makeUnique;
using common::Opt;
using common::Shared;
using common::Unique;
using common::Vec;

Parser::Parser(Unique<ILexer> lexer, Shared<CompilerErrorHandler> errorHandler)
    : lexer(std::move(lexer)), errorHandler(errorHandler) {}

ParserResult Parser::parse() {
  advance();

  ParserResult result;

  while (!check(TokenType::END_OF_FILE)) {
    try {
      result.declarations.push_back(topDeclaration());
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getErrorKind(), e.what());
      synchronizeTopDeclaration();
    }
  }

  return result;
}

Token Parser::scanToken() {
  for (;;) {
    Token token = lexer->scanToken();
    if (token.type != TokenType::ERROR) {
      return token;
    }
    errorHandler->errorAt(token, token.lexeme);
  }
}

void Parser::advance() {
  previous = current;
  if (lookahead.has_value()) {
    current = lookahead.value();
    lookahead.reset();
    if (lookahead2.has_value()) {
      lookahead = lookahead2.value();
      lookahead2.reset();
    }
  } else {
    current = scanToken();
  }
}

bool Parser::peek(TokenType type) {
  if (!lookahead.has_value()) {
    lookahead = scanToken();
  }
  return lookahead->type == type;
}

bool Parser::peek2(TokenType type) {
  if (!lookahead.has_value()) {
    lookahead = scanToken();
  }
  if (!lookahead2.has_value()) {
    lookahead2 = scanToken();
  }
  return lookahead2->type == type;
}

bool Parser::peekAny(std::initializer_list<TokenType> types) {
  if (!lookahead.has_value()) {
    lookahead = scanToken();
  }
  for (TokenType type : types) {
    if (lookahead->type == type) {
      return true;
    }
  }
  return false;
}

bool Parser::looksLikeNextMatchArm() {
  if (checkAny({TokenType::ELSE, TokenType::RIGHT_BRACE})) {
    return true;
  }
  if (checkAny({TokenType::INT, TokenType::FLOAT, TokenType::STRING,
                TokenType::TRUE, TokenType::FALSE, TokenType::IDENTIFIER}) &&
      (peek(TokenType::FAT_ARROW) || peek(TokenType::PIPE))) {
    return true;
  }
  if (check(TokenType::MINUS) && peekAny({TokenType::INT, TokenType::FLOAT}) &&
      (peek2(TokenType::FAT_ARROW) || peek2(TokenType::PIPE))) {
    return true;
  }
  return false;
}

Unique<ast::Declaration> Parser::topDeclaration() {
  ParsedModifiers modifiers = parseModifiers();
  if (match(TokenType::IMPORT)) {
    return importDeclaration(modifiers);
  } else if (match(TokenType::SCRIPT)) {
    return scriptDeclaration(modifiers);
  } else if (match(TokenType::STATE)) {
    return stateDeclaration(modifiers);
  }

  reportOrphanModifiers(modifiers);
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

bool Parser::matchAny(std::initializer_list<TokenType> types) {
  for (auto type : types) {
    if (check(type)) {
      advance();
      return true;
    }
  }
  return false;
}

bool Parser::check(TokenType type) const { return current.type == type; }

bool Parser::checkAny(std::initializer_list<TokenType> types) const {
  for (TokenType type : types) {
    if (check(type)) {
      return true;
    }
  }
  return false;
}

bool Parser::checkModifier() const {
  return checkAny({TokenType::AUTO, TokenType::CONDITIONAL, TokenType::HIDDEN,
                   TokenType::NATIVE, TokenType::STATIC});
}

ParsedModifiers Parser::parseModifiers() {
  ParsedModifiers modifiers;

  while (checkModifier()) {
    bool duplicate = false;
    for (const ParsedModifier& parsed : modifiers) {
      if (parsed.location.type == current.type) {
        errorHandler->errorAt(current, CompilerErrorKind::DuplicateModifier,
                              "Duplicate modifier '{}'.", current.lexeme);
        duplicate = true;
        break;
      }
    }

    if (duplicate) {
      advance();
      continue;
    }

    if (match(TokenType::AUTO)) {
      modifiers.push_back({VellumModifier::Auto, previous});
    } else if (match(TokenType::CONDITIONAL)) {
      modifiers.push_back({VellumModifier::Conditional, previous});
    } else if (match(TokenType::HIDDEN)) {
      modifiers.push_back({VellumModifier::Hidden, previous});
    } else if (match(TokenType::NATIVE)) {
      modifiers.push_back({VellumModifier::Native, previous});
    } else if (match(TokenType::STATIC)) {
      modifiers.push_back({VellumModifier::Static, previous});
    }
  }

  return modifiers;
}

void Parser::reportOrphanModifiers(const ParsedModifiers& modifiers) {
  for (const ParsedModifier& parsed : modifiers) {
    errorHandler->errorAt(parsed.location, CompilerErrorKind::InvalidModifier,
                          "Modifier '{}' is not attached to a declaration.",
                          modifierName(parsed.modifier));
  }
}

Unique<ast::Declaration> Parser::scriptDeclaration(ParsedModifiers modifiers) {
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
        errorHandler->errorAt(e.getToken(), e.getErrorKind(), e.what());
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
      parentScriptNameLocation, std::move(scriptMembers), std::move(modifiers));
}

Unique<ast::Declaration> Parser::importDeclaration(ParsedModifiers modifiers) {
  consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectDeclaration,
          "Import name expected.");
  std::string_view importName = previous.lexeme;
  return makeUnique<ast::ImportDeclaration>(importName, previous,
                                            std::move(modifiers));
}

Unique<ast::Declaration> Parser::stateDeclaration(ParsedModifiers modifiers) {
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
        errorHandler->errorAt(e.getToken(), e.getErrorKind(), e.what());
        synchronizeDeclaration();
      }
    }

    consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
            "Expect '}}' after state declaration.");
  } else if (!check(TokenType::END_OF_FILE) &&
             previous.location.start.line == current.location.start.line) {
    throw ParseException(current, "Unexpected token after state declaration.");
  }

  return makeUnique<ast::StateDeclaration>(stateName, stateNameLocation,
                                           std::move(modifiers),
                                           std::move(stateMembers));
}

Unique<ast::Declaration> Parser::scriptMemberDeclaration() {
  ParsedModifiers modifiers = parseModifiers();

  if (match(TokenType::VAR)) {
    return variableDeclaration(modifiers);
  } else if (match(TokenType::FUN)) {
    return functionDeclaration(FunctionType::Function, modifiers);
  } else if (match(TokenType::EVENT)) {
    return functionDeclaration(FunctionType::Event, modifiers);
  }

  reportOrphanModifiers(modifiers);
  throw ParseException(current,
                       "Expect a declaraion: var, property or fun/event.");
}

Unique<ast::Declaration> Parser::variableDeclaration(
    ParsedModifiers modifiers) {
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

  if (typeName.has_value() && check(TokenType::LEFT_BRACE)) {
    return propertyDeclaration(name, nameLocation, typeName.value(),
                               typeLocation.value(), modifiers);
  }

  Unique<ast::Expression> initializer;
  if (!isArray && match(TokenType::EQUAL)) {
    initializer = unaryNumericToLiteral(expression());
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
      name, typeName, std::move(initializer), nameLocation, typeLocation,
      std::move(modifiers));
}

Unique<ast::Declaration> Parser::functionDeclaration(
    FunctionType functionType, ParsedModifiers modifiers) {
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
      try {
        consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectParamName,
                "Expect a parameter name.");
        std::string_view paramName = previous.lexeme;
        Token paramNameLocation = previous;

        consume(TokenType::COLON, CompilerErrorKind::ExpectColon,
                "Expect ':' after parameter name.");
        const bool paramIsArray = match(TokenType::LEFT_BRACK);
        consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectTypeName,
                "Expect a parameter type.");
        VellumType paramTypeName = VellumType::unresolved(previous.lexeme);
        Token paramTypeLocation = previous;
        if (paramIsArray) {
          consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
                  "Expect ']' after array element type.");
          paramTypeName = VellumType::array(paramTypeName);
        }

        Opt<VellumLiteral> defaultValue = std::nullopt;
        if (match(TokenType::EQUAL)) {
          auto initializer = unaryNumericToLiteral(expression());
          if (!initializer->isLiteralExpression()) {
            throw ParseException(
                current, "Expect a literal value for default argument.");
          }
          defaultValue = initializer->asLiteral().getLiteral();
        }

        parameters.emplace_back(paramName, paramTypeName,
                                std::move(defaultValue), paramNameLocation,
                                paramTypeLocation);
      } catch (const ParseException& e) {
        errorHandler->errorAt(e.getToken(), e.getErrorKind(), e.what());
        synchronizeToRightParen();
        break;
      }
    } while (match(TokenType::COMMA));
  }

  consume(TokenType::RIGHT_PAREN, CompilerErrorKind::ExpectRightParen,
          "Expect ')' after {} declaration.", functionTypeName);

  Opt<Token> returnTypeLocation = std::nullopt;
  auto returnTypeName = VellumType::none();
  if (functionType == FunctionType::Function && match(TokenType::ARROW)) {
    bool isArray = match(TokenType::LEFT_BRACK);

    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectReturnType,
            "Expect a function return type name after '->'.");
    VellumType subtype(VellumType::unresolved(previous.lexeme));
    returnTypeName = isArray ? VellumType::array(subtype) : subtype;
    returnTypeLocation = previous;

    if (isArray) {
      consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
              "']' expected.");
    }
  }

  const bool isEvent = functionType == FunctionType::Event;

  Unique<ast::BlockStatement> body;
  if (modifiersBitmask(modifiers) & VellumModifier::Native) {
    body = makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{});
    if (check(TokenType::LEFT_BRACE)) {
      body = parseBlockStatement();
    }
  } else {
    body = parseBlockStatement();
  }

  return makeUnique<ast::FunctionDeclaration>(
      name, parameters, returnTypeName, std::move(body), std::move(modifiers),
      nameLocation, returnTypeLocation, isEvent);
}

Unique<ast::Declaration> Parser::propertyDeclaration(
    std::string_view name, const Token& nameLocation, const VellumType& type,
    const Token& typeLocation, ParsedModifiers modifiers) {
  consume(TokenType::LEFT_BRACE, CompilerErrorKind::ExpectLeftBrace,
          "Expect '{{' after property type.");

  Opt<Unique<ast::BlockStatement>> getAccessor;
  Opt<Unique<ast::BlockStatement>> setAccessor;

  for (int i = 0; i < 2; ++i) {
    if (match(TokenType::GET)) {
      if (getAccessor.has_value()) {
        errorHandler->errorAt(
            previous,
            std::format("Get accessor for property '{}' already declared.",
                        name));
        continue;
      }
      getAccessor =
          makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{});
      if (check(TokenType::LEFT_BRACE)) {
        getAccessor = parseBlockStatement();
      }
    } else if (match(TokenType::SET)) {
      if (setAccessor.has_value()) {
        errorHandler->errorAt(
            previous,
            std::format("Set accessor for property '{}' already declared.",
                        name));
        continue;
      }
      setAccessor =
          makeUnique<ast::BlockStatement>(Vec<Unique<ast::Statement>>{});
      if (check(TokenType::LEFT_BRACE)) {
        setAccessor = parseBlockStatement();
      }
    }
  }

  consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
          "Expect '}}' after property type.");

  Opt<VellumLiteral> value = std::nullopt;
  if (match(TokenType::EQUAL)) {
    auto expr = unaryNumericToLiteral(expression());
    if (!expr->isLiteralExpression()) {
      throw ParseException(previous,
                           "Expect a literal value for property initializer.");
    }
    value = expr->asLiteral().getLiteral();
  }

  std::string_view documentationString = "";
  return makeUnique<ast::PropertyDeclaration>(
      name, type, documentationString, std::move(getAccessor),
      std::move(setAccessor), value, nameLocation, typeLocation,
      std::move(modifiers));
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
  } else if (match(TokenType::BREAK)) {
    stmt = breakStatement();
  } else if (match(TokenType::CONTINUE)) {
    stmt = continueStatement();
  } else if (match(TokenType::FOR)) {
    stmt = forStatement();
  } else if (match(TokenType::MATCH)) {
    stmt = matchStatement();
  } else {
    stmt = expressionStatement();
  }

  return stmt;
}

Unique<ast::Statement> Parser::ifStatement() {
  auto condition = conditionExpression();
  auto thenBlock = blockStatement();

  Unique<ast::Statement> elseBlock;
  if (match(TokenType::ELSE)) {
    if (match(TokenType::IF)) {
      elseBlock = ifStatement();
    } else {
      elseBlock = blockStatement();
    }
  }

  return makeUnique<ast::IfStatement>(
      std::move(condition), std::move(thenBlock), std::move(elseBlock));
}

Unique<ast::Statement> Parser::returnStatement() {
  const Token returnToken = previous;
  if (canStartExpression(current)) {
    return makeUnique<ast::ReturnStatement>(expression(), returnToken);
  }
  return makeUnique<ast::ReturnStatement>(nullptr, returnToken);
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
  auto condition = conditionExpression();
  return makeUnique<ast::WhileStatement>(std::move(condition),
                                         blockStatement());
}

Unique<ast::Statement> Parser::breakStatement() {
  return makeUnique<ast::BreakStatement>(previous);
}

Unique<ast::Statement> Parser::continueStatement() {
  return makeUnique<ast::ContinueStatement>(previous);
}

Unique<ast::Statement> Parser::forStatement() {
  auto nameExpr = primaryExpression();
  if (!nameExpr->isIdentifierExpression()) {
    throw ParseException(previous, "for loop variable must be an identifier");
  }
  Token nameLocation = previous;

  consume(TokenType::IN, CompilerErrorKind::ExpectIn,
          "Expect 'in' keyword after for loop variable.");

  auto arrayExpr = expression();
  Token arrayLocation = previous;

  return makeUnique<ast::ForStatement>(std::move(nameExpr),
                                       std::move(arrayExpr), blockStatement(),
                                       nameLocation, arrayLocation);
}

Unique<ast::BlockStatement> Parser::parseBlockStatement() {
  consume(TokenType::LEFT_BRACE, CompilerErrorKind::ExpectLeftBrace,
          "Expect '{{'.");

  Vec<Unique<ast::Statement>> block;
  while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::END_OF_FILE)) {
    try {
      block.push_back(statement());
    } catch (const ParseException& e) {
      errorHandler->errorAt(e.getToken(), e.getErrorKind(), e.what());
      synchronizeStatement();
    }
  }

  consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
          "Expect '}}' after code block.");

  return makeUnique<ast::BlockStatement>(std::move(block));
}

Unique<ast::Statement> Parser::blockStatement() {
  return parseBlockStatement();
}

Unique<ast::Statement> Parser::matchArmBody() {
  if (check(TokenType::LEFT_BRACE)) {
    return blockStatement();
  }
  if (match(TokenType::RETURN)) {
    const Token returnToken = previous;
    if (canStartExpression(current) && !looksLikeNextMatchArm()) {
      return makeUnique<ast::ReturnStatement>(expression(), returnToken);
    }
    return makeUnique<ast::ReturnStatement>(nullptr, returnToken);
  }
  if (match(TokenType::BREAK)) {
    return breakStatement();
  }
  if (match(TokenType::CONTINUE)) {
    return continueStatement();
  }
  return expressionStatement();
}

Unique<ast::Statement> Parser::matchStatement() {
  Token matchToken = previous;

  auto scrutinee = logicalOrExpression();

  consume(TokenType::LEFT_BRACE, CompilerErrorKind::ExpectLeftBrace,
          "Expect '{{' after match scrutinee.");

  Vec<ast::MatchArm> arms;
  while (!check(TokenType::ELSE) && !check(TokenType::RIGHT_BRACE)) {
    Vec<Unique<ast::Expression>> patterns;
    do {
      patterns.push_back(patternExpression());
    } while (match(TokenType::PIPE));

    consume(TokenType::FAT_ARROW, CompilerErrorKind::ExpectFatArrow,
            "Expect '=>' after pattern(s).");

    Unique<ast::Statement> body = matchArmBody();

    arms.emplace_back(std::move(patterns), std::move(body));
  }

  Unique<ast::Statement> elseBody;
  if (match(TokenType::ELSE)) {
    consume(TokenType::FAT_ARROW, CompilerErrorKind::ExpectFatArrow,
            "Expect '=>' after else.");

    elseBody = matchArmBody();
  }

  consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
          "Expect '}}' after match statement.");

  return makeUnique<ast::MatchStatement>(std::move(scrutinee), std::move(arms),
                                         std::move(elseBody), matchToken);
}

Unique<ast::Statement> Parser::expressionStatement() {
  return makeUnique<ast::ExpressionStatement>(expression());
}

Unique<ast::Expression> Parser::expression() { return assignExpression(); }

Unique<ast::Expression> Parser::assignExpression() {
  auto expr = logicalOrExpression();

  if (matchAny({TokenType::EQUAL, TokenType::PLUS_EQUAL, TokenType::MINUS_EQUAL,
                TokenType::STAR_EQUAL, TokenType::SLASH_EQUAL,
                TokenType::PERCENT_EQUAL})) {
    ast::AssignOperator op = ast::AssignOperator::Assign;
    switch (previous.type) {
      case TokenType::PLUS_EQUAL:
        op = ast::AssignOperator::Add;
        break;
      case TokenType::MINUS_EQUAL:
        op = ast::AssignOperator::Subtract;
        break;
      case TokenType::STAR_EQUAL:
        op = ast::AssignOperator::Multiply;
        break;
      case TokenType::SLASH_EQUAL:
        op = ast::AssignOperator::Divide;
        break;
      case TokenType::PERCENT_EQUAL:
        op = ast::AssignOperator::Modulo;
        break;
      default:
        break;
    }
    if (expr->isPropertyGetExpression()) {
      ast::PropertyGetExpression& getExpr = expr->asPropertyGet();
      Token location = previous;
      return makeUnique<ast::PropertySetExpression>(
          getExpr.releaseObject(), getExpr.getProperty(), assignExpression(),
          op, getExpr.getLocation(), location);
    } else if (expr->isArrayIndexExpression()) {
      ast::ArrayIndexExpression& indexExpr = expr->asArrayIndex();
      return makeUnique<ast::ArrayIndexSetExpression>(
          indexExpr.releaseArray(), indexExpr.releaseIndex(),
          assignExpression(), op, previous);
    }

    return makeUnique<ast::AssignExpression>(std::move(expr),
                                             assignExpression(), op, previous);
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

  if (match(TokenType::QUES)) {
    return ternaryExpression(std::move(expr));
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

  while (matchAny({TokenType::EQUAL_EQUAL, TokenType::BANG_EQUAL})) {
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

  while (matchAny({TokenType::LESS, TokenType::LESS_EQUAL, TokenType::GREATER,
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

  while (matchAny({TokenType::MINUS, TokenType::PLUS})) {
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

  while (matchAny({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
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
  if (matchAny({TokenType::MINUS, TokenType::BANG})) {
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
        std::move(expr),
        makeUnique<ast::IdentifierExpression>(VellumIdentifier(previous.lexeme),
                                              previous),
        op);
  }

  if (match(TokenType::IS)) {
    const Token op = previous;
    consume(TokenType::IDENTIFIER, CompilerErrorKind::ExpectIsTargetType,
            "Expect a target type for 'is'.");
    return makeUnique<ast::IsExpression>(
        std::move(expr),
        makeUnique<ast::IdentifierExpression>(VellumIdentifier(previous.lexeme),
                                              previous),
        op);
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
    } else if (check(TokenType::LEFT_PAREN)) {
      if (!expr->isIdentifierExpression() && !expr->isPropertyGetExpression()) {
        break;
      }

      match(TokenType::LEFT_PAREN);

      expr = callExpression(std::move(expr), previous);
    } else if (match(TokenType::DOT)) {
      if (!check(TokenType::IDENTIFIER)) {
        break;
      }
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
      arguments.push_back(logicalOrExpression());
    } while (match(TokenType::COMMA));
  }

  consume(TokenType::RIGHT_PAREN, CompilerErrorKind::ExpectRightParen,
          "Expect ')' after function call.");

  return makeUnique<ast::CallExpression>(std::move(callee),
                                         std::move(arguments), location);
}

Unique<ast::Expression> Parser::primaryExpression() {
  if (matchAny({TokenType::INT, TokenType::FLOAT, TokenType::FALSE,
                TokenType::TRUE, TokenType::STRING, TokenType::NONE})) {
    return makeUnique<ast::LiteralExpression>(*previous.value, previous);
  }

  if (match(TokenType::SUPER)) {
    return makeUnique<ast::SuperExpression>(previous);
  }

  if (match(TokenType::SELF)) {
    return makeUnique<ast::SelfExpression>(previous);
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

  if (match(TokenType::INTERP_START)) {
    return interpolatedStringExpression();
  }

  throw ParseException(current, "Expect an expression.");
}

Unique<ast::Expression> Parser::arrayExpression() {
  if (match(TokenType::RIGHT_BRACK)) {
    return makeUnique<ast::NewArrayExpression>(std::nullopt, VellumLiteral(0),
                                               previous);
  }

  if (check(TokenType::IDENTIFIER) && peek(TokenType::SEMICOLON)) {
    advance();  // typename
    const Token subtypeLocation = previous;
    VellumType subtype = VellumType::unresolved(previous.lexeme);

    advance();  // consume ;
    consume(TokenType::INT, CompilerErrorKind::ArrayLengthAsIntExpected,
            "Expect array length as Int value.");
    if (!previous.value.has_value()) {
      throw ParseException(previous, "Array length must be provided.");
    }

    const VellumLiteral length = previous.value.value();
    consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
            "Expect ']' after array.");
    return makeUnique<ast::NewArrayExpression>(subtype, length, previous,
                                               subtypeLocation);
  }

  Vec<Unique<ast::Expression>> elements;

  do {
    elements.push_back(logicalOrExpression());
  } while (match(TokenType::COMMA));

  consume(TokenType::RIGHT_BRACK, CompilerErrorKind::ExpectRightBracket,
          "Expect ']' after array.");
  return makeUnique<ast::NewArrayElementsExpression>(std::move(elements),
                                                     previous);
}

Unique<ast::Expression> Parser::ternaryExpression(
    Unique<ast::Expression> condition) {
  auto location = previous;
  auto left = logicalOrExpression();

  consume(TokenType::COLON, CompilerErrorKind::ExpectColon,
          "Expect ':' between ternary operator operands.");

  auto right = logicalOrExpression();

  return makeUnique<ast::TernaryExpression>(
      std::move(condition), std::move(left), std::move(right), location);
}

Unique<ast::Expression> Parser::unaryNumericToLiteral(
    Unique<ast::Expression> expr) {
  if (!expr->isUnaryExpression()) {
    return expr;
  }

  auto& unary = expr->asUnary();

  if (unary.getOperator() == ast::UnaryExpression::Operator::Negate &&
      unary.getOperand()->isLiteralExpression()) {
    const auto& litExpr = unary.getOperand()->asLiteral();
    const VellumLiteral& lit = litExpr.getLiteral();
    if (lit.getType() == VellumLiteralType::Int) {
      expr = makeUnique<ast::LiteralExpression>(VellumLiteral(-lit.asInt()),
                                                litExpr.getLocation());
    } else if (lit.getType() == VellumLiteralType::Float) {
      expr = makeUnique<ast::LiteralExpression>(VellumLiteral(-lit.asFloat()),
                                                litExpr.getLocation());
    }
  }

  return expr;
}

Unique<ast::Expression> Parser::conditionExpression() {
  return logicalOrExpression();
}

Unique<ast::Expression> Parser::patternExpression() {
  if (match(TokenType::MINUS)) {
    const Token minusToken = previous;
    if (!matchAny({TokenType::INT, TokenType::FLOAT})) {
      throw ParseException(current, CompilerErrorKind::ExpectMatchPattern,
                           "Expect a numeric literal after '-' in pattern.");
    }
    auto literal =
        makeUnique<ast::LiteralExpression>(*previous.value, previous);
    return unaryNumericToLiteral(
        makeUnique<ast::UnaryExpression>(ast::UnaryExpression::Operator::Negate,
                                         std::move(literal), minusToken));
  }

  if (matchAny({TokenType::INT, TokenType::FLOAT, TokenType::FALSE,
                TokenType::TRUE, TokenType::STRING})) {
    return makeUnique<ast::LiteralExpression>(*previous.value, previous);
  }

  if (match(TokenType::IDENTIFIER)) {
    return makeUnique<ast::IdentifierExpression>(
        VellumIdentifier(previous.lexeme), previous);
  }

  throw ParseException(current, CompilerErrorKind::ExpectMatchPattern,
                       "Expect a match pattern.");
}

Unique<ast::Expression> Parser::interpolatedStringExpression() {
  Vec<Unique<ast::Expression>> parts;

  Token location = previous;

  while (!check(TokenType::INTERP_END) && !check(TokenType::END_OF_FILE)) {
    if (match(TokenType::STRING_FRAGMENT)) {
      if (!previous.value) {
        throw ParseException(previous, CompilerErrorKind::UnexpectedToken,
                             "Expect a string fragment value.");
      }
      parts.push_back(
          makeUnique<ast::LiteralExpression>(*previous.value, previous));
    } else if (match(TokenType::LEFT_BRACE)) {
      parts.push_back(logicalOrExpression());
      consume(TokenType::RIGHT_BRACE, CompilerErrorKind::ExpectRightBrace,
              "Expect '}}' after interpolated expression.");
    } else {
      throw ParseException(
          current, CompilerErrorKind::UnexpectedToken,
          "Expect a string fragment or '{{' in interpolated string.");
    }
  }

  consume(TokenType::INTERP_END, CompilerErrorKind::UnexpectedToken,
          "Expect '\"' to close interpolated string.");

  return makeUnique<ast::InterpolatedStringExpression>(std::move(parts),
                                                       location);
}

bool Parser::canStartExpression(const Token& token) {
  (void)token;
  return checkAny({TokenType::INT, TokenType::FLOAT, TokenType::FALSE,
                   TokenType::TRUE, TokenType::STRING, TokenType::NONE,
                   TokenType::MINUS, TokenType::BANG, TokenType::IDENTIFIER,
                   TokenType::SUPER, TokenType::SELF, TokenType::LEFT_PAREN,
                   TokenType::LEFT_BRACK, TokenType::INTERP_START});
}

void Parser::synchronizeTopDeclaration() {
  errorHandler->disablePanicMode();
  while (!check(TokenType::END_OF_FILE)) {
    if (checkModifier()) {
      return;
    }
    switch (current.type) {
      case TokenType::IMPORT:
      case TokenType::SCRIPT:
      case TokenType::STATE:
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
    if (checkModifier()) {
      return;
    }
    switch (current.type) {
      case TokenType::FUN:
      case TokenType::EVENT:
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
      case TokenType::WHILE:
      case TokenType::RETURN:
      case TokenType::MATCH:
      case TokenType::RIGHT_BRACE:
        return;
      default:
        break;
    }
    advance();
  }
}

void Parser::synchronizeToRightParen() {
  errorHandler->disablePanicMode();
  while (!check(TokenType::RIGHT_PAREN) && !check(TokenType::END_OF_FILE)) {
    advance();
  }
}

}  // namespace vellum