#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "common/types.h"
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

  ast::ScriptDeclaration expected("TestScript", tokens[1], "ParentScript",
                                  tokens[3], {});
  CHECK(expected == *result.declarations[0]);
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

  ast::ScriptDeclaration expected("TestScript", tokens[1], {}, {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
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
      ast::FunctionBody{}));

  ast::ScriptDeclaration expected("TestScript", tokens[1], {}, {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
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
      "bar", params, VellumType::none(),
      ast::FunctionBody{}));

  ast::ScriptDeclaration expected("TestScript", tokens[1], {}, {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
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
      ast::FunctionBody{}));

  ast::ScriptDeclaration expected("TestScript", tokens[1], {}, {},
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
      "qux", params, VellumType::unresolved("Int"),
      ast::FunctionBody{}));

  ast::ScriptDeclaration expected("TestScript", tokens[1], {}, {},
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
      "mix", params, VellumType::none(), ast::FunctionBody{}));

  ast::ScriptDeclaration expected("TestScript", tokens[1], {}, {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
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
      std::move(expected_body)));

  ast::ScriptDeclaration expected("TestScript", tokens[1], {}, {},
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
      std::move(expected_body)));

  ast::ScriptDeclaration expected("TestScript", tokens[1], {}, {},
                                  std::move(members));

  CHECK(expected == *result.declarations[0]);
}