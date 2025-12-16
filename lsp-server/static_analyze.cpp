#include "static_analyze.h"

#include "analyze/semantic_analyzer.h"
#include "compiler/resolver.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vellum/vellum_object.h"

namespace vellum {

std::vector<DiagnosticMessage> StaticAnalyze::analyze(
    std::string_view sourceCode) {
  auto lexer = std::make_unique<Lexer>(sourceCode);
  auto errorHandler = std::make_shared<CompilerErrorHandler>();

  Parser parser(std::move(lexer), errorHandler);
  ParserResult parseResult = parser.parse();

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

  SemanticAnalyzer semantic(errorHandler, parseResult.resolver, "");
  const SemanticAnalyzeResult semanticResult =
      semantic.analyze(std::move(parseResult.declarations));
  if (errorHandler->hadError()) {
    return errorHandler->getErrors();
  }

  return std::vector<DiagnosticMessage>();
}

}  // namespace vellum