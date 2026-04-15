#include "static_analyze.h"

#include "analyze/declaration_collector.h"
#include "analyze/import_resolver.h"
#include "analyze/semantic_analyzer.h"
#include "analyze/type_collector.h"
#include "compiler/builtin_functions.h"
#include "compiler/resolver.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vellum/vellum_object.h"

namespace vellum {
using common::makeShared;
using common::makeUnique;
using common::Vec;

Vec<DiagnosticMessage> StaticAnalyze::analyze(
    std::string_view filename, std::string_view sourceCode,
    const Shared<ImportLibrary>& importLibrary) {
  auto lexer = makeUnique<Lexer>(sourceCode);
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier(filename)),
                           errorHandler, importLibrary, builtinFunctions);

  Parser parser(std::move(lexer), errorHandler);
  ParserResult parseResult = parser.parse();

  if (errorHandler->hadError()) {
    return errorHandler->getErrors();
  }

  TypeCollector typeCollector;
  typeCollector.collect(parseResult.declarations);

  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  DeclarationCollector collector(errorHandler, resolver, filename);
  collector.collect(parseResult.declarations);

  if (errorHandler->hadError()) {
    return errorHandler->getErrors();
  }

  SemanticAnalyzer semantic(errorHandler, resolver, filename);
  const SemanticAnalyzeResult semanticResult =
      semantic.analyze(std::move(parseResult.declarations));
  if (errorHandler->hadError()) {
    return errorHandler->getErrors();
  }

  return Vec<DiagnosticMessage>();
}

}  // namespace vellum