#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <vector>

#include "lexer/lexer.h"
#include "utils.h"

using namespace vellum;

TEST_CASE("LexerScriptTest") {
  std::vector<Token> expected{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "name", VellumIdentifier("name")),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "parent", VellumIdentifier("parent")),
  };

  std::string_view source = "script name : parent";
  CHECK_THAT(scanTokens(std::make_unique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerVarDeclarationTest") {
  std::vector<Token> expected{
      makeToken(TokenType::VAR, 1, "var"),
      makeToken(TokenType::IDENTIFIER, 1, "number", VellumIdentifier("number")),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "Int", VellumIdentifier("Int")),
      makeToken(TokenType::EQUAL, 1, "="),
      makeToken(TokenType::INT, 1, "42", VellumValue(42))};

  std::string_view source = "var number: Int = 42";
  CHECK_THAT(scanTokens(std::make_unique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}