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

Vec<DiagnosticMessage> analyzeFull(
    ParserResult& parseResult, std::string_view scriptName,
    const Shared<ImportLibrary>& importLibrary,
    const Shared<CompilerErrorHandler>& errorHandler) {
  auto importResolver = makeShared<ImportResolver>(errorHandler, importLibrary);
  auto builtinFunctions = makeShared<BuiltinFunctions>();
  auto resolver =
      makeShared<Resolver>(VellumObject(VellumType::identifier(scriptName)),
                           errorHandler, importLibrary, builtinFunctions);

  TypeCollector typeCollector;
  typeCollector.collect(parseResult.declarations);

  importResolver->buildImportGraph(typeCollector.getDiscoveredTypes());

  DeclarationCollector collector(errorHandler, resolver, scriptName);
  collector.collect(parseResult.declarations);

  if (errorHandler->hadError()) {
    return errorHandler->getErrors();
  }

  SemanticAnalyzer semantic(errorHandler, resolver, scriptName);
  semantic.analyze(std::move(parseResult.declarations));
  if (errorHandler->hadError()) {
    return errorHandler->getErrors();
  }

  return Vec<DiagnosticMessage>();
}

}  // namespace

AnalysisResult DocumentAnalyzer::run(std::string_view source,
                                     std::string_view scriptName,
                                     const Shared<ImportLibrary>& importLibrary,
                                     AnalysisKind kind) {
  ParseOutcome parseOutcome = parseDocument(source);

  if (parseOutcome.errorHandler->hadError()) {
    return AnalysisResult{
        .diagnostics = parseOutcome.errorHandler->getErrors(),
        .semanticTokens = buildSemanticTokensLexerFallback(source),
    };
  }

  lsp::SemanticTokens semanticTokens =
      buildSemanticTokensFromParse(parseOutcome.parseResult, source);

  if (kind == AnalysisKind::SemanticTokens) {
    return AnalysisResult{.diagnostics = {}, .semanticTokens = semanticTokens};
  }

  Vec<DiagnosticMessage> diagnostics = analyzeFull(
      parseOutcome.parseResult, scriptName, importLibrary,
      parseOutcome.errorHandler);

  return AnalysisResult{.diagnostics = std::move(diagnostics),
                        .semanticTokens = std::move(semanticTokens)};
}

}  // namespace vellum
