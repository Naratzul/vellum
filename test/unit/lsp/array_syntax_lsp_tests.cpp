#include <catch2/catch_test_macros.hpp>

#include "ast_locator.h"
#include "common/types.h"
#include "compiler/compiler_error_handler.h"
#include "lexer/lexer.h"
#include "lsp_fixture.h"
#include "parser/parser.h"
#include "semantic_tokens.h"

namespace vellum {
namespace {

using vellum::lsp_test::LspTestFixture;

ParserResult parseSource(std::string_view source) {
  auto errorHandler = common::makeShared<CompilerErrorHandler>();
  Parser parser(common::makeUnique<Lexer>(source), errorHandler);
  ParserResult result = parser.parse();
  REQUIRE_FALSE(errorHandler->hadError());
  return result;
}

bool semanticTokensIncludeTypeAt(std::string_view source, unsigned line,
                                 unsigned character) {
  auto result = parseSource(source);
  const lsp::SemanticTokens tokens =
      buildSemanticTokensFromParse(result, source);

  int prevLine = 0;
  int prevChar = 0;
  for (size_t i = 0; i + 4 < tokens.data.size(); i += 5) {
    prevLine += static_cast<int>(tokens.data[i]);
    if (tokens.data[i] != 0) {
      prevChar = static_cast<int>(tokens.data[i + 1]);
    } else {
      prevChar += static_cast<int>(tokens.data[i + 1]);
    }
    const unsigned length = tokens.data[i + 2];
    const unsigned tokenType = tokens.data[i + 3];
    if (tokenType == static_cast<unsigned>(SemanticTokenLegendType::Type) &&
        static_cast<unsigned>(prevLine) == line &&
        character >= static_cast<unsigned>(prevChar) &&
        character < static_cast<unsigned>(prevChar) + length) {
      return true;
    }
  }
  return false;
}

}  // namespace

TEST_CASE("AstLocator sized array type name") {
  constexpr const char* kSource = R"(script TestScript {
  fun test() {
    var a = [Int; 4]
  }
}
)";

  auto result = parseSource(kSource);
  const auto target = AstLocator::locate(result, lsp::Position{.line = 2, .character = 13});
  REQUIRE(target.has_value());
  CHECK(target->kind == AstLocatorTargetKind::TypeReference);
  REQUIRE(target->identifier.has_value());
  CHECK(target->identifier->toString() == "Int");
}

TEST_CASE("SemanticTokens sized array type name") {
  constexpr const char* kSource = R"(script TestScript {
  fun test() {
    var a = [Int; 4]
  }
}
)";

  CHECK(semanticTokensIncludeTypeAt(kSource, 2, 13));
}

TEST_CASE("SemanticTokens array literal elements") {
  constexpr const char* kSource = R"(script TestScript {
  fun test() {
    var a = [1, 2, 3]
  }
}
)";

  auto result = parseSource(kSource);
  const lsp::SemanticTokens tokens =
      buildSemanticTokensFromParse(result, kSource);

  int numberCount = 0;
  for (size_t i = 3; i < tokens.data.size(); i += 5) {
    if (tokens.data[i] == static_cast<unsigned>(SemanticTokenLegendType::Number)) {
      ++numberCount;
    }
  }
  CHECK(numberCount >= 3);
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion sized array create type position") {
  openDoc(R"(script TrainingMannequin {
  fun test() {
    var a = [
  }
}
)");
  analyze();
  REQUIRE(hasLabel(complete(2, 13), "Int"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion array literal member access") {
  openDoc(R"(script TrainingMannequin {
  fun test() {
    [1, 2, 3].length
  }
}
)");
  analyze();
  REQUIRE(hasLabel(completeDot(2, 13), "length"));
}

TEST_CASE_METHOD(LspTestFixture, "LspCompletion sized array member access") {
  openDoc(R"(script TrainingMannequin {
  fun test() {
    [Int; 4].length
  }
}
)");
  analyze();
  REQUIRE(hasLabel(completeDot(2, 12), "length"));
}

TEST_CASE_METHOD(LspTestFixture,
                 "LspCompletion local array bare dot member access") {
  openDoc(R"(script TrainingMannequin {
  fun test() {
    var nums: [Int] = []
    nums.
  }
}
)");
  analyze();
  const auto list = completeDot(3, 8);
  REQUIRE(hasLabel(list, "length"));
  REQUIRE(hasLabel(list, "find"));
  REQUIRE(hasLabel(list, "rfind"));
}

TEST_CASE_METHOD(LspTestFixture,
                 "LspCompletion call result bare dot member access") {
  openDoc(R"(script TrainingMannequin {
  fun makeArray() -> [Int] {
    return []
  }

  fun test() {
    makeArray().
  }
}
)");
  analyze();
  const auto list = completeDot(6, 15);
  REQUIRE(hasLabel(list, "length"));
  REQUIRE(hasLabel(list, "find"));
  REQUIRE(hasLabel(list, "rfind"));
}

TEST_CASE_METHOD(LspTestFixture,
                 "LspCompletion local array partial dot member access") {
  openDoc(R"(script TrainingMannequin {
  fun test() {
    var nums: [Int] = []
    nums.l
  }
}
)");
  analyze();
  const auto list = completeDot(3, 10);
  REQUIRE(hasLabel(list, "length"));
  REQUIRE_FALSE(hasLabel(list, "find"));
}

}  // namespace vellum
