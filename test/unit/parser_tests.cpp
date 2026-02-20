#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "ast/decl/declaration.h"
#include "ast/expression/expression.h"
#include "common/types.h"
#include "lexer/token.h"
#include "mock/mock_lexer.h"
#include "parser/parser.h"
#include "utils.h"

using namespace vellum;
using common::makeShared;
using common::makeUnique;
using common::Shared;
using common::Unique;
using common::Vec;

TEST_CASE("ParserScriptDeclarationTest") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "ParentScript"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::ScriptDeclaration expected(
      VellumType::identifier("TestScript"), tokens[1],
      VellumType::identifier("ParentScript"), tokens[3], {});
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserSuperCallInFunction") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::SUPER, 1, "super"),
                    makeToken(TokenType::DOT, 1, "."),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody().size() == 1);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* call =
      dynamic_cast<const ast::CallExpression*>(exprStmt->getExpression().get());
  REQUIRE(call != nullptr);
  REQUIRE(call->getCallee()->isPropertyGetExpression());
  REQUIRE(call->getCallee()->asPropertyGet().getObject()->isSuperExpression());
  REQUIRE(call->getCallee()->asPropertyGet().getProperty().toString() == "foo");
}

TEST_CASE("ParserSelfCallInFunction") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::SELF, 1, "self"),
                    makeToken(TokenType::DOT, 1, "."),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody().size() == 1);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* call =
      dynamic_cast<const ast::CallExpression*>(exprStmt->getExpression().get());
  REQUIRE(call != nullptr);
  REQUIRE(call->getCallee()->isPropertyGetExpression());
  REQUIRE(call->getCallee()->asPropertyGet().getObject()->isSelfExpression());
  REQUIRE(call->getCallee()->asPropertyGet().getProperty().toString() == "foo");
}

TEST_CASE("ParserGlobalVarDeclarationTest") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "number"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::GlobalVariableDeclaration>(
      "number", VellumType::unresolved("Int"),
      makeUnique<ast::LiteralExpression>(VellumLiteral(42))));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserGlobalVarDeclaration_NegativeInitializer") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "num"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::MINUS, 1, "-"),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* varDecl = dynamic_cast<const ast::GlobalVariableDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(varDecl != nullptr);
  REQUIRE(varDecl->initializer() != nullptr);
  REQUIRE(varDecl->initializer()->isLiteralExpression());
  REQUIRE(varDecl->initializer()->asLiteral().getLiteral().asInt() == -42);
}

TEST_CASE("ParserFunctionDeclaration_NoParams_NoReturn") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "foo", Vec<ast::FunctionParameter>{}, VellumType::none(),
      ast::FunctionBody{}, false));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserBreakInsideWhile") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::WHILE, 1, "while"),
                    makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::BREAK, 1, "break"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody().size() == 1);
  const auto* whileStmt = dynamic_cast<const ast::WhileStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(whileStmt != nullptr);
  REQUIRE(whileStmt->getBody().size() == 1);
  REQUIRE(dynamic_cast<const ast::BreakStatement*>(whileStmt->getBody()[0].get()) !=
          nullptr);
}

TEST_CASE("ParserContinueInsideWhile") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::WHILE, 1, "while"),
                    makeToken(TokenType::TRUE, 1, "true", VellumLiteral(true)),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::CONTINUE, 1, "continue"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody().size() == 1);
  const auto* whileStmt = dynamic_cast<const ast::WhileStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(whileStmt != nullptr);
  REQUIRE(whileStmt->getBody().size() == 1);
  REQUIRE(
      dynamic_cast<const ast::ContinueStatement*>(whileStmt->getBody()[0].get()) !=
      nullptr);
}

TEST_CASE("ParserPropertyDeclaration_NoInitializer") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::SET, 1, "set"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* propDecl = dynamic_cast<const ast::PropertyDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(propDecl != nullptr);
  REQUIRE(propDecl->getName() == "x");
  REQUIRE(propDecl->getTypeName().toString() == "Int");
  REQUIRE_FALSE(propDecl->getDefaultValue().has_value());
  REQUIRE(propDecl->getGetAccessor().has_value());
  REQUIRE(propDecl->getSetAccessor().has_value());
  REQUIRE(propDecl->getGetAccessor()->empty());
  REQUIRE(propDecl->getSetAccessor()->empty());
}

TEST_CASE("ParserPropertyDeclaration_WithIntInitializer") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::SET, 1, "set"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* propDecl = dynamic_cast<const ast::PropertyDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(propDecl != nullptr);
  REQUIRE(propDecl->getDefaultValue().has_value());
  REQUIRE(propDecl->getDefaultValue()->getType() == VellumLiteralType::Int);
  REQUIRE(propDecl->getDefaultValue()->asInt() == 42);
}

TEST_CASE("ParserPropertyDeclaration_WithNoneInitializer") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "obj"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "SomeScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::SET, 1, "set"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::NONE, 1, "None", VellumLiteral()),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* propDecl = dynamic_cast<const ast::PropertyDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(propDecl != nullptr);
  REQUIRE(propDecl->getDefaultValue().has_value());
  REQUIRE(propDecl->getDefaultValue()->getType() == VellumLiteralType::None);
}

TEST_CASE("ParserPropertyDeclaration_ReadonlyAuto") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* propDecl = dynamic_cast<const ast::PropertyDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(propDecl != nullptr);
  REQUIRE(propDecl->isReadonly());
  REQUIRE(propDecl->getGetAccessor().has_value());
  REQUIRE_FALSE(propDecl->getSetAccessor().has_value());
}

TEST_CASE("ParserPropertyDeclaration_InitializerMustBeLiteral") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::VAR, 1, "var"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::GET, 1, "get"),
                    makeToken(TokenType::SET, 1, "set"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE(errorHandler->hadError());
}

TEST_CASE("ParserFunctionDeclaration_Params_NoReturn") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "bar"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::IDENTIFIER, 1, "y"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Float"),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<ast::FunctionParameter> params = {{"x", VellumType::unresolved("Int")},
                                        {"y", VellumType::unresolved("Float")}};

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "bar", params, VellumType::none(), ast::FunctionBody{}, false));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_ParamWithDefaultValue") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "5", VellumLiteral(5)),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getParameters().size() == 1);
  REQUIRE(funcDecl->getParameters()[0].defaultValue.has_value());
  REQUIRE(funcDecl->getParameters()[0].defaultValue->getType() ==
          VellumLiteralType::Int);
  REQUIRE(funcDecl->getParameters()[0].defaultValue->asInt() == 5);
}

TEST_CASE("ParserFunctionDeclaration_NegativeDefaultValue") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::MINUS, 1, "-"),
                    makeToken(TokenType::INT, 1, "5", VellumLiteral(5)),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getParameters().size() == 1);
  REQUIRE(funcDecl->getParameters()[0].defaultValue.has_value());
  REQUIRE(funcDecl->getParameters()[0].defaultValue->getType() ==
          VellumLiteralType::Int);
  REQUIRE(funcDecl->getParameters()[0].defaultValue->asInt() == -5);
}

TEST_CASE("ParserFunctionDeclaration_MultipleParamsWithTrailingDefault") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "bar"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "a"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::IDENTIFIER, 1, "b"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "String"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::STRING, 1, "hi",
                             VellumLiteral(std::string_view("hi"))),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getParameters().size() == 2);
  REQUIRE_FALSE(funcDecl->getParameters()[0].defaultValue.has_value());
  REQUIRE(funcDecl->getParameters()[1].defaultValue.has_value());
  REQUIRE(funcDecl->getParameters()[1].defaultValue->getType() ==
          VellumLiteralType::String);
  REQUIRE(funcDecl->getParameters()[1].defaultValue->asString() == "hi");
}

TEST_CASE("ParserFunctionDeclaration_NoParams_WithReturn") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "baz"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::ARROW, 1, "->"),
                    makeToken(TokenType::IDENTIFIER, 1, "String"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "baz", Vec<ast::FunctionParameter>{}, VellumType::unresolved("String"),
      ast::FunctionBody{}, false));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_Params_WithReturn") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "qux"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "a"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Bool"),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::ARROW, 1, "->"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<ast::FunctionParameter> params = {{"a", VellumType::unresolved("Bool")}};

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "qux", params, VellumType::unresolved("Int"), ast::FunctionBody{},
      false));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_MultipleParams_MixedTypes") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "mix"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Int"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::IDENTIFIER, 1, "flag"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "Bool"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::IDENTIFIER, 1, "name"),
                    makeToken(TokenType::COLON, 1, ":"),
                    makeToken(TokenType::IDENTIFIER, 1, "String"),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  Vec<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"flag", VellumType::unresolved("Bool")},
      {"name", VellumType::unresolved("String")}};

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "mix", params, VellumType::none(), ast::FunctionBody{}, false));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserArrayIndexExpression") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "arr"),
                    makeToken(TokenType::LEFT_BRACK, 1, "["),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACK, 1, "]"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody().size() == 1);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* indexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(indexExpr != nullptr);
}

TEST_CASE("ParserArrayIndexAssignment") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "arr"),
                    makeToken(TokenType::LEFT_BRACK, 1, "["),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACK, 1, "]"),
                    makeToken(TokenType::EQUAL, 1, "="),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody().size() == 1);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* setExpr = dynamic_cast<const ast::ArrayIndexSetExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(setExpr != nullptr);
}

TEST_CASE("ParserComposedAssign_Identifier") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "x"),
                    makeToken(TokenType::PLUS_EQUAL, 1, "+="),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* assignExpr = dynamic_cast<const ast::AssignExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(assignExpr != nullptr);
  REQUIRE(assignExpr->getOperator() == ast::AssignOperator::Add);
}

TEST_CASE("ParserComposedAssign_Property") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "obj"),
                    makeToken(TokenType::DOT, 1, "."),
                    makeToken(TokenType::IDENTIFIER, 1, "prop"),
                    makeToken(TokenType::MINUS_EQUAL, 1, "-="),
                    makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* setExpr = dynamic_cast<const ast::PropertySetExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(setExpr != nullptr);
  REQUIRE(setExpr->getOperator() == ast::AssignOperator::Subtract);
}

TEST_CASE("ParserComposedAssign_ArrayIndex") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "arr"),
                    makeToken(TokenType::LEFT_BRACK, 1, "["),
                    makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
                    makeToken(TokenType::RIGHT_BRACK, 1, "]"),
                    makeToken(TokenType::STAR_EQUAL, 1, "*="),
                    makeToken(TokenType::INT, 1, "2", VellumLiteral(2)),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);
  const auto* setExpr = dynamic_cast<const ast::ArrayIndexSetExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(setExpr != nullptr);
  REQUIRE(setExpr->getOperator() == ast::AssignOperator::Multiply);
}

TEST_CASE("ParserCall_NoArgs") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  auto expected_call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      Vec<Unique<ast::Expression>>{});
  auto expected_body = Vec<Unique<ast::Statement>>{};
  expected_body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(expected_call_expr)));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(expected_body), false));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserCall_WithArgs") {
  Vec<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                    makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::FUN, 1, "fun"),
                    makeToken(TokenType::IDENTIFIER, 1, "test"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::LEFT_BRACE, 1, "{"),
                    makeToken(TokenType::IDENTIFIER, 1, "foo"),
                    makeToken(TokenType::LEFT_PAREN, 1, "("),
                    makeToken(TokenType::IDENTIFIER, 1, "bar"),
                    makeToken(TokenType::COMMA, 1, ","),
                    makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
                    makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                    makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  auto args = Vec<Unique<ast::Expression>>{};
  args.emplace_back(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("bar")));
  args.emplace_back(makeUnique<ast::LiteralExpression>(VellumLiteral(42)));

  auto expected_call_expr = makeUnique<ast::CallExpression>(
      makeUnique<ast::IdentifierExpression>(VellumIdentifier("foo")),
      std::move(args));

  auto expected_body = Vec<Unique<ast::Statement>>{};
  expected_body.emplace_back(
      makeUnique<ast::ExpressionStatement>(std::move(expected_call_expr)));

  Vec<Unique<ast::Declaration>> members;
  members.push_back(makeUnique<ast::FunctionDeclaration>(
      "test", Vec<ast::FunctionParameter>{}, VellumType::none(),
      std::move(expected_body), false));

  ast::ScriptDeclaration expected(VellumType::identifier("TestScript"),
                                  tokens[1], VellumType::none(), {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserArrayIndex_LiteralIndex") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "test"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "arr"),
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);

  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody().size() == 1);

  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);

  const auto* indexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(indexExpr != nullptr);
  REQUIRE(indexExpr->getArray()->isIdentifierExpression());
  REQUIRE(indexExpr->getArray()->asIdentifier().getIdentifier().toString() ==
          "arr");
  REQUIRE(indexExpr->getIndex()->isLiteralExpression());
  REQUIRE(indexExpr->getIndex()->asLiteral().getLiteral().asInt() == 0);
}

TEST_CASE("ParserArrayIndex_Nested") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "test"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "arr"),
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);

  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody().size() == 1);

  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);

  const auto* outerIndexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      exprStmt->getExpression().get());
  REQUIRE(outerIndexExpr != nullptr);

  const auto* innerIndexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      outerIndexExpr->getArray().get());
  REQUIRE(innerIndexExpr != nullptr);

  REQUIRE(innerIndexExpr->getArray()->isIdentifierExpression());
  REQUIRE(
      innerIndexExpr->getArray()->asIdentifier().getIdentifier().toString() ==
      "arr");

  REQUIRE(innerIndexExpr->getIndex()->isLiteralExpression());
  REQUIRE(innerIndexExpr->getIndex()->asLiteral().getLiteral().asInt() == 0);

  REQUIRE(outerIndexExpr->getIndex()->isLiteralExpression());
  REQUIRE(outerIndexExpr->getIndex()->asLiteral().getLiteral().asInt() == 1);
}

TEST_CASE("ParserArrayIndex_Precedence_IndexBeforePropertyAndCall") {
  Vec<Token> tokens{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "TestScript"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "test"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "arr"),
      makeToken(TokenType::LEFT_BRACK, 1, "["),
      makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
      makeToken(TokenType::RIGHT_BRACK, 1, "]"),
      makeToken(TokenType::DOT, 1, "."),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::END_OF_FILE, 1, ""),
  };

  auto errorHandler = makeShared<CompilerErrorHandler>();
  const auto result =
      Parser(makeUnique<LexerMock>(tokens), errorHandler).parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  const auto* scriptDecl =
      dynamic_cast<const ast::ScriptDeclaration*>(result.declarations[0].get());
  REQUIRE(scriptDecl != nullptr);
  REQUIRE(scriptDecl->getMemberDecls().size() == 1);

  const auto* funcDecl = dynamic_cast<const ast::FunctionDeclaration*>(
      scriptDecl->getMemberDecls()[0].get());
  REQUIRE(funcDecl != nullptr);
  REQUIRE(funcDecl->getBody().size() == 1);

  const auto* exprStmt = dynamic_cast<const ast::ExpressionStatement*>(
      funcDecl->getBody()[0].get());
  REQUIRE(exprStmt != nullptr);

  const auto* callExpr =
      dynamic_cast<const ast::CallExpression*>(exprStmt->getExpression().get());
  REQUIRE(callExpr != nullptr);

  const auto* propertyExpr = dynamic_cast<const ast::PropertyGetExpression*>(
      callExpr->getCallee().get());
  REQUIRE(propertyExpr != nullptr);

  REQUIRE(propertyExpr->getProperty().toString() == "foo");

  const auto* indexExpr = dynamic_cast<const ast::ArrayIndexExpression*>(
      propertyExpr->getObject().get());
  REQUIRE(indexExpr != nullptr);

  REQUIRE(indexExpr->getArray()->isIdentifierExpression());
  REQUIRE(indexExpr->getArray()->asIdentifier().getIdentifier().toString() ==
          "arr");

  REQUIRE(indexExpr->getIndex()->isLiteralExpression());
  REQUIRE(indexExpr->getIndex()->asLiteral().getLiteral().asInt() == 0);
}