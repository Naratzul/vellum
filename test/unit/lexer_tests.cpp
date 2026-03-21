#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "common/types.h"
#include "lexer/lexer.h"
#include "utils.h"

using namespace vellum;
using common::makeUnique;
using common::Unique;
using common::Vec;

TEST_CASE("LexerScriptTest") {
  Vec<Token> expected{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "name"),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "parent"),
  };

  std::string_view source = "script name : parent";
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerSelfKeyword") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("self"));
  REQUIRE(tokens.size() >= 1);
  CHECK(tokens[0].type == TokenType::SELF);
  CHECK(tokens[0].lexeme == "self");
}

TEST_CASE("LexerBreakKeyword") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("break"));
  REQUIRE(tokens.size() >= 1);
  CHECK(tokens[0].type == TokenType::BREAK);
  CHECK(tokens[0].lexeme == "break");
}

TEST_CASE("LexerContinueKeyword") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("continue"));
  REQUIRE(tokens.size() >= 1);
  CHECK(tokens[0].type == TokenType::CONTINUE);
  CHECK(tokens[0].lexeme == "continue");
}

TEST_CASE("LexerForKeyword") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("for"));
  REQUIRE(tokens.size() >= 1);
  CHECK(tokens[0].type == TokenType::FOR);
  CHECK(tokens[0].lexeme == "for");
}

TEST_CASE("LexerInKeyword") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("in"));
  REQUIRE(tokens.size() >= 1);
  CHECK(tokens[0].type == TokenType::IN);
  CHECK(tokens[0].lexeme == "in");
}

TEST_CASE("LexerVarDeclarationTest") {
  Vec<Token> expected{
      makeToken(TokenType::VAR, 1, "var"),
      makeToken(TokenType::IDENTIFIER, 1, "number"),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::EQUAL, 1, "="),
      makeToken(TokenType::INT, 1, "42", VellumLiteral(42))};

  std::string_view source = "var number: Int = 42";
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_NoParams_NoReturn") {
  Vec<Token> expected{
      makeToken(TokenType::FUN, 1, "fun"),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::LEFT_BRACE, 1, "{")
      // ... function body tokens would follow
  };
  std::string_view source = "fun foo() {";
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_Params_NoReturn") {
  Vec<Token> expected{makeToken(TokenType::FUN, 1, "fun"),
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
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_NoParams_WithReturn") {
  Vec<Token> expected{makeToken(TokenType::FUN, 1, "fun"),
                              makeToken(TokenType::IDENTIFIER, 1, "baz"),
                              makeToken(TokenType::LEFT_PAREN, 1, "("),
                              makeToken(TokenType::RIGHT_PAREN, 1, ")"),
                              makeToken(TokenType::ARROW, 1, "->"),
                              makeToken(TokenType::IDENTIFIER, 1, "String"),
                              makeToken(TokenType::LEFT_BRACE, 1, "{")};
  std::string_view source = "fun baz() -> String {";
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_Params_WithReturn") {
  Vec<Token> expected{makeToken(TokenType::FUN, 1, "fun"),
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
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFunctionDeclaration_MultipleParams_MixedTypes") {
  Vec<Token> expected{makeToken(TokenType::FUN, 1, "fun"),
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
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerCall_NoArgs") {
  Vec<Token> expected{
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
  };
  std::string_view source = "foo()";
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerCall_WithArgs") {
  Vec<Token> expected{
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::IDENTIFIER, 1, "bar"),
      makeToken(TokenType::COMMA, 1, ","),
      makeToken(TokenType::INT, 1, "42", VellumLiteral(42)),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
  };
  std::string_view source = "foo(bar, 42)";
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerComposedAssignmentTokens") {
  Vec<Token> expected{
      makeToken(TokenType::IDENTIFIER, 1, "x"),
      makeToken(TokenType::PLUS_EQUAL, 1, "+="),
      makeToken(TokenType::IDENTIFIER, 1, "a"),
      makeToken(TokenType::MINUS_EQUAL, 1, "-="),
      makeToken(TokenType::IDENTIFIER, 1, "b"),
      makeToken(TokenType::STAR_EQUAL, 1, "*="),
      makeToken(TokenType::IDENTIFIER, 1, "c"),
      makeToken(TokenType::SLASH_EQUAL, 1, "/="),
      makeToken(TokenType::IDENTIFIER, 1, "d"),
      makeToken(TokenType::PERCENT_EQUAL, 1, "%="),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
  };
  std::string_view source = "x += a -= b *= c /= d %= 1";
  CHECK_THAT(scanTokens(makeUnique<Lexer>(source)),
             Catch::Matchers::Equals(expected));
}