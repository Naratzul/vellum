#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>

#include "ast/decl/declaration.h"
#include "common/fs.h"
#include "compiler/compiler_error_handler.h"
#include "parser/papyrus_lexer.h"
#include "parser/papyrus_parser.h"
#include "utils.h"

using namespace vellum;
using common::makeShared;
using common::makeUnique;
using common::Shared;

namespace {

Shared<CompilerErrorHandler> parsePapyrusSource(std::string_view source,
                                              ParserResult& out) {
  auto errorHandler = makeShared<CompilerErrorHandler>();
  PapyrusParser parser(makeUnique<PapyrusLexer>(source), errorHandler);
  out = parser.parse();
  return errorHandler;
}

int countDeclarationsOfOrder(const ParserResult& result,
                             ast::DeclarationOrder order) {
  int count = 0;
  for (const auto& decl : result.declarations) {
    if (decl->getOrder() == order) {
      ++count;
    }
  }
  return count;
}

}  // namespace

TEST_CASE("PapyrusLexerSemicolonBlockComment") {
  Vec<Token> tokens = scanTokens(makeUnique<PapyrusLexer>(";/ hidden /; foo"));
  REQUIRE(tokens.size() == 1);
  CHECK(tokens[0].type == TokenType::IDENTIFIER);
  CHECK(tokens[0].lexeme == "foo");
}

TEST_CASE("PapyrusLexerLineComment") {
  Vec<Token> tokens =
      scanTokens(makeUnique<PapyrusLexer>("; line comment\nbar"));
  REQUIRE(tokens.size() == 1);
  CHECK(tokens[0].type == TokenType::IDENTIFIER);
  CHECK(tokens[0].lexeme == "bar");
}

TEST_CASE("PapyrusLexerDocStringSkipped") {
  Vec<Token> tokens = scanTokens(
      makeUnique<PapyrusLexer>("{doc comment}\nvalue"));
  REQUIRE(tokens.size() == 1);
  CHECK(tokens[0].type == TokenType::IDENTIFIER);
  CHECK(tokens[0].lexeme == "value");
}

TEST_CASE("PapyrusLexerKeywords") {
  CHECK(scanTokens(makeUnique<PapyrusLexer>("ScriptName"))[0].type ==
        TokenType::SCRIPTNAME);
  CHECK(scanTokens(makeUnique<PapyrusLexer>("EndProperty"))[0].type ==
        TokenType::ENDPROPERTY);
  CHECK(scanTokens(makeUnique<PapyrusLexer>("ElseIf"))[0].type ==
        TokenType::ELSEIF);
  CHECK(scanTokens(makeUnique<PapyrusLexer>("EndIf"))[0].type ==
        TokenType::ENDIF);
  CHECK(scanTokens(makeUnique<PapyrusLexer>("EndWhile"))[0].type ==
        TokenType::ENDWHILE);
}

TEST_CASE("PapyrusLexerLogicalOperators") {
  Vec<Token> tokens = scanTokens(makeUnique<PapyrusLexer>("a && b || c"));
  REQUIRE(tokens.size() >= 5);
  CHECK(tokens[1].type == TokenType::AND);
  CHECK(tokens[3].type == TokenType::OR);
}

TEST_CASE("PapyrusParserMinimalScript") {
  constexpr std::string_view source = R"(
ScriptName TestScript extends ObjectReference
Import Debug
ObjectReference Property Controller Auto
Int Count = 0
Event OnInit()
EndEvent
)";

  ParserResult result;
  auto errorHandler = parsePapyrusSource(source, result);

  CHECK_FALSE(errorHandler->hadError());
  CHECK(result.declarations.size() == 5);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Script) == 1);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Import) == 1);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Property) == 1);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Variable) == 1);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Function) == 1);
}

TEST_CASE("PapyrusParserFullPropertyWithHidden") {
  constexpr std::string_view source = R"(
ScriptName TestScript extends ObjectReference
Int Property LeverState Hidden
  Int Function Get()
    Return 0
  EndFunction
  Function Set(Int value)
  EndFunction
EndProperty
)";

  ParserResult result;
  auto errorHandler = parsePapyrusSource(source, result);

  CHECK_FALSE(errorHandler->hadError());
  CHECK(result.declarations.size() == 2);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Property) == 1);
}

TEST_CASE("PapyrusParserAutoSlashSemicolonRegression") {
  constexpr std::string_view source = R"(
ScriptName TestScript extends ObjectReference
Bool Property PuzzleLever Auto/;
Int Property LeverNumber
  Int Function Get()
    Return 0
  EndFunction
EndProperty
)";

  ParserResult result;
  auto start = std::chrono::steady_clock::now();
  auto errorHandler = parsePapyrusSource(source, result);
  auto elapsed = std::chrono::steady_clock::now() - start;

  CHECK(elapsed < std::chrono::seconds(1));
  CHECK(result.declarations.size() >= 3);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Property) == 2);
}

TEST_CASE("PapyrusParserStraySlashRecovery") {
  constexpr std::string_view source = R"(
ScriptName TestScript extends ObjectReference
/
Int Value
)";

  ParserResult result;
  auto errorHandler = parsePapyrusSource(source, result);

  CHECK(result.declarations.size() == 2);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Variable) == 1);
}

TEST_CASE("PapyrusParserPuzLevers03Subset") {
  constexpr std::string_view source = R"(
ScriptName puzLevers03 extends ObjectReference
;/bool Property puzzle01LeverA Auto
Bool Property puzzle01LeverB Auto
Bool Property puzzle01LeverC Auto
Bool Property puzzle01LeverD Auto
Bool Property puzzle01LeverE Auto/;
Int leverNumberVal = 0
Int Property leverNumber
  {This property is the lever number for this lever
  valid values are 1 - 5}
  Int Function Get()
    Return leverNumberVal
  EndFunction
  Function Set(Int value)
  EndFunction
EndProperty
)";

  ParserResult result;
  auto start = std::chrono::steady_clock::now();
  auto errorHandler = parsePapyrusSource(source, result);
  auto elapsed = std::chrono::steady_clock::now() - start;

  CHECK(elapsed < std::chrono::seconds(1));
  CHECK_FALSE(errorHandler->hadError());
  CHECK(result.declarations.size() == 3);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Property) == 1);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Variable) == 1);
}

TEST_CASE("PapyrusParserPuzLevers03File") {
  const std::string scriptsPath =
      "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special "
      "Edition\\Data\\Source\\Scripts\\puzLevers03.psc";

  if (!std::filesystem::exists(scriptsPath)) {
    SKIP("Skyrim scripts path not available");
  }

  std::string source = common::readFileContent(scriptsPath);
  ParserResult result;
  auto start = std::chrono::steady_clock::now();
  auto errorHandler = parsePapyrusSource(source, result);
  auto elapsed = std::chrono::steady_clock::now() - start;

  CHECK(elapsed < std::chrono::seconds(1));
  CHECK(result.declarations.size() == 12);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Script) == 1);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Import) == 1);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Property) == 3);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Variable) == 5);
  CHECK(countDeclarationsOfOrder(result, ast::DeclarationOrder::Function) == 2);
}
