#include "static_analyze.h"

#include "analyze/declaration_collector.h"
#include "analyze/import_resolver.h"
#include "analyze/semantic_analyzer.h"
#include "analyze/type_collector.h"
#include "compiler/resolver.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vellum/vellum_object.h"

namespace vellum {
using common::makeShared;
using common::makeUnique;
using common::Vec;

Vec<DiagnosticMessage> StaticAnalyze::analyze(std::string_view filename,
                                              std::string_view sourceCode) {
  auto lexer = makeUnique<Lexer>(sourceCode);
  auto errorHandler = makeShared<CompilerErrorHandler>();
  auto importLibrary = makeShared<ImportLibrary>(Vec<std::string>{});
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier(filename)),
                           errorHandler, importLibrary);

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

  SemanticAnalyzer semantic(errorHandler, resolver, filename);
  const SemanticAnalyzeResult semanticResult =
      semantic.analyze(std::move(parseResult.declarations));
  if (errorHandler->hadError()) {
    return errorHandler->getErrors();
  }

  return Vec<DiagnosticMessage>();
}

}  // namespace vellum