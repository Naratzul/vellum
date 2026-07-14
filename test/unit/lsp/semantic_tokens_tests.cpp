#include <catch2/catch_test_macros.hpp>

#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic_tokens.h"

namespace vellum {
namespace {

ParserResult parseSource(std::string_view source) {
  auto errorHandler = common::makeShared<CompilerErrorHandler>();
  Parser parser(common::makeUnique<Lexer>(source), errorHandler);
  ParserResult result = parser.parse();
  REQUIRE_FALSE(errorHandler->hadError());
  return result;
}

struct DecodedSpan {
  int line;
  int character;
  int length;
  unsigned tokenType;
};

common::Vec<DecodedSpan> decodeSpans(const lsp::SemanticTokens& tokens) {
  common::Vec<DecodedSpan> spans;
  int prevLine = 0;
  int prevChar = 0;
  for (size_t i = 0; i + 4 < tokens.data.size(); i += 5) {
    prevLine += static_cast<int>(tokens.data[i]);
    if (tokens.data[i] != 0) {
      prevChar = static_cast<int>(tokens.data[i + 1]);
    } else {
      prevChar += static_cast<int>(tokens.data[i + 1]);
    }
    spans.push_back(DecodedSpan{
        .line = prevLine,
        .character = prevChar,
        .length = static_cast<int>(tokens.data[i + 2]),
        .tokenType = tokens.data[i + 3],
    });
  }
  return spans;
}

bool hasStringSpanCovering(const common::Vec<DecodedSpan>& spans, int line,
                           int character) {
  for (const auto& span : spans) {
    if (span.tokenType !=
        static_cast<unsigned>(SemanticTokenLegendType::String)) {
      continue;
    }
    if (span.line == line && character >= span.character &&
        character < span.character + span.length) {
      return true;
    }
  }
  return false;
}

int countTokenType(const common::Vec<DecodedSpan>& spans,
                   SemanticTokenLegendType type) {
  int count = 0;
  for (const auto& span : spans) {
    if (span.tokenType == static_cast<unsigned>(type)) {
      ++count;
    }
  }
  return count;
}

}  // namespace

TEST_CASE("SemanticTokens interpolated string fragments are strings") {
  constexpr const char* kSource = R"(script TestScript {
  fun test(name: String) {
    $"Hi {name}!"
  }
}
)";

  auto result = parseSource(kSource);
  const lsp::SemanticTokens tokens =
      buildSemanticTokensFromParse(result, kSource);
  const auto spans = decodeSpans(tokens);

  // $"
  CHECK(hasStringSpanCovering(spans, 2, 4));
  // Hi
  CHECK(hasStringSpanCovering(spans, 2, 6));
  // !
  CHECK(hasStringSpanCovering(spans, 2, 15));
  // closing "
  CHECK(hasStringSpanCovering(spans, 2, 16));

  CHECK(countTokenType(spans, SemanticTokenLegendType::String) >= 4);
}

TEST_CASE("SemanticTokens interpolated string hole keeps variable token") {
  constexpr const char* kSource = R"(script TestScript {
  fun test(name: String) {
    $"Hi {name}"
  }
}
)";

  auto result = parseSource(kSource);
  const lsp::SemanticTokens tokens =
      buildSemanticTokensFromParse(result, kSource);
  const auto spans = decodeSpans(tokens);

  bool foundNameVariable = false;
  for (const auto& span : spans) {
    if (span.tokenType ==
            static_cast<unsigned>(SemanticTokenLegendType::Variable) &&
        span.line == 2 && span.character == 10 && span.length == 4) {
      foundNameVariable = true;
      break;
    }
  }
  CHECK(foundNameVariable);
}

TEST_CASE("SemanticTokens lexer fallback highlights interp fragments") {
  constexpr const char* kSource = R"($"hello {x}")";
  const lsp::SemanticTokens tokens =
      buildSemanticTokensLexerFallback(kSource);
  const auto spans = decodeSpans(tokens);

  CHECK(hasStringSpanCovering(spans, 0, 0));   // $"
  CHECK(hasStringSpanCovering(spans, 0, 2));   // hello
  CHECK(hasStringSpanCovering(spans, 0, 11));  // closing "
  CHECK(countTokenType(spans, SemanticTokenLegendType::String) >= 3);
}

}  // namespace vellum
