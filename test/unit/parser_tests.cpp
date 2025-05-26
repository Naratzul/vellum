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