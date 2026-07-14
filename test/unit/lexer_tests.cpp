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

TEST_CASE("LexerHiddenKeyword") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("hidden"));
  REQUIRE(tokens.size() >= 1);
  CHECK(tokens[0].type == TokenType::HIDDEN);
  CHECK(tokens[0].lexeme == "hidden");
}

TEST_CASE("LexerConditionalKeyword") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("conditional"));
  REQUIRE(tokens.size() >= 1);
  CHECK(tokens[0].type == TokenType::CONDITIONAL);
  CHECK(tokens[0].lexeme == "conditional");
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

TEST_CASE("LexerColumnZeroTokenPosition") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("import Utility"));
  REQUIRE(tokens.size() >= 2);
  CHECK(tokens[0].location.start.position == 0);
  CHECK(tokens[1].location.start.position == 7);
}

TEST_CASE("LexerQuesToken") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("?"));
  REQUIRE_FALSE(tokens.empty());
  CHECK(tokens[0].type == TokenType::QUES);
  CHECK(tokens[0].lexeme == "?");
}

TEST_CASE("LexerQuesBetweenIdentifiers") {
  Vec<Token> expected{
      makeToken(TokenType::IDENTIFIER, 1, "x"),
      makeToken(TokenType::QUES, 1, "?"),
      makeToken(TokenType::IDENTIFIER, 1, "y"),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("x ? y")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerFatArrowToken") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("=>"));
  REQUIRE(tokens.size() >= 1);
  CHECK(tokens[0].type == TokenType::FAT_ARROW);
  CHECK(tokens[0].lexeme == "=>");
}

TEST_CASE("LexerPipeToken") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("|"));
  REQUIRE(tokens.size() >= 1);
  CHECK(tokens[0].type == TokenType::PIPE);
  CHECK(tokens[0].lexeme == "|");
}

TEST_CASE("LexerMatchSyntaxTokens") {
  Vec<Token> expected{
      makeToken(TokenType::MATCH, 1, "match"),
      makeToken(TokenType::IDENTIFIER, 1, "x"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::PIPE, 1, "|"),
      makeToken(TokenType::INT, 1, "2", VellumLiteral(2)),
      makeToken(TokenType::FAT_ARROW, 1, "=>"),
      makeToken(TokenType::IDENTIFIER, 1, "foo"),
      makeToken(TokenType::LEFT_PAREN, 1, "("),
      makeToken(TokenType::RIGHT_PAREN, 1, ")"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("match x { 1 | 2 => foo() }")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerOrAndArrowAndGreaterEqualStillWork") {
  Vec<Token> expected{
      makeToken(TokenType::IDENTIFIER, 1, "a"),
      makeToken(TokenType::OR, 1, "||"),
      makeToken(TokenType::IDENTIFIER, 1, "b"),
      makeToken(TokenType::ARROW, 1, "->"),
      makeToken(TokenType::IDENTIFIER, 1, "Int"),
      makeToken(TokenType::GREATER_EQUAL, 1, ">="),
      makeToken(TokenType::INT, 1, "0", VellumLiteral(0)),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("a || b -> Int >= 0")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_Empty") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\"\"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_LiteralOnly") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::STRING_FRAGMENT, 1, "hello", VellumLiteral(std::string_view("hello"))),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\"hello\"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_PreservesSpaces") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::STRING_FRAGMENT, 1, " a b ", VellumLiteral(std::string_view(" a b "))),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\" a b \"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_SingleHole") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::STRING_FRAGMENT, 1, "Hi ", VellumLiteral(std::string_view("Hi "))),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "name"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::STRING_FRAGMENT, 1, "!", VellumLiteral(std::string_view("!"))),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\"Hi {name}!\"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_HoleOnly") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "x"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\"{x}\"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_EscapedBraces") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::STRING_FRAGMENT, 1, "{{x}}", VellumLiteral(std::string_view("{x}"))),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\"{{x}}\"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_EscapeAroundHole") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::STRING_FRAGMENT, 1, "{{", VellumLiteral(std::string_view("{"))),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "x"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::STRING_FRAGMENT, 1, "}}", VellumLiteral(std::string_view("}"))),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\"{{{x}}}\"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_ExpressionInHole") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "a"),
      makeToken(TokenType::PLUS, 1, "+"),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\"{a + 1}\"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_NestedBracesInHole") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::MATCH, 1, "match"),
      makeToken(TokenType::IDENTIFIER, 1, "x"),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::INT, 1, "1", VellumLiteral(1)),
      makeToken(TokenType::FAT_ARROW, 1, "=>"),
      makeToken(TokenType::IDENTIFIER, 1, "a"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\"{match x { 1 => a }}\"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_NestedInterpolatedString") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::STRING_FRAGMENT, 1, "outer ",
               VellumLiteral(std::string_view("outer "))),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::STRING_FRAGMENT, 1, "inner ",
               VellumLiteral(std::string_view("inner "))),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::IDENTIFIER, 1, "x"),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::INTERP_END, 1, "\""),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  CHECK_THAT(scanTokens(makeUnique<Lexer>("$\"outer {$\"inner {x}\"}\"")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_StringLiteralInHole") {
  Vec<Token> expected{
      makeToken(TokenType::INTERP_START, 1, "$\""),
      makeToken(TokenType::LEFT_BRACE, 1, "{"),
      makeToken(TokenType::STRING, 1, "\"}\"", VellumLiteral(std::string_view("}"))),
      makeToken(TokenType::RIGHT_BRACE, 1, "}"),
      makeToken(TokenType::INTERP_END, 1, "\""),
  };
  // Hole contains a plain string whose contents is `}` — must not close the
  // hole. Source: $"{"}"}"
  CHECK_THAT(scanTokens(makeUnique<Lexer>(R"($"{"}"}")")),
             Catch::Matchers::Equals(expected));
}

TEST_CASE("LexerInterpolatedString_UnexpectedCloseBrace") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("$\"}\""));
  REQUIRE(tokens.size() >= 2);
  CHECK(tokens[0].type == TokenType::INTERP_START);
  CHECK(tokens[1].type == TokenType::ERROR);
}

TEST_CASE("LexerInterpolatedString_UnterminatedText") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("$\"hello"));
  REQUIRE(tokens.size() == 3);
  CHECK(tokens[0].type == TokenType::INTERP_START);
  CHECK(tokens[1].type == TokenType::STRING_FRAGMENT);
  CHECK(tokens[1].lexeme == "hello");
  CHECK(tokens[2].type == TokenType::ERROR);
  CHECK(tokens[2].lexeme == "Unterminated interpolated string.");
}

TEST_CASE("LexerInterpolatedString_UnterminatedTextEmpty") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("$\""));
  REQUIRE(tokens.size() == 2);
  CHECK(tokens[0].type == TokenType::INTERP_START);
  CHECK(tokens[1].type == TokenType::ERROR);
  CHECK(tokens[1].lexeme == "Unterminated interpolated string.");
}

TEST_CASE("LexerInterpolatedString_UnterminatedExpression") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("$\"{x"));
  REQUIRE(tokens.size() == 4);
  CHECK(tokens[0].type == TokenType::INTERP_START);
  CHECK(tokens[1].type == TokenType::LEFT_BRACE);
  CHECK(tokens[2].type == TokenType::IDENTIFIER);
  CHECK(tokens[2].lexeme == "x");
  CHECK(tokens[3].type == TokenType::ERROR);
  CHECK(tokens[3].lexeme == "Unterminated interpolated expression.");
}

TEST_CASE("LexerInterpolatedString_UnterminatedExpressionEmptyHole") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("$\"{"));
  REQUIRE(tokens.size() == 3);
  CHECK(tokens[0].type == TokenType::INTERP_START);
  CHECK(tokens[1].type == TokenType::LEFT_BRACE);
  CHECK(tokens[2].type == TokenType::ERROR);
  CHECK(tokens[2].lexeme == "Unterminated interpolated expression.");
}

TEST_CASE("LexerDollarWithoutQuoteIsError") {
  Vec<Token> tokens = scanTokens(makeUnique<Lexer>("$foo"));
  REQUIRE_FALSE(tokens.empty());
  CHECK(tokens[0].type == TokenType::ERROR);
}