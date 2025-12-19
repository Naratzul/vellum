#include "static_analyze.h"

#include "analyze/semantic_analyzer.h"
#include "compiler/resolver.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vellum/vellum_object.h"

namespace vellum {
using common::makeShared;
using common::makeUnique;
using common::Vec;

Vec<DiagnosticMessage> StaticAnalyze::analyze(
    std::string_view filename, std::string_view sourceCode) {
  auto lexer = makeUnique<Lexer>(sourceCode);
  auto errorHandler = makeShared<CompilerErrorHandler>();

  Parser parser(std::move(lexer), errorHandler);
  ParserResult parseResult = parser.parse();

  if (errorHandler->hadError()) {
    return errorHandler->getErrors();
  }

  VellumObject debug(VellumType::identifier("Debug"));
  debug.addFunction(VellumFunction(
      VellumIdentifier("messageBox"), VellumType::none(),
      {VellumVariable(VellumIdentifier("message"),
                      VellumType::literal(VellumLiteralType::String))}));
  debug.addFunction(VellumFunction(
      VellumIdentifier("notification"), VellumType::none(),
      {VellumVariable(VellumIdentifier("message"),
                      VellumType::literal(VellumLiteralType::String))}));

  parseResult.resolver->importObject(debug);

  SemanticAnalyzer semantic(errorHandler, parseResult.resolver,
                            std::string(filename));
  const SemanticAnalyzeResult semanticResult =
      semantic.analyze(std::move(parseResult.declarations));
  if (errorHandler->hadError()) {
    return errorHandler->getErrors();
  }

  return Vec<DiagnosticMessage>();
}

}  // namespace vellum