#include <catch2/catch_test_macros.hpp>

#include "lexer/token.h"
#include "lsp_locations.h"

using namespace vellum;

namespace {

unsigned rangeWidth(const lsp::Range& range) {
  return range.end.character - range.start.character;
}

}  // namespace

TEST_CASE("toLspRange converts inclusive compiler end to exclusive LSP end") {
  Token token;
  token.type = TokenType::IDENTIFIER;
  token.lexeme = "MathEx3";
  token.location = {.start = {.line = 6, .position = 7},
                    .end = {.line = 6, .position = 13}};

  const lsp::Range range = toLspRange(token);

  REQUIRE(range.start.line == 6);
  REQUIRE(range.start.character == 7);
  REQUIRE(range.end.line == 6);
  REQUIRE(range.end.character == 14);
  REQUIRE(rangeWidth(range) == token.lexeme.size());
}

TEST_CASE("toLspRange ERROR token keeps lexer exclusive end") {
  Token token;
  token.type = TokenType::ERROR;
  token.lexeme = "Unexpected character";
  token.location = {.start = {.line = 0, .position = 5},
                    .end = {.line = 0, .position = 6}};

  const lsp::Range range = toLspRange(token);

  REQUIRE(range.start.character == 5);
  REQUIRE(range.end.character == 6);
}
