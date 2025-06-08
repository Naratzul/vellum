#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <vector>

#include "lexer/lexer.h"
#include "utils.h"

using namespace vellum;

TEST_CASE("LexerScriptTest") {
  std::vector<Token> expected{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "name"),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "parent"),
  };

  std::string_view source = "script name : parent";
  CHECK_THAT(scanTokens(std::make_unique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerVarDeclarationTest") {
  std::vector<Token> expected{
      makeToken(TokenType::VAR, 1, "var"),
      makeToken(TokenType::IDENTIFIER, 1, "number"),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::EQUAL, 1, "="),
      makeToken(TokenType::INT, 1, "42", VellumLiteral(42))};

  std::string_view source = "var number: Int = 42";
  CHECK_THAT(scanTokens(std::make_unique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_NoParams_NoReturn") {
  std::vector<Token> expected{
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{")
      // ... function body tokens would follow
  };
  std::string_view source = "fun foo() {";
  CHECK_THAT(scanTokens(std::make_unique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_Params_NoReturn") {
  std::vector<Token> expected{makeToken(TokenType::FUN, 1, "fun"),
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
                              makeToken(TokenType::LEFT_BRACE, 1, "{")};
  std::string_view source = "fun bar(x: Int, y: Float) {";
  CHECK_THAT(scanTokens(std::make_unique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_NoParams_WithReturn") {
  std::vector<Token> expected{makeToken(TokenType::FUN, 1, "fun"),
                              makeToken(TokenType::IDENTIFIER, 1, "baz"),
                              makeToken(TokenType::LEFT_PAREN, 1, "("),
                              makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                              makeToken(TokenType::ARROW, 1, "->"),
                              makeToken(TokenType::IDENTIFIER, 1, "String"),
                              makeToken(TokenType::LEFT_BRACE, 1, "{")};
  std::string_view source = "fun baz() -> String {";
  CHECK_THAT(scanTokens(std::make_unique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_Params_WithReturn") {
  std::vector<Token> expected{makeToken(TokenType::FUN, 1, "fun"),
                              makeToken(TokenType::IDENTIFIER, 1, "qux"),
                              makeToken(TokenType::LEFT_PAREN, 1, "("),
                              makeToken(TokenType::IDENTIFIER, 1, "a"),
                              makeToken(TokenType::COLON, 1, ":"),
                              makeToken(TokenType::IDENTIFIER, 1, "Bool"),
                              makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                              makeToken(TokenType::ARROW, 1, "->"),
                              makeToken(TokenType::IDENTIFIER, 1, "Int"),
                              makeToken(TokenType::LEFT_BRACE, 1, "{")};
  std::string_view source = "fun qux(a: Bool) -> Int {";
  CHECK_THAT(scanTokens(std::make_unique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_MultipleParams_MixedTypes") {
  std::vector<Token> expected{makeToken(TokenType::FUN, 1, "fun"),
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
                              makeToken(TokenType::LEFT_BRACE, 1, "{")};
  std::string_view source = "fun mix(x: Int, flag: Bool, name: String) {";
  CHECK_THAT(scanTokens(std::make_unique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}