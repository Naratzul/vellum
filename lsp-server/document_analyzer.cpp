#include "document_analyzer.h"

#include "analyze/declaration_collector.h"
#include "analyze/import_resolver.h"
#include "analyze/semantic_analyzer.h"
#include "analyze/type_collector.h"
#include "compiler/builtin_functions.h"
#include "compiler/resolver.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic_tokens.h"
#include "vellum/vellum_object.h"

namespace vellum {
using common::makeShared;
using common::makeUnique;

namespace {

struct ParseOutcome {
  Shared<CompilerErrorHandler> errorHandler;
  ParserResult parseResult;
};

ParseOutcome parseDocument(std::string_view source) {
  auto lexer = makeUnique<Lexer>(source);
  auto errorHandler = makeShared<CompilerErrorHandler>();
  Parser parser(std::move(lexer), errorHandler);
  ParserResult parseResult = parser.parse();
  return {std::move(errorHandler), std::move(parseResult)};
}

struct FullAnalyzeOutcome {
  Vec<DiagnosticMessage> diagnostics;
  NavigationContext navigation;
};

FullAnalyzeOutcome analyzeFull(ParserResult parseResult,
                               std::string_view scriptName,
                               const Shared<ImportLibrary>& importLibrary,
                               const Shared<CompilerErrorHandler>& errorHandler) {
  NavigationContext navigation;
  navigation.parseResult = std::move(parseResult);
  navigation.parseOk = true;

  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier(scriptName)),
                           errorHandler, importLibrary, builtinFunctions);

  navigation.importResolver = importResolver;
  navigation.resolver = resolver;

  TypeCollector typeCollector;
  typeCollector.collect(navigation.parseResult.declarations);

  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  DeclarationCollector collector(errorHandler, resolver, scriptName);
  collector.collect(navigation.parseResult.declarations);

  SemanticAnalyzer semantic(errorHandler, resolver, scriptName);
  auto semanticResult =
      semantic.analyze(std::move(navigation.parseResult.declarations));
  navigation.parseResult.declarations = std::move(semanticResult.declarations);
  // Best-effort: keep resolver/AST types for completion even when diagnostics
  // report errors (e.g. undefined identifier while typing).
  navigation.semanticOk = true;

  if (errorHandler->hadError()) {
    return {.diagnostics = errorHandler->getErrors(),
            .navigation = std::move(navigation)};
  }

  return {.diagnostics = Vec<DiagnosticMessage>(),
          .navigation = std::move(navigation)};
}

}  // namespace

AnalysisResult DocumentAnalyzer::run(std::string_view source,
                                     std::string_view scriptName,
                                     const Shared<ImportLibrary>& importLibrary) {
  ParseOutcome parseOutcome = parseDocument(source);

  if (parseOutcome.errorHandler->hadError()) {
    return AnalysisResult{
        .diagnostics = parseOutcome.errorHandler->getErrors(),
        .semanticTokens = buildSemanticTokensLexerFallback(source),
        .navigation = std::nullopt,
    };
  }

  FullAnalyzeOutcome fullOutcome =
      analyzeFull(std::move(parseOutcome.parseResult), scriptName, importLibrary,
                  parseOutcome.errorHandler);

  lsp::SemanticTokens semanticTokens = buildSemanticTokensFromParse(
      fullOutcome.navigation.parseResult, source);

  return AnalysisResult{.diagnostics = std::move(fullOutcome.diagnostics),
                        .semanticTokens = std::move(semanticTokens),
                        .navigation = std::move(fullOutcome.navigation)};
}

}  // namespace vellum
