#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <memory>
#include <vector>

#include "mock/mock_lexer.h"
#include "parser/parser.h"
#include "utils.h"

using namespace vellum;

TEST_CASE("ParserScriptDeclarationTest") {
  std::vector<Token> tokens{makeToken(TokenType::SCRIPT, 1, "script"),
                            makeToken(TokenType::IDENTIFIER, 1, "name"),
                            makeToken(TokenType::COLON, 1, ":"),
                            makeToken(TokenType::IDENTIFIER, 1, "parent"),
                            makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  const auto result =
      Parser(std::make_unique<LexerMock>(std::move(tokens)), errorHandler)
          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::ScriptDeclaration expected("name", "parent");
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserGlobalVarDeclarationTest") {
  std::vector<Token> tokens{
      makeToken(TokenType::VAR, 1, "var"),
      makeToken(TokenType::IDENTIFIER, 1, "number"),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::EQUAL, 1, "="),
      makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
      makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  const auto result =
      Parser(std::make_unique<LexerMock>(std::move(tokens)), errorHandler)
          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::GlobalVariableDeclaration expected(
      "number", VellumType::unresolved("Int"),
      std::make_unique<ast::LiteralExpression>(VellumLiteral(42)));
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_NoParams_NoReturn") {
  std::vector<Token> tokens{makeToken(TokenType::FUN, 1, "fun"),
                            makeToken(TokenType::IDENTIFIER, 1, "foo"),
                            makeToken(TokenType::LEFT_PAREN, 1, "("),
                            makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                            makeToken(TokenType::LEFT_BRACE, 1, "{"),
                            makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                            makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  const auto result =
      Parser(std::make_unique<LexerMock>(std::move(tokens)), errorHandler)
          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::FunctionDeclaration expected("foo", {}, VellumType::none(),
                                    ast::FunctionBody{});
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_Params_NoReturn") {
  std::vector<Token> tokens{makeToken(TokenType::FUN, 1, "fun"),
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
                            makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  const auto result =
      Parser(std::make_unique<LexerMock>(std::move(tokens)), errorHandler)
          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  std::vector<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"y", VellumType::unresolved("Float")}};
  ast::FunctionDeclaration expected("bar", params, VellumType::none(),
                                    ast::FunctionBody{});
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_NoParams_WithReturn") {
  std::vector<Token> tokens{makeToken(TokenType::FUN, 1, "fun"),
                            makeToken(TokenType::IDENTIFIER, 1, "baz"),
                            makeToken(TokenType::LEFT_PAREN, 1, "("),
                            makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                            makeToken(TokenType::ARROW, 1, "->"),
                            makeToken(TokenType::IDENTIFIER, 1, "String"),
                            makeToken(TokenType::LEFT_BRACE, 1, "{"),
                            makeToken(TokenType::RIGHT_BRACE, 1, "}"),
                            makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  const auto result =
      Parser(std::make_unique<LexerMock>(std::move(tokens)), errorHandler)
          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  ast::FunctionDeclaration expected("baz", {}, VellumType::unresolved("String"),
                                    ast::FunctionBody{});
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_Params_WithReturn") {
  std::vector<Token> tokens{makeToken(TokenType::FUN, 1, "fun"),
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
                            makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  const auto result =
      Parser(std::make_unique<LexerMock>(std::move(tokens)), errorHandler)
          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  std::vector<ast::FunctionParameter> params = {
      {"a", VellumType::unresolved("Bool")}};
  ast::FunctionDeclaration expected(
      "qux", params, VellumType::unresolved("Int"), ast::FunctionBody{});
  CHECK(expected == *result.declarations[0]);
}

TEST_CASE("ParserFunctionDeclaration_MultipleParams_MixedTypes") {
  std::vector<Token> tokens{makeToken(TokenType::FUN, 1, "fun"),
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
                            makeToken(TokenType::END_OF_FILE, 1, "")};

  auto errorHandler = std::make_shared<CompilerErrorHandler>();
  const auto result =
      Parser(std::make_unique<LexerMock>(std::move(tokens)), errorHandler)
          .parse();

  REQUIRE_FALSE(errorHandler->hadError());
  REQUIRE(result.declarations.size() == 1);

  std::vector<ast::FunctionParameter> params = {
      {"x", VellumType::unresolved("Int")},
      {"flag", VellumType::unresolved("Bool")},
      {"name", VellumType::unresolved("String")}};
  ast::FunctionDeclaration expected("mix", params, VellumType::none(),
                                    ast::FunctionBody{});
  CHECK(expected == *result.declarations[0]);
}