#include <vector>

#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"
#include "lexer/lexer.h"

using namespace vellum;

static Token makeToken(TokenType type, int line, std::string_view lexeme,
                       VellumValue value = VellumValue()) {
  Token token;
  token.type = type;
  token.lexeme = lexeme;
  token.line = line;
  token.value = value;
  return token;
}

static std::vector<Token> scanTokens(std::string_view source) {
  Lexer lexer(source);
  std::vector<Token> tokens;

  for (;;) {
    Token token = lexer.scanToken();
    if (token.type == TokenType::END_OF_FILE) {
      break;
    }
    tokens.push_back(token);
  }

  return tokens;
}

TEST_CASE("LexerScriptTest") {
  std::vector<Token> expected{
      makeToken(TokenType::SCRIPT, 1, "script"),
      makeToken(TokenType::IDENTIFIER, 1, "name", VellumIdentifier("name")),
      makeToken(TokenType::COLON, 1, ":"),
      makeToken(TokenType::IDENTIFIER, 1, "parent", VellumIdentifier("parent")),
  };

  std::string_view source = "script name : parent";
  CHECK_THAT(scanTokens(source), Catch::Matchers::Equals(expected));
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
  CHECK_THAT(scanTokens(source), Catch::Matchers::Equals(expected));
}